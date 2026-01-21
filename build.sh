#!/bin/bash
# VoxScript build script for macOS
# Usage: ./build.sh [debug|release]

set -e  # Exit on error

BUILD_TYPE="${1:-Debug}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-${BUILD_TYPE}"

echo "============================================"
echo "VoxScript Build Script"
echo "============================================"
echo "Build type: $BUILD_TYPE"
echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"
echo ""

# Check for JUCE_PATH
if [ -z "$JUCE_PATH" ]; then
    # Try default SDK location
    JUCE_PATH="/Users/avishaylidani/DEV/SDK/JUCE"
    if [ ! -d "$JUCE_PATH" ]; then
        echo "ERROR: JUCE_PATH not set and not found at default location: $JUCE_PATH"
        echo "Please set JUCE_PATH environment variable or place JUCE in SDK/JUCE"
        exit 1
    fi
    echo "Using JUCE from: $JUCE_PATH"
fi

# Check for ARA_SDK_PATH
if [ -z "$ARA_SDK_PATH" ]; then
    # Try default SDK location
    ARA_SDK_PATH="/Users/avishaylidani/DEV/SDK/ARA_SDK"
    if [ ! -d "$ARA_SDK_PATH" ]; then
        echo "ERROR: ARA_SDK_PATH not set and not found at default location: $ARA_SDK_PATH"
        echo "Please download ARA SDK 2.2.0:"
        echo "  cd /Users/avishaylidani/DEV/SDK"
        echo "  git clone --recursive --branch releases/2.2.0 https://github.com/Celemony/ARA_SDK"
        exit 1
    fi
    echo "Using ARA SDK from: $ARA_SDK_PATH"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring CMake..."
echo "--------------------------------------------"

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DJUCE_PATH="$JUCE_PATH" \
    -DARA_SDK_PATH="$ARA_SDK_PATH" \
    -DPLUGIN_DEV_MODE=ON \
    -G "Xcode"

echo ""
echo "Building plugin..."
echo "--------------------------------------------"

cmake --build . --config "$BUILD_TYPE" --parallel

echo ""
echo "============================================"
echo "Build complete!"
echo "============================================"
echo ""
echo "Plugin formats built:"
echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3"
echo "  Standalone: ~/Applications/VoxScript.app"
echo ""
echo "To test in a DAW:"
echo "  1. Open Logic Pro, Studio One, Reaper, or Pro Tools"
echo "  2. Create an audio track with vocal audio"
echo "  3. Insert VoxScript as an ARA plugin"
echo "  4. Check that 'ARA2 Active' shows GREEN in the plugin UI"
echo ""
