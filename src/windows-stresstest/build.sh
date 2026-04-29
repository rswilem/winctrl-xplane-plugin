#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake \
    -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/../../toolchain-win.cmake" \
    "$SCRIPT_DIR"

make -j"$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)"

echo ""
echo "Build complete: $BUILD_DIR/fmc-stresstest.exe"
