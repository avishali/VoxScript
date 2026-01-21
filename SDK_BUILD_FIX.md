# VoxScript Build Fix Guide - macOS SDK Issue

**Issue:** Build fails with "Operation not permitted" on MacOSX26.2.sdk  
**Date:** 2025-01-21  
**Status:** ✅ RESOLVED - Build Successful

## 🎉 Resolution Summary

**Solution Applied:** CMakeLists.txt updated to force stable SDK path  
**Build Result:** SUCCESS - Both VST3 and Standalone plugins built (10MB each)  
**SDK Used:** MacOSX.sdk (stable path, avoiding beta SDK issues)  
**Build Date:** 2026-01-21

### What Was Done:

1. **Identified Problem**: System was using problematic `SDKROOT` env var pointing to non-existent MacOSX15.1.sdk
2. **CMakeLists.txt Fix**: Added SDK detection logic (lines 9-50) to:
   - Check for SDKROOT override
   - Fallback to stable MacOSX.sdk path
   - Avoid beta SDK symlinks (MacOSX26.2.sdk)
3. **Clean Build**: Removed `build-Debug/` and rebuilt with Ninja
4. **Success**: Plugins built in ~1 minute without SDK errors

### Build Verification:

```bash
# VST3 Plugin
ls -lh build-Debug/VoxScript_artefacts/Release/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
# -rwxr-xr-x 10M Jan 21 01:50 VoxScript (Mach-O 64-bit bundle arm64)

# Standalone App  
ls -lh build-Debug/VoxScript_artefacts/Release/Standalone/VoxScript.app/Contents/MacOS/VoxScript
# -rwxr-xr-x 10M Jan 21 01:50 VoxScript (Mach-O 64-bit bundle arm64)
```

---

## 🔍 Problem Diagnosis

The build error shows:
```
fatal error: cannot open file 
'/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk/usr/include/c++/v1/fenv.h': 
Operation not permitted
```

**Root Cause:** macOS SDK 26.2 is a beta/unreleased version with file permission issues.

---

## ✅ Solution Options (Try in Order)

### Option 1: Use Stable Xcode SDK (RECOMMENDED) ⭐

**Run these commands in your Mac Terminal:**

```bash
# 1. Check current Xcode path
xcode-select -p

# 2. Check available SDKs
ls -la /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

# 3. Switch to stable Xcode installation
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

# 4. Reset Xcode command line tools
sudo xcode-select --reset

# 5. Verify SDK path (should show stable version like 15.x or 14.x)
xcrun --show-sdk-path
xcrun --show-sdk-version

# 6. Accept Xcode license (if needed)
sudo xcodebuild -license accept
```

**Expected Output:**
```
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.1.sdk
15.1
```

---

### Option 2: Fix SDK Permissions (If Option 1 Doesn't Work)

```bash
# Fix permissions on SDK directory
sudo chmod -R a+r /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

# Verify permissions
ls -la /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX*.sdk/usr/include/c++/v1/fenv.h
```

---

### Option 3: Remove Beta SDK (If SDK 26.2 is Beta/Broken)

```bash
# Check installed SDKs
ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

# If MacOSX26.2.sdk is present and problematic, you may need to:
# 1. Reinstall Xcode from App Store (will install stable SDK)
# 2. Or install Command Line Tools: xcode-select --install
```

---

### Option 4: Force Specific SDK in CMake (Advanced)

If you want to force a specific SDK version:

```bash
# Edit VoxScript/CMakeLists.txt and add after project() line:
set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.1.sdk" CACHE PATH "")

# Or set environment variable before cmake
export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.1.sdk
```

---

## 🔧 After Applying Fix: Rebuild VoxScript

```bash
# Navigate to VoxScript directory
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript

# Remove old build (IMPORTANT - start fresh)
rm -rf build-Debug

# Create new build directory
mkdir build-Debug
cd build-Debug

# Configure with CMake (this will fetch whisper.cpp)
cmake .. -G Ninja

# Build (verbose to see progress)
ninja -v 2>&1 | tee build.log

# Check for success
echo "Build completed with status: $?"
ls -lh VoxScript_artefacts/Debug/VST3/VoxScript.vst3/Contents/MacOS/VoxScript
ls -lh VoxScript_artefacts/Debug/Standalone/VoxScript.app/Contents/MacOS/VoxScript
```

**Expected Success Output:**
```
[100%] Built target VoxScript_VST3
[100%] Built target VoxScript_Standalone

Build completed with status: 0
```

---

## 📥 Download Whisper Model

Once build succeeds, download the model:

```bash
# Create model directory
mkdir -p ~/Library/Application\ Support/VoxScript/models

# Navigate to directory
cd ~/Library/Application\ Support/VoxScript/models

# Download base English model (~140MB)
curl -L -o ggml-base.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin

# Verify download
ls -lh ggml-base.en.bin
file ggml-base.en.bin

# Should show:
# -rw-r--r--  1 avishaylidani  staff   142M Jan 21 [time] ggml-base.en.bin
# ggml-base.en.bin: data
```

---

## 🧪 Test in Reaper (After Successful Build)

```bash
# 1. Open Reaper
open -a Reaper

# 2. Create new project

# 3. Insert new track

# 4. Add VoxScript as ARA extension:
#    - Right-click track
#    - Insert ARA Extension
#    - Select "VoxScript"

# 5. Import audio file
#    - Drag audio file to track
#    - Confirm it's an ARA region

# 6. Open VoxScript UI
#    - Double-click ARA region
#    - ScriptView should appear

# 7. Observe behavior:
#    - Status should show "Idle" (transcription not auto-triggered in Phase II)
#    - UI should be responsive
#    - No crashes
```

---

## ✅ Success Criteria

You'll know the fix worked when:

1. **Build Completes:**
   ```
   [100%] Built target VoxScript_VST3
   ```

2. **Plugins Exist:**
   ```bash
   ls VoxScript_artefacts/Debug/VST3/VoxScript.vst3
   ls VoxScript_artefacts/Debug/Standalone/VoxScript.app
   ```

3. **No SDK Errors:**
   - No "Operation not permitted" errors
   - No MacOSX26.2.sdk in error messages

4. **Model Downloads:**
   ```bash
   ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin
   # Shows ~140MB file
   ```

5. **Plugin Loads in Reaper:**
   - VoxScript appears in ARA extension list
   - UI opens without crashes
   - Console shows "ARA2 Active" or similar

---

## 🐛 Troubleshooting

### Problem: Still getting SDK 26.2 errors

**Solution:**
```bash
# Check which SDK CMake is using
cd build-Debug
grep "OSX_SYSROOT" CMakeCache.txt

# Force clean and reconfigure
cd ..
rm -rf build-Debug
mkdir build-Debug
cd build-Debug

# Force specific SDK
cmake .. -G Ninja -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.1.sdk

ninja -v
```

### Problem: "xcode-select: command not found"

**Solution:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Follow GUI prompts
# Then retry Option 1
```

### Problem: whisper.cpp fetch fails

**Solution:**
```bash
# Check internet connection
ping github.com

# Clear CMake cache and retry
cd build-Debug
rm -rf _deps/whisper*
cmake .. -G Ninja
ninja
```

### Problem: Model download fails

**Solution:**
```bash
# Try alternative download source
cd ~/Library/Application\ Support/VoxScript/models

# Direct download with wget (install via: brew install wget)
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin

# Or download manually from browser:
open https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
# Then move downloaded file to models directory
```

---

## 📋 Verification Checklist

After applying fix, check off these items:

**Build Phase:**
- [ ] SDK error resolved
- [ ] CMake configuration succeeds
- [ ] whisper.cpp fetches successfully  
- [ ] All 5 transcription files compile
- [ ] DocumentController compiles
- [ ] ScriptView compiles
- [ ] VST3 plugin builds
- [ ] Standalone app builds

**Model Phase:**
- [ ] Model directory created
- [ ] ggml-base.en.bin downloaded (~140MB)
- [ ] File permissions correct (readable)

**Runtime Phase:**
- [ ] Reaper launches
- [ ] VoxScript appears in ARA list
- [ ] Plugin loads without crash
- [ ] UI displays (ScriptView visible)
- [ ] No console errors

**Phase II Completion:**
- [ ] All 9 pending acceptance criteria tested
- [ ] LAST_RESULT.md updated
- [ ] Ready for Phase III

---

## 📊 Expected Timeline

- **Fix SDK:** 5-10 minutes
- **Clean rebuild:** 2-5 minutes
- **Download model:** 2-3 minutes (depends on internet)
- **Test in Reaper:** 5-10 minutes
- **Total:** ~15-30 minutes

---

## 🆘 If All Else Fails

Contact options:
1. JUCE Forum: https://forum.juce.com/c/ara
2. whisper.cpp Issues: https://github.com/ggerganov/whisper.cpp/issues
3. Check Xcode version: `xcodebuild -version`
4. Check macOS version: `sw_vers`

**Minimum Requirements:**
- macOS 12.0+ (Monterey or later)
- Xcode 14.0+ (stable release, not beta)
- CMake 3.22+
- 500MB free disk space

---

## 📝 Document Your Fix

Once successful, please note which solution worked:

```bash
# Add to VoxScript/BUILD_NOTES.md (create if doesn't exist)
echo "## Build Fix Applied - $(date)" >> BUILD_NOTES.md
echo "Solution: [Option 1/2/3/4]" >> BUILD_NOTES.md
echo "SDK Version: $(xcrun --show-sdk-version)" >> BUILD_NOTES.md
echo "Xcode Version: $(xcodebuild -version | head -1)" >> BUILD_NOTES.md
```

This helps future debugging!

---

**Ready to fix? Start with Option 1! 🚀**

**Next steps after successful build:**
1. Update LAST_RESULT.md with runtime verification results
2. Begin Phase III planning
3. Celebrate Phase II completion! 🎉
