#!/bin/bash
# VoxScript Diagnostic Script

echo "================================================"
echo "VoxScript Debug Diagnostic"
echo "================================================"
echo ""

echo "1. Checking which VoxScript VST3 is installed:"
echo "--------------------------------------------"
if [ -f ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ]; then
    ls -lh ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
    echo ""
    echo "File info:"
    file ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
    echo ""
    echo "Last modified:"
    stat -f "%Sm" ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
else
    echo "❌ VoxScript.vst3 NOT FOUND in system plugins folder!"
fi

echo ""
echo "2. Checking build artifacts:"
echo "--------------------------------------------"
if [ -f /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ]; then
    echo "✅ Debug VST3 exists in build folder"
    ls -lh /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
    
    echo ""
    echo "Checking for debug symbols:"
    file /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
    
    echo ""
    echo "Checking if it's a Debug build (should contain DWARF debug info):"
    otool -l /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript | grep -A 5 "LC_SEGMENT_64 __DWARF" | head -10
else
    echo "❌ Debug VST3 NOT FOUND in build folder!"
fi

echo ""
echo "3. Comparing timestamps:"
echo "--------------------------------------------"
if [ -f ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ] && \
   [ -f /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ]; then
    
    INSTALLED_TIME=$(stat -f "%m" ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3/Contents/MacOS/VoxScript)
    BUILD_TIME=$(stat -f "%m" /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript)
    
    if [ "$BUILD_TIME" -gt "$INSTALLED_TIME" ]; then
        echo "⚠️  BUILD IS NEWER THAN INSTALLED VERSION!"
        echo "   Build artifact is newer - plugin wasn't copied to system location"
        echo ""
        echo "   SOLUTION: Copy the plugin manually:"
        echo "   cp -r /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3 ~/Library/Audio/Plug-Ins/VST3/"
    else
        echo "✅ Installed version matches or is newer than build"
    fi
fi

echo ""
echo "4. Testing console output:"
echo "--------------------------------------------"
echo "Checking if Reaper is running..."
if pgrep -x "REAPER" > /dev/null; then
    echo "✅ Reaper is running"
    echo ""
    echo "Getting recent Reaper logs (last 10 lines with 'VoxScript'):"
    echo "--------------------------------------------"
    log show --predicate 'process == "REAPER"' --last 5m --info --debug 2>/dev/null | grep -i voxscript | tail -10 || echo "No VoxScript messages found in recent logs"
else
    echo "❌ Reaper is NOT running"
    echo ""
    echo "When you launch Reaper, run this to see live logs:"
    echo "log stream --predicate 'process == \"REAPER\"' --info --debug | grep -i voxscript"
fi

echo ""
echo "5. Checking if debug logging is compiled in:"
echo "--------------------------------------------"
if [ -f /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript ]; then
    echo "Checking for DBG symbols in binary:"
    strings /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript | grep -i "VOXSCRIPT.*LOADED" | head -5 || echo "⚠️  Debug strings not found - might be stripped"
fi

echo ""
echo "================================================"
echo "RECOMMENDATIONS:"
echo "================================================"
echo ""
echo "1. If build is newer than installed, manually copy:"
echo "   cp -r /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3 ~/Library/Audio/Plug-Ins/VST3/"
echo ""
echo "2. Restart Reaper after copying the new plugin"
echo ""
echo "3. Monitor console in real-time when loading plugin:"
echo "   log stream --predicate 'process == \"REAPER\"' --info --debug | grep -i voxscript"
echo ""
echo "4. In Reaper, verify ARA is enabled:"
echo "   Preferences → Plug-ins → VST → Enable VST3 ARA support"
echo ""
