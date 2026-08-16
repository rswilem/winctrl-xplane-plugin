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

# Deploy to the Windows VM's shared folder. The exe needs fonts/ beside it: the
# font loader reads <exe dir>/fonts, and without it the font list is empty.
DEPLOY_DIR="/Volumes/New folder/stresstest"
if [ -d "$(dirname "$DEPLOY_DIR")" ]; then
    mkdir -p "$DEPLOY_DIR"
    cp "$BUILD_DIR/fmc-stresstest.exe" "$DEPLOY_DIR/"
    rm -rf "$DEPLOY_DIR/fonts"
    cp -R "$SCRIPT_DIR/../../fonts" "$DEPLOY_DIR/fonts"
    echo "Deployed to: $DEPLOY_DIR (exe + $(ls -1 "$DEPLOY_DIR/fonts" | wc -l | tr -d ' ') font files)"
else
    echo "Skipped deploy: $(dirname "$DEPLOY_DIR") is not mounted."
fi
