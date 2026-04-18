#!/bin/sh

# Help function
show_help() {
    cat << EOF
Usage: ./build_platforms.sh [OPTIONS]

Build script for the WINCTRL X-Plane plugin. Supports building for multiple platforms
and development mode with live reload to X-Plane.

OPTIONS:
    --help              Show this help message and exit

    --dev[=PATH]        Enable development mode for faster iteration
                        - Skips interactive prompts
                        - Skips distribution bundle creation
                        - Optionally specify X-Plane plugins path for live reload
                        - Defaults to building Mac only (override with --platform)

    --platform=PLATFORM Build only the specified platform
                        Available platforms: mac, win, lin
                        Can be combined with --dev for targeted development builds

EXAMPLES:
    # Interactive mode (default) - prompts for platforms and options
    ./build_platforms.sh

    # Build only for Mac
    ./build_platforms.sh --platform=mac

    # Build for Mac and Windows
    ./build_platforms.sh --platform=mac --platform=win

    # Development mode with live reload to X-Plane (Mac only by default)
    ./build_platforms.sh --dev="/Users/username/X-Plane 12/Resources/plugins"

    # Development mode, will prompt for X-Plane path
    ./build_platforms.sh --dev

    # Development mode for Windows with live reload
    ./build_platforms.sh --dev="/Users/username/X-Plane 12/Resources/plugins" --platform=win

    # Quick rebuild after changes (run after initial build)
    make -C build/mac

NOTES:
    - The SDK/ folder must be present in the project root
    - In dev mode, use 'make -C build/<platform>' for quick rebuilds
    - Linux builds require Docker

EOF
    exit 0
}

# Check for flags
DEV_MODE=false
XPLANE_PLUGIN_PATH=""
PLATFORM_OVERRIDE=""

for arg in "$@"; do
    case $arg in
        --help)
            show_help
            ;;
        --dev)
            DEV_MODE=true
            shift
            ;;
        --dev=*)
            DEV_MODE=true
            XPLANE_PLUGIN_PATH="${arg#*=}"
            shift
            ;;
        --platform=*)
            PLATFORM_OVERRIDE="${arg#*=}"
            shift
            ;;
        *)
            ;;
    esac
done

PROJECT_NAME=$(find . -name "*.xcodeproj" | sed 's/\.xcodeproj//g' | sed 's/^\.\///g' | tr '[:upper:]' '[:lower:]')
VERSION=$(grep "#define VERSION " src/include/config.h | cut -d " " -f 3 | tr -d '"')

AVAILABLE_PLATFORMS="mac win lin"

# Validate platform override if provided
if [ ! -z "$PLATFORM_OVERRIDE" ]; then
    if ! echo $AVAILABLE_PLATFORMS | grep -q $PLATFORM_OVERRIDE; then
        echo "Invalid platform: $PLATFORM_OVERRIDE. Available: $AVAILABLE_PLATFORMS"
        exit 1
    fi
    PLATFORMS="$PLATFORM_OVERRIDE"
fi

if [ "$DEV_MODE" = true ]; then
    echo "Development mode enabled."
    if [ -z "$XPLANE_PLUGIN_PATH" ]; then
        echo "Enter X-Plane plugins path (e.g., /Users/username/X-Plane 12/Resources/plugins):"
        read XPLANE_PLUGIN_PATH
    fi
    if [ -z "$PLATFORMS" ]; then
        PLATFORMS="mac"
    fi
    CLEAN_BUILD="n"
elif [ -z "$PLATFORMS" ]; then
    echo "Building $PROJECT_NAME.xpl version $VERSION. Is this correct? (y/n):"
    read CONFIRM

    if [ -z "$CONFIRM" ]; then
        CONFIRM="y"
    fi

    if [ "$CONFIRM" != "y" ]; then
        echo "Please update the version number in config.h and try again."
        exit 1
    fi

    echo "Which platforms would you like to build? ($AVAILABLE_PLATFORMS):"
    read PLATFORMS

    if [ -z "$PLATFORMS" ]; then
        PLATFORMS=$AVAILABLE_PLATFORMS
    fi
fi

for platform in $PLATFORMS; do
    if ! echo $AVAILABLE_PLATFORMS | grep -q $platform; then
        echo "Invalid platform: $platform. Exiting."
        exit 1
    fi
done

echo "Building for platforms: \033[1m$PLATFORMS\033[0m\n"

if [ ! -d "SDK" ]; then
    echo "SDK/ folder not found. Please ensure the SDK is present in the project root."
    exit 1
fi

SDK_VERSION=$(grep "#define kXPLM_Version" SDK/CHeaders/XPLM/XPLMDefs.h | awk '{print $3}' | tr -d '()')

echo "Building with SDK version $SDK_VERSION\n"

if [ "$DEV_MODE" = false ]; then
    echo "Clean build directory? (y/n):"
    read CLEAN_BUILD

    if [ -z "$CLEAN_BUILD" ]; then
        CLEAN_BUILD="n"
    fi
fi

if [ "$CLEAN_BUILD" = "y" ]; then
    echo "Cleaning build directories..."
    if [ -d "build" ]; then
        rm -rf build
    fi
fi

for platform in $PLATFORMS; do
    echo "Building $platform..."
    if [ $platform = "lin" ]; then
        docker build -t gcc-cmake -f ./docker/Dockerfile.linux . && \
        docker run --user $(id -u):$(id -g) --rm -v $(pwd):/src -w /src gcc-cmake:latest bash -c "\
        cmake -DCMAKE_CXX_FLAGS='-march=x86-64' -DCMAKE_TOOLCHAIN_FILE=toolchain-$platform.cmake -DSDK_VERSION=$SDK_VERSION -Bbuild/$platform -H. && \
        make -C build/$platform -j\$(nproc)"
    else
        if [ "$DEV_MODE" = true ] && [ ! -z "$XPLANE_PLUGIN_PATH" ]; then
            cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-$platform.cmake -DSDK_VERSION=$SDK_VERSION -DXPLANE_PLUGIN_PATH="$XPLANE_PLUGIN_PATH" -Bbuild/$platform -H.
        else
            cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-$platform.cmake -DSDK_VERSION=$SDK_VERSION -Bbuild/$platform -H.
        fi
        make -C build/$platform
    fi

    if [ $? -eq 0 ]; then
        echo "\n\n"
        echo "\033[1;32m$platform build succeeded.\033[0m\nProduct: build/$platform/${platform}_x64/${PROJECT_NAME}.xpl"
        file build/$platform/${platform}_x64/${PROJECT_NAME}.xpl
        sleep 3
    else
        echo "\033[1;31m$platform build failed.\033[0m"
        exit 1
    fi
done

echo "Building has finished."

if [ "$DEV_MODE" = true ]; then
    echo "\n\033[1;32mDevelopment build complete!\033[0m"
    if [ ! -z "$XPLANE_PLUGIN_PATH" ]; then
        # Create plugin directory structure
        PLUGIN_DIR="$XPLANE_PLUGIN_PATH/$PROJECT_NAME"
        mkdir -p "$PLUGIN_DIR"
        
        for platform in $PLATFORMS; do
            echo "Plugin installed to: $PLUGIN_DIR/${platform}_x64/$PROJECT_NAME.xpl"
        done
    fi
    echo "\nTo rebuild after changes, run: make -C build/$PLATFORMS"
    exit 0
fi

echo "Creating distribution bundle..."
if [ -d "build/dist" ]; then
    rm -rf build/dist
fi

for platform in $AVAILABLE_PLATFORMS; do
    mkdir -p build/dist/${platform}_x64
    if [ -d "build/$platform/${platform}_x64" ]; then
        cp build/$platform/${platform}_x64/${PROJECT_NAME}.xpl build/dist/${platform}_x64/${PROJECT_NAME}.xpl
    fi
done

cp -r fonts build/dist

# Only add Skunkcrafts for XP12
if [ $SDK_VERSION -ge 400 ]; then
    echo "module|https://ramonster.nl/winctrl-plugin\nname|WINCTRL\nversion|$VERSION\nlocked|false\ndisabled|false\nzone|custom" > build/dist/skunkcrafts_updater.cfg
fi

cd build
mv dist $PROJECT_NAME

if [ $SDK_VERSION -lt 400 ]; then
    XPLANE_VERSION=XP11
else
    XPLANE_VERSION=XP12
fi

VERSION=$VERSION-$XPLANE_VERSION

rm -f $PROJECT_NAME-$VERSION.zip
zip -rq $PROJECT_NAME-$VERSION.zip $PROJECT_NAME -x "*/.DS_Store" -x "*/__MACOSX/*"

mv $PROJECT_NAME dist
cd ..

echo "Bundle created. Distribution: build/dist/$PROJECT_NAME-$VERSION.zip"

./update_readme.sh
