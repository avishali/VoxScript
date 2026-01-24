#!/bin/bash

# Navigate to VoxScript directory
cd ~/DEV/GitHubRepo/VoxScript || exit 1

# Rebuild
echo "=== Rebuilding VoxScript with buffer overflow fix + anti-hallucination ==="
cmake --build build-debug --config Debug --target VoxScript_VST3 -j8

# Check build result
if [ $? -eq 0 ]; then
    echo ""
    echo "=== BUILD SUCCESS ==="
    echo ""
    echo "Installing plugin..."
    cp -r build-debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3 ~/Library/Audio/Plug-Ins/VST3/
    echo "✓ Plugin installed to ~/Library/Audio/Plug-Ins/VST3/"
    echo ""
    echo "=== FIXES APPLIED ==="
    echo "1. ✅ Buffer overflow fix in AudioExtractor (crash fix)"
    echo "   • Increased resampler buffer safety margin (×1.5 + 128)"
    echo ""
    echo "2. ✅ Anti-hallucination fixes in Whisper"
    echo "   • params.no_context = true (prevents music classification)"
    echo "   • params.detect_language = false (forces English)"
    echo "   • params.suppress_blank = true (reduces garbage)"
    echo "   • Improved initial_prompt for lyrics focus"
    echo ""
    echo "=== TESTING INSTRUCTIONS ==="
    echo "1. Restart REAPER (important!)"
    echo "2. Load VoxScript on a vocal track"
    echo "3. Should now: (a) not crash, (b) show lyrics instead of music descriptions"
    echo ""
else
    echo ""
    echo "=== BUILD FAILED ==="
    exit 1
fi
