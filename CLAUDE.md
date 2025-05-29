# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the Luau Language Server - a Language Server Protocol (LSP) implementation for the Luau programming language. It provides IDE features like type checking, autocomplete, hover information, and more for Luau code, with special support for Roblox development environments.

## Build Commands

### Primary Build
```bash
# Configure build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the language server
cmake --build . --target Luau.LanguageServer.CLI --config Release

# Build with debug info and profiling (for development)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DLUAU_ENABLE_TIME_TRACE=ON

# Build with ASAN (Address Sanitizer)
cmake .. -DLSP_BUILD_ASAN=ON

# Build with Sentry crash reporting
cmake .. -DLSP_BUILD_WITH_SENTRY=ON
```

### Testing
```bash
# Build tests
cmake --build . --target Luau.LanguageServer.Test --config Release

# Run tests
./Luau.LanguageServer.Test
```

### VSCode Extension
```bash
cd editors/code
npm install
npm run compile          # Compile TypeScript
npm run package         # Package extension
npm run lint            # Lint TypeScript code
npm run test            # Run tests
```

### Toolchain Management
Uses Aftman for tool management. Install tools with:
```bash
aftman install
```
This installs: rojo (7.4.0-rc3), selene (0.26.1), stylua (2.0.2), lune (0.8.9)

## Running the Language Server

### Standalone Analysis
```bash
# Analyze files for type errors and lints
luau-lsp analyze [files...]

# With Roblox sourcemap support
luau-lsp analyze --sourcemap sourcemap.json [files...]

# With custom definitions
luau-lsp analyze --definitions types.d.luau [files...]

# Different output formats
luau-lsp analyze --formatter=gnu [files...]
```

### Language Server Mode
```bash
# Start LSP server (listens on stdin/stdout)
luau-lsp lsp

# With custom definitions and documentation
luau-lsp lsp --definitions types.d.luau --docs documentation.json

# With crash reporting enabled
luau-lsp lsp --enable-crash-reporting

# Debug mode (delays startup for debugger attachment)
luau-lsp lsp --delay-startup
```

## Architecture Overview

### Core Components

**LanguageServer** (`src/LanguageServer.cpp`): Main LSP server implementation that handles LSP protocol messages and dispatches to appropriate handlers.

**WorkspaceFolder** (`src/Workspace.cpp`): Manages individual workspace folders, handles file management, type checking via Luau frontend, and diagnostics generation.

**Client** (`src/Client.cpp`): Handles communication with LSP clients, manages client capabilities and configuration.

**FileResolver** (`src/WorkspaceFileResolver.cpp`): Resolves module names to file paths, handles require() resolution including Roblox-style string requires.

### Platform Support

**LSPPlatform** (`src/platform/LSPPlatform.cpp`): Base platform abstraction for different environments.

**RobloxPlatform** (`src/platform/roblox/`): Roblox-specific features including:
- Sourcemap integration with Rojo
- DataModel type resolution
- String require support
- Studio plugin integration
- API types and documentation

### LSP Operations

Located in `src/operations/`:
- **Completion**: Autocomplete functionality
- **Diagnostics**: Type checking and linting
- **Hover**: Type information on hover
- **GotoDefinition**: Navigate to symbol definitions
- **References**: Find all references to symbols
- **Rename**: Symbol renaming
- **SemanticTokens**: Syntax highlighting
- **CodeAction**: Quick fixes and refactoring
- **InlayHints**: Type annotations

### Configuration

**ClientConfiguration** (`src/include/LSP/ClientConfiguration.hpp`): Handles LSP client settings with extensive configuration options for:
- Diagnostics behavior
- Completion settings
- Type checking strictness
- Platform-specific features
- FFlag management

### Transport Layer

**StdioTransport** and **PipeTransport** (`src/transport/`): Handle communication channels between client and server.

## Key Directories

- `src/`: Core C++ language server implementation
- `src/include/`: Header files organized by component
- `src/operations/`: LSP feature implementations
- `src/platform/`: Platform-specific code (Roblox support)
- `luau/`: Luau compiler/analyzer submodule
- `editors/code/`: VSCode extension TypeScript code
- `tests/`: C++ unit tests
- `scripts/`: Python scripts for API updates and releases
- `plugin/`: Roblox Studio plugin for DataModel integration

## Development Workflow

1. **Configuration Files**: Uses `.luaurc` for Luau-specific settings, `selene.toml` for linting
2. **Type Definitions**: Global types loaded from `scripts/globalTypes.*.d.luau` files based on security level
3. **API Updates**: `scripts/dumpRobloxTypes.py` fetches latest Roblox API definitions
4. **Testing**: Extensive test suite in `tests/` with custom fixtures and test utilities
5. **FFlag System**: Supports Luau feature flags for experimental features via `--flag` arguments

## Special Features

- **Sourcemap Integration**: Rojo sourcemap parsing for DataModel instance resolution
- **String Requires**: Support for Roblox-style `require("path/to/module")` syntax
- **Nevermore Support**: Special handling for Nevermore framework string requires
- **Crash Reporting**: Optional Sentry integration for production debugging
- **Time Tracing**: Built-in profiling with `--timetrace` for performance analysis
- **Multiple Clients**: Can handle multiple workspace folders simultaneously