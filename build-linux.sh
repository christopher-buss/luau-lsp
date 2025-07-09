#!/bin/bash

# Build script for Luau Language Server on Linux
# Based on the release workflow commands

echo "Building Luau Language Server for Linux..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DLSP_STATIC_CRT:BOOL=ON

# Build the server
cmake --build . --config Release --target Luau.LanguageServer.CLI -j 3

echo "Build complete! The binary is located at: build/luau-lsp"