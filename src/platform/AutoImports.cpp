#include "Platform/AutoImports.hpp"

namespace Luau::LanguageServer::AutoImports
{
bool FindImportsVisitor::containsRequire(const std::string& module) const
{
    for (const auto& map : requiresMap)
        if (contains(map, module))
            return true;
    return false;
}

bool FindImportsVisitor::visit(Luau::AstStatLocal* local)
{
    if (local->vars.size != 1 || local->values.size != 1)
        return false;

    auto localName = local->vars.data[0];
    auto expr = local->values.data[0];

    if (!localName || !expr)
        return false;

    auto startLine = local->location.begin.line;
    auto endLine = local->location.end.line;

    if (handleLocal(local, localName, expr, startLine, endLine))
        return false;

    // Check if this is a require redefinition (e.g., local require = require(...))
    bool isRequireRedefinition = std::string(localName->name.value) == "require" && isRequire(expr);
    if (isRequireRedefinition)
    {
        requireRedefinitionLine = endLine;
    }

    // Only add normal requires to the requiresMap, not require redefinitions
    if (isRequire(expr) && !isRequireRedefinition)
    {
        firstRequireLine = !firstRequireLine.has_value() || firstRequireLine.value() >= startLine ? startLine : firstRequireLine.value();

        // If the requires are too many lines away, treat it as a new group
        if (previousRequireLine && startLine - previousRequireLine.value() > 1)
            requiresMap.emplace_back(); // Construct a new group

        requiresMap.back().emplace(std::string(localName->name.value), local);
        previousRequireLine = endLine;
    }

    return false;
}

bool FindImportsVisitor::visit(Luau::AstStatBlock* block)
{
    for (Luau::AstStat* stat : block->body)
    {
        stat->visit(this);
    }

    return false;
}

std::string makeValidVariableName(std::string name)
{
    size_t pos = 0;
    while ((pos = name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_", pos)) != std::string::npos)
    {
        name.replace(pos, 1, "_");
    }
    return name;
}

lsp::TextEdit createRequireTextEdit(const std::string& name, const std::string& path, size_t lineNumber, bool prependNewline)
{
    auto range = lsp::Range{{lineNumber, 0}, {lineNumber, 0}};
    auto importText = "local " + name + " = require(" + path + ")\n";
    if (prependNewline)
        importText = "\n" + importText;
    return {range, importText};
}

lsp::CompletionItem createSuggestRequire(const std::string& name, const std::vector<lsp::TextEdit>& textEdits, const char* sortText,
    const std::string& path, const std::string& requirePath)
{
    std::string documentation;
    for (const auto& edit : textEdits)
        documentation += edit.newText;

    lsp::CompletionItem item;
    item.label = name;
    item.labelDetails = {std::nullopt, requirePath};
    item.kind = lsp::CompletionItemKind::Module;
    item.detail = requirePath;
    item.documentation = {lsp::MarkupKind::Markdown, codeBlock("luau", documentation) + "\n\n" + path};
    item.insertText = name;
    item.sortText = sortText;

    item.additionalTextEdits = textEdits;

    return item;
}

size_t computeMinimumLineNumberForRequire(const FindImportsVisitor& importsVisitor, size_t hotCommentsLineNumber)
{
    size_t minimumLineNumber = hotCommentsLineNumber;
    size_t visitorMinimumLine = importsVisitor.getMinimumRequireLine();

    if (visitorMinimumLine > minimumLineNumber)
        minimumLineNumber = visitorMinimumLine;

    if (importsVisitor.firstRequireLine)
        minimumLineNumber = *importsVisitor.firstRequireLine >= minimumLineNumber ? (*importsVisitor.firstRequireLine) : minimumLineNumber;
    return minimumLineNumber;
}

static size_t getLengthEqual(const std::string_view& a, const std::string_view& b)
{
    size_t i = 0;
    for (; i < a.size() && i < b.size(); ++i)
    {
        if (a[i] != b[i])
            break;
    }
    return i;
}

// static bool isStringRequire(const std::string& requirePath)
// {
//     return Luau::startsWith(requirePath, "\"");
// }

size_t computeBestLineForRequire(
    const FindImportsVisitor& importsVisitor, const TextDocument& textDocument, const std::string& require, size_t minimumLineNumber)
{
    size_t lineNumber = minimumLineNumber;
    size_t bestLength = 0;

    // If we have a require redefinition and this is a string require,
    // ensure we place it after the redefinition line
    if (importsVisitor.requireRedefinitionLine.has_value())
    {
        lineNumber = std::max(lineNumber, importsVisitor.requireRedefinitionLine.value() + 1);
    }

    for (auto& group : importsVisitor.requiresMap)
    {
        for (auto& [_, stat] : group)
        {
            auto line = stat->location.end.line;

            // HACK: We read the text of the require argument to sort the lines
            // Note: requires may be in the form `require(path) :: any`, so we need to handle that too
            auto* call = stat->values.data[0]->as<Luau::AstExprCall>();
            if (auto assertion = stat->values.data[0]->as<Luau::AstExprTypeAssertion>())
                call = assertion->expr->as<Luau::AstExprCall>();
            if (!call || call->args.size != 1)
                continue;

            std::string argText;
            if (const auto stringContents = call->args.data[0]->as<Luau::AstExprConstantString>())
                argText = std::string(stringContents->value.data, stringContents->value.size);
            else
            {
                auto location = call->args.data[0]->location;
                auto range = lsp::Range{{location.begin.line, location.begin.column}, {location.end.line, location.end.column}};
                argText = textDocument.getText(range);
            }
            auto length = getLengthEqual(argText, require);

            // Force relative requires below aliased requires
            if (length == 0 && Luau::startsWith(argText, "@") && !Luau::startsWith(require, "@"))
                lineNumber = line + 1;
            else if (length > bestLength && argText < require && line >= lineNumber)
                lineNumber = line + 1;
        }
    }

    return lineNumber;
}
} // namespace Luau::LanguageServer::AutoImports