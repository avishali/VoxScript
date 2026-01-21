#!/bin/bash
# Rebuild VoxScript with Ninja generator (better for VST3 compatibility)

set -e

PROJECT_DIR="/Users/avishaylidani/DEV/GitHubRepo/VoxScript"
BUILD_DIR="${PROJECT_DIR}/build-Debug"

echo "================================================"
echo "VoxScript Rebuild - Fixing Reaper Compatibility"
echo "================================================"
echo ""

cd "$PROJECT_DIR"

# Clean old build
echo "1. Cleaning old Xcode build..."
rm -rf "$BUILD_DIR"

# Set up environment
export JUCE_PATH="/Users/avishaylidani/DEV/SDK/JUCE"
export ARA_SDK_PATH="/Users/avishaylidani/DEV/SDK/ARA_SDK"

echo ""
echo "2. Configuring with Ninja generator (better VST3 compatibility)..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DJUCE_PATH="$JUCE_PATH" \
    -DARA_SDK_PATH="$ARA_SDK_PATH" \
    -DPLUGIN_DEV_MODE=ON \
    -G "Ninja"

echo ""
echo "3. Building..."
cmake --build . --config Debug --parallel

echo ""
echo "4. Manually copying to system location..."
# Remove old plugin
rm -rf ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3

# Copy new one
cp -r "$BUILD_DIR/VoxScript_artefacts/Debug/VST3/VoxScript.vst3" ~/Library/Audio/Plug-Ins/VST3/

echo ""
echo "5. Verifying installation..."
if [ -f ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ]; then
    echo "✅ Plugin installed successfully"
    ls -lh ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
else
    echo "❌ Plugin installation failed"
    exit 1
fi

echo ""
echo "================================================"
echo "REBUILD COMPLETE!"
echo "================================================"
echo ""
echo "Next steps:"
echo "1. QUIT Reaper completely"
echo "2. Open Console.app and search for 'VoxScript'"  
echo "3. In Terminal, run:"
echo "   log stream --predicate 'process == \"REAPER\"' --info --debug | grep -i voxscript"
echo "4. Launch Reaper"
echo "5. Clear Reaper's plugin cache:"
echo "   Preferences → Plug-ins → VST → Clear cache/Re-scan"
echo "6. Load VoxScript on a track"
echo ""
