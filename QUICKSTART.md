# VoxScript Quick Start with ARA2 Support

Get VoxScript Phase I (ARA Skeleton) running with proper ARA2 configuration.

## ⚡ Prerequisites Check

```bash
# Check CMake version (need 3.22+)
cmake --version

# Check for C++ compiler
clang++ --version  # or g++ --version

# Check JUCE 8 location
ls /Users/avishaylidani/DEV/SDK/JUCE/modules/juce_core

# Check ARA SDK (should already be present)
ls /Users/avishaylidani/DEV/SDK/ARA_SDK
```

## 🔧 ARA SDK Setup (Already Done!)

The ARA SDK 2.2.0 is already present in your SDK folder. If you need to verify or reinstall:

```bash
cd /Users/avishaylidani/DEV/SDK

# Check current version
cd ARA_SDK
git describe --tags
# Should show: releases/2.2.0

# If not present or wrong version, clone:
# git clone --recursive --branch releases/2.2.0 https://github.com/Celemony/ARA_SDK
```

## 🚀 Build Steps

### 1. Navigate to Project
```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
```

### 2. Set Up Environment (Optional - has defaults)
```bash
# These are already set as defaults in CMakeLists.txt:
# JUCE_PATH=/Users/avishaylidani/DEV/SDK/JUCE
# ARA_SDK_PATH=/Users/avishaylidani/DEV/SDK/ARA_SDK

# But you can override if needed:
export JUCE_PATH=/Users/avishaylidani/DEV/SDK/JUCE
export ARA_SDK_PATH=/Users/avishaylidani/DEV/SDK/ARA_SDK
```

### 3. Build Plugin
```bash
# Make build script executable (first time only)
chmod +x build.sh

# Run build (creates Debug build)
./build.sh

# Or build Release version
./build.sh release
```

### Alternative: Manual CMake Build
```bash
mkdir build
cd build

# Configure (SDK paths are auto-detected)
cmake .. -DPLUGIN_DEV_MODE=ON

# Or use Xcode
cmake .. -G Xcode -DPLUGIN_DEV_MODE=ON
open VoxScript.xcodeproj

# Build
cmake --build . --config Debug
```

## ✅ Expected Build Output

You should see:

```
==========================================
VoxScript Plugin Configuration:
  Name: VoxScript
  Version: 0.1.0
  Company: MelechDSP
  Formats: VST3;Standalone
  Dev Mode: ON
  ARA2 Enabled: TRUE

SDK Paths:
  JUCE: /Users/avishaylidani/DEV/SDK/JUCE
  ARA SDK: /Users/avishaylidani/DEV/SDK/ARA_SDK
  MelechDSP Modules: FALSE
==========================================
```

Key indicators of success:
- ✅ "ARA SDK configured: /Users/avishaylidani/DEV/SDK/ARA_SDK"
- ✅ "ARA2 Enabled: TRUE"
- ✅ No errors about missing ARA SDK
- ✅ Plugin formats include VST3 (ARA requires VST3 or AU)

## 🎛️ Testing in ARA-Compatible DAWs

### Logic Pro X (10.7+)
1. Open Logic Pro
2. Create audio track with vocal clip
3. Click "Audio FX" slot
4. Navigate to: Audio Units > MelechDSP > VoxScript
5. ✅ Check: "ARA2 Active" shows GREEN in plugin UI
6. ✅ Console logs show ARA callbacks

**Logic Pro Troubleshooting:**
- Logic must be 10.7 or later for ARA support
- Preferences > Audio > Enable ARA must be checked
- On Apple Silicon, Logic may run plugin out-of-process (PDF Section 6.2)

### Studio One (6.0+)
1. Open Studio One
2. Create audio track, drag vocal file
3. Right-click audio event
4. Select: Extensions > VoxScript
5. ✅ Plugin opens in integrated editor pane
6. ✅ "ARA2 Active" indicator green

### Reaper (6.0+)
1. Open Reaper
2. Import audio file
3. Click "FX" button on track
4. Add: VST3 > MelechDSP > VoxScript
5. ✅ Plugin window opens
6. ✅ "ARA2 Active" shows green

**Reaper Note:** Reaper instantiates multiple plugin instances (UI + audio thread). This is normal and handled by VoxScript's architecture.

### Pro Tools (2024.06+)
1. Open Pro Tools (must be 2024.06 or later)
2. Create audio track with clip
3. Right-click clip
4. Select: ARA Extensions > VoxScript
5. ✅ Plugin opens in tabbed editor
6. ✅ "ARA2 Active" indicator green

**Pro Tools Note:** ARA support added in version 2024.06. Earlier versions won't show ARA extensions.

## 🔍 Verification Checklist

Phase I ARA integration is working if:

- [x] **Build Success**
  - CMake detects both JUCE and ARA SDK
  - "ARA SDK configured" message appears
  - No ARA-related errors during compilation
  - VST3 format is built (ARA requires VST3 or AU)

- [ ] **Plugin Loads**
  - Plugin appears in DAW plugin list
  - No crashes when loading
  - UI window opens properly

- [ ] **ARA Activation**
  - "ARA2 Active" indicator shows GREEN (not orange)
  - This confirms DAW is using ARA interface

- [ ] **Console Logs** (check Xcode console or DAW logs)
  ```
  VoxScriptDocumentController: Created
  VoxScriptAudioProcessor: ARA enabled: YES
  VoxScriptAudioSource: Created
  VoxScriptPlaybackRenderer: Created
  ```

- [ ] **Timeline Integration**
  - Drag audio clip onto DAW timeline
  - Check console for "Creating audio source" log
  - Verify plugin UI updates

- [ ] **State Persistence**
  - Save DAW project
  - Close DAW
  - Reopen project
  - Plugin state should restore (no crashes)

## 🐛 Troubleshooting

### "ARA SDK path not found"
```bash
# Check if ARA SDK exists
ls /Users/avishaylidani/DEV/SDK/ARA_SDK

# If missing, clone it:
cd /Users/avishaylidani/DEV/SDK
git clone --recursive --branch releases/2.2.0 https://github.com/Celemony/ARA_SDK

# Rebuild
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
rm -rf build
./build.sh
```

### "ARA2 Not Active" (Orange Indicator)

This means the plugin loaded, but the DAW is NOT using ARA interface:

**Cause 1**: Non-ARA host
- Solution: Use Logic Pro 10.7+, Studio One 6+, Reaper 6+, or Pro Tools 2024.06+

**Cause 2**: Plugin not loaded as ARA extension
- Solution: In DAW, specifically select the ARA version or use "Extensions" menu

**Cause 3**: AU format on macOS without ARA enablement
- Solution: Check if AU supports ARA in this DAW, or use VST3

**Cause 4**: Logic Pro on Apple Silicon (out-of-process issue)
- Solution: See PDF Section 6.2 - may need Rosetta mode or IPC workaround

### Build Errors

**"juce_set_ara_sdk_path is not a function"**
```bash
# Check JUCE version (need JUCE 7+)
cd /Users/avishaylidani/DEV/SDK/JUCE
git describe --tags

# Update if needed
git fetch --tags
git checkout 8.0.0  # or latest JUCE 8.x
```

**"JucePlugin_Enable_ARA not defined"**
- This is correct! It's defined in CMakeLists.txt via target_compile_definitions
- Make sure you're building with the updated CMakeLists.txt

### Plugin Not Appearing in DAW

```bash
# Check plugin was copied to system location
ls ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3
ls ~/Library/Audio/Plug-Ins/Components/VoxScript.component

# If missing, rebuild with COPY_PLUGIN_AFTER_BUILD
cmake --build build --target VoxScript_VST3

# Force DAW to rescan plugins
# Logic Pro: Preferences > Plug-In Manager > Reset & Rescan Selection
# Studio One: Options > Locations > VST Plug-Ins > Reset Cache
```

## 📊 What Phase I Tests

### ✅ Tests (Automated in Code)
- ARA document controller creation
- Audio source detection when clip added
- Playback renderer instantiation
- Thread-safe communication between UI and audio thread

### ✅ Manual Tests (You Perform)
1. **Load Test**: Plugin loads without crashes
2. **ARA Detection**: "ARA2 Active" indicator shows green
3. **Clip Assignment**: Drag audio to timeline, verify logs
4. **State Persistence**: Save/load project maintains state
5. **Multi-Instance**: Open multiple VoxScript instances

### ❌ Not Yet Tested (Phase II+)
- Transcription (will show placeholder text)
- Audio editing (passthrough only)
- Text interaction (read-only UI)
- Phoneme alignment (placeholder visualization)

## 🎯 Success Criteria

**Phase I passes when:**
1. ✅ Builds without errors on macOS
2. ✅ ARA SDK properly detected and configured
3. ⏳ Loads in Logic Pro with "ARA2 Active" green
4. ⏳ Loads in Studio One with ARA integration
5. ⏳ Loads in Reaper with ARA support
6. ⏳ Loads in Pro Tools 2024.06+ as ARA extension
7. ⏳ Console shows proper ARA callback sequence
8. ⏳ No crashes during normal operation

**Then proceed to Phase II** (Whisper transcription)

## 🚀 Next Steps After Verification

1. **Document DAW Behavior**: Note any DAW-specific quirks
2. **Performance Baseline**: Check CPU usage with multiple instances
3. **Prepare Phase II**: Set up whisper.cpp development branch
4. **Test on Other Systems**: If available, test on different macOS versions

## 📚 References

- **ARA Documentation**: `/Users/avishaylidani/DEV/SDK/ARA_SDK/ARA_Library/html_docs/index.html`
- **JUCE ARA Guide**: From uploaded ARA.md file
- **Product Spec**: Audio_Plugin_Development_Requirements.pdf (Section 2.2, 3.1)
- **VoxScript README**: Project overview and architecture

---

**Ready to build!** 🎉

```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
./build.sh
```

Then test in your DAW and verify "ARA2 Active" shows GREEN! 🟢
