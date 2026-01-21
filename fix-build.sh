#!/bin/bash
# VoxScript Build Fix Script
# Date: 2025-01-21
# Purpose: Fix macOS SDK 26.2 issue and rebuild VoxScript

set -e  # Exit on error

echo "============================================"
echo "VoxScript Build Fix Script"
echo "============================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}ERROR: This script must be run on macOS${NC}"
    exit 1
fi

echo "Step 1: Diagnosing SDK Issue"
echo "----------------------------------------"

# Check current Xcode path
echo -e "${YELLOW}Current Xcode path:${NC}"
xcode-select -p || echo "xcode-select not found - need to install Xcode"

# Check available SDKs
echo -e "\n${YELLOW}Available SDKs:${NC}"
ls -1 /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ 2>/dev/null || echo "No SDKs found"

# Check current SDK
echo -e "\n${YELLOW}Current SDK:${NC}"
xcrun --show-sdk-path || echo "No SDK configured"
xcrun --show-sdk-version || echo "No SDK version"

echo ""
echo "Step 2: Applying Fix (Option 1 - Recommended)"
echo "----------------------------------------"

# Switch to stable Xcode
echo -e "${YELLOW}Switching to stable Xcode installation...${NC}"
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

# Reset xcode-select
echo -e "${YELLOW}Resetting xcode-select...${NC}"
sudo xcode-select --reset

# Check result
echo -e "\n${GREEN}New SDK configuration:${NC}"
xcrun --show-sdk-path
xcrun --show-sdk-version

SDK_VERSION=$(xcrun --show-sdk-version)
if [[ "$SDK_VERSION" == "26.2" ]]; then
    echo -e "${RED}WARNING: Still using SDK 26.2 (beta)${NC}"
    echo "You may need to reinstall Xcode or try Option 2 from SDK_BUILD_FIX.md"
    exit 1
else
    echo -e "${GREEN}✓ Using stable SDK version: $SDK_VERSION${NC}"
fi

echo ""
echo "Step 3: Clean Previous Build"
echo "----------------------------------------"

cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript

if [ -d "build-Debug" ]; then
    echo -e "${YELLOW}Removing old build directory...${NC}"
    rm -rf build-Debug
    echo -e "${GREEN}✓ Old build removed${NC}"
else
    echo "No previous build found (OK)"
fi

echo ""
echo "Step 4: Configure with CMake"
echo "----------------------------------------"

mkdir build-Debug
cd build-Debug

echo -e "${YELLOW}Running CMake configuration...${NC}"
cmake .. -G Ninja

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ CMake configuration successful${NC}"
else
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi

echo ""
echo "Step 5: Build VoxScript"
echo "----------------------------------------"

echo -e "${YELLOW}Building VoxScript (this may take 2-5 minutes)...${NC}"
ninja -v 2>&1 | tee build.log

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    echo "Check build.log for errors"
    exit 1
fi

echo ""
echo "Step 6: Verify Build Artifacts"
echo "----------------------------------------"

VST3_PATH="VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript"
STANDALONE_PATH="VoxScript_artefacts/Debug/Standalone/VoxScript.app/Contents/MacOS/VoxScript"

if [ -f "$VST3_PATH" ]; then
    SIZE=$(ls -lh "$VST3_PATH" | awk '{print $5}')
    echo -e "${GREEN}✓ VST3 plugin built: $SIZE${NC}"
else
    echo -e "${RED}✗ VST3 plugin NOT found${NC}"
fi

if [ -f "$STANDALONE_PATH" ]; then
    SIZE=$(ls -lh "$STANDALONE_PATH" | awk '{print $5}')
    echo -e "${GREEN}✓ Standalone app built: $SIZE${NC}"
else
    echo -e "${RED}✗ Standalone app NOT found${NC}"
fi

echo ""
echo "Step 7: Download Whisper Model"
echo "----------------------------------------"

MODEL_DIR="$HOME/Library/Application Support/VoxScript/models"
MODEL_FILE="$MODEL_DIR/ggml-base.en.bin"

if [ -f "$MODEL_FILE" ]; then
    SIZE=$(ls -lh "$MODEL_FILE" | awk '{print $5}')
    echo -e "${GREEN}✓ Model already exists: $SIZE${NC}"
else
    echo -e "${YELLOW}Downloading whisper model (~140MB)...${NC}"
    
    mkdir -p "$MODEL_DIR"
    cd "$MODEL_DIR"
    
    curl -L -o ggml-base.en.bin \
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
    
    if [ $? -eq 0 ] && [ -f "ggml-base.en.bin" ]; then
        SIZE=$(ls -lh ggml-base.en.bin | awk '{print $5}')
        echo -e "${GREEN}✓ Model downloaded: $SIZE${NC}"
    else
        echo -e "${RED}✗ Model download failed${NC}"
        echo "You can download manually from:"
        echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
    fi
fi

echo ""
echo "============================================"
echo "Build Fix Complete!"
echo "============================================"
echo ""
echo "Summary:"
echo "  SDK Version: $(xcrun --show-sdk-version)"
echo "  Build Status: SUCCESS"
echo "  VST3 Plugin: $([ -f "$VST3_PATH" ] && echo "✓" || echo "✗")"
echo "  Standalone: $([ -f "$STANDALONE_PATH" ] && echo "✓" || echo "✗")"
echo "  Model: $([ -f "$MODEL_FILE" ] && echo "✓" || echo "✗")"
echo ""
echo "Next Steps:"
echo "  1. Test in Reaper (see SDK_BUILD_FIX.md)"
echo "  2. Run 5 manual runtime tests"
echo "  3. Update LAST_RESULT.md"
echo "  4. Begin Phase III planning"
echo ""
echo "For detailed testing instructions, see:"
echo "  SDK_BUILD_FIX.md"
echo ""
