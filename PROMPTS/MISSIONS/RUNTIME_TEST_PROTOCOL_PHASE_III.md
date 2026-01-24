# VoxScript Phase III Runtime Test Protocol

**Version:** 1.0  
**Date:** 2026-01-22  
**Purpose:** Comprehensive runtime verification for Phase III (Audio Extraction & Auto-Transcription)

---

## Prerequisites Checklist

Before starting runtime tests, ensure:

- [ ] **Build Successful:** Phase III build completed without errors
- [ ] **Whisper Model Present:**
  ```bash
  ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin
  ```
  Should show ~140MB file
  
- [ ] **Test Audio Files Ready:**
  - Short file: 30-60 seconds (WAV or MP3)
  - Medium file: 3-5 minutes
  - Long file: 30+ minutes (optional)
  
- [ ] **Reaper Installed:** Version 6.0+ recommended
- [ ] **Console Access:** Xcode Console or Reaper Console configured

---

## Test Environment Setup

### Enable Console Logging in Reaper

**Option A: Xcode Console (Recommended)**
1. Open Xcode
2. Window → Devices and Simulators
3. Select your Mac
4. Open Console tab
5. Filter: "VoxScript" OR "AudioExtractor" OR "WhisperEngine"

**Option B: Reaper ReaScript Console**
1. Reaper → Actions → Show action list
2. Search: "ReaScript console"
3. Run action
4. Console will show DBG() output

**Option C: System Console.app**
```bash
open /Applications/Utilities/Console.app
# Filter: process:Reaper
```

### Prepare Test Audio Files

```bash
# Create test directory
mkdir -p ~/Desktop/VoxScript_Test_Audio

# Option 1: Record test audio (30 seconds)
say "This is a test of the VoxScript transcription plugin. \
     It should automatically extract audio from the ARA interface \
     and transcribe it using Whisper. \
     The transcription should appear in the script view." \
     -o ~/Desktop/VoxScript_Test_Audio/test_short.aiff

# Option 2: Download sample audio
# Use any speech audio file (podcasts, interviews, etc.)

# Convert to WAV if needed
afconvert -f WAVE -d LEI16 input.mp3 ~/Desktop/VoxScript_Test_Audio/test.wav
```

---

## Test Suite

### Test 1: Single Audio File Auto-Transcription ⭐ CRITICAL

**Purpose:** Verify basic automatic transcription functionality

**Test Steps:**

1. **Launch Reaper**
   ```bash
   open -a Reaper
   ```

2. **Create New Project**
   - File → New Project
   - Don't save previous project

3. **Import Audio File**
   - Drag `test_short.aiff` to empty track
   - Audio should appear as waveform

4. **Add VoxScript as ARA Extension**
   - Right-click on waveform
   - Extensions → VoxScript
   - Plugin window should open (no need to interact)

5. **Monitor Console Output**
   - Watch for automatic extraction and transcription
   - Timeline: ~10-30 seconds for 30-second audio

**Expected Console Output:**

```
VoxScriptAudioSource: Properties updated
  Sample Rate: 44100 Hz
  Channels: 1
  Duration: 30.5 seconds
  
AudioExtractor: Starting extraction
  Source: 44100 Hz, 1 ch, 1344900 samples
  Target: 16000 Hz, 1 ch
  
AudioExtractor: Resampling 44100Hz → 16000Hz using Lagrange interpolation
AudioExtractor: Writing WAV: /tmp/voxscript_a3f2d8c4.wav
AudioExtractor: Extraction complete - 61440 bytes
  Output: 16000 Hz, 1 ch, 30720 samples

VoxScriptAudioSource: Extraction complete: /tmp/voxscript_a3f2d8c4.wav
  File size: 60 KB
  
WhisperEngine: Starting transcription of /tmp/voxscript_a3f2d8c4.wav
WhisperEngine: Model loaded: ggml-base.en.bin
WhisperEngine: Processing audio (30.5 seconds)

VoxScriptAudioSource: Transcription progress: 25%
VoxScriptAudioSource: Transcription progress: 50%
VoxScriptAudioSource: Transcription progress: 75%

WhisperEngine: Transcription complete - 8 segments
VoxScriptAudioSource: Transcription complete!
  Segments: 8
  
VoxScriptAudioSource: Deleting temp file: /tmp/voxscript_a3f2d8c4.wav
```

**Verification Checklist:**

- [ ] **Automatic Trigger:** Transcription started without manual action
- [ ] **Progress Updates:** Saw 0%, 25%, 50%, 75%, 100% messages
- [ ] **Extraction Success:** Temp WAV file created in /tmp/
- [ ] **Correct Format:** WAV is 16kHz, mono, 16-bit PCM (check with `afinfo`)
- [ ] **Transcription Complete:** "Transcription complete" message appears
- [ ] **Cleanup:** Temp file deleted after completion
- [ ] **No Crashes:** Reaper remains responsive
- [ ] **No Errors:** No error messages in console

**Manual Checks:**

```bash
# During transcription, verify temp file format
afinfo /tmp/voxscript_*.wav
# Expected:
#   File type ID: WAVE
#   Data format: 1 ch, 16000 Hz, 'lpcm' (16-bit little-endian signed integer)

# After transcription, verify cleanup
ls /tmp/voxscript_*.wav
# Expected: No files found
```

**Success Criteria:**
- All checklist items ✅
- Transcription completes in reasonable time (< 2x audio duration)
- Console output matches expected pattern

---

### Test 2: Multiple Audio Files (Sequential)

**Purpose:** Verify independent transcription of multiple files

**Test Steps:**

1. **Keep Reaper Project Open** (from Test 1)

2. **Import Second Audio File**
   - Create new track (Ctrl+T / Cmd+T)
   - Drag `test_short.aiff` to new track (or different audio file)
   
3. **Add VoxScript to Second File**
   - Right-click second waveform
   - Extensions → VoxScript
   
4. **Monitor Console for Second Transcription**
   - Should see similar output as Test 1
   - Watch for different temp file UUID

**Expected Behavior:**

```
# First file already completed
VoxScriptAudioSource: Transcription complete! (Track 1)
  Segments: 8

# Second file starts
VoxScriptAudioSource: Properties updated (Track 2)
AudioExtractor: Starting extraction
AudioExtractor: Writing WAV: /tmp/voxscript_b7e4a1f3.wav  # Different UUID
[... same process as Test 1 ...]
VoxScriptAudioSource: Transcription complete! (Track 2)
  Segments: 7
```

**Verification Checklist:**

- [ ] **Independent Processing:** Second file transcribes successfully
- [ ] **No Interference:** First transcription unaffected
- [ ] **Separate Temp Files:** Different UUIDs for each file
- [ ] **Both Cleaned Up:** Both temp files deleted
- [ ] **No Errors:** Console shows success for both

**Success Criteria:**
- Both transcriptions complete successfully
- No crashes or errors
- Files process sequentially (not simultaneously - that's Phase IV)

---

### Test 3: Error Handling - Missing Model

**Purpose:** Verify graceful error handling when model file is missing

**Test Steps:**

1. **Backup Model File**
   ```bash
   mv ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin \
      ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin.bak
   ```

2. **Create New Reaper Project**
   - File → New Project

3. **Import Audio and Add VoxScript**
   - Drag test audio to track
   - Right-click → Extensions → VoxScript

4. **Monitor Console for Error Handling**

**Expected Console Output:**

```
VoxScriptAudioSource: Properties updated
  Sample Rate: 44100 Hz
  Channels: 1
  Duration: 30.5 seconds

AudioExtractor: Starting extraction
AudioExtractor: Extraction complete - 61440 bytes

VoxScriptAudioSource: Extraction complete: /tmp/voxscript_c9a2f5b1.wav
  File size: 60 KB

WhisperEngine: Starting transcription of /tmp/voxscript_c9a2f5b1.wav
WhisperEngine ERROR: Model file not found: 
  ~/Library/Application Support/VoxScript/models/ggml-base.en.bin
  
VoxScriptAudioSource: Transcription failed: Model file not found
VoxScriptAudioSource: Deleting temp file after failure
```

**Verification Checklist:**

- [ ] **Extraction Succeeds:** Audio still extracted to temp WAV
- [ ] **Error Message Clear:** Model file error clearly stated
- [ ] **No Crash:** Plugin remains stable despite error
- [ ] **Cleanup Happens:** Temp file still deleted
- [ ] **Reaper Responsive:** Can continue using DAW
- [ ] **Status Updates:** transcriptionStatus reflects error

**Recovery Test:**

5. **Restore Model File**
   ```bash
   mv ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin.bak \
      ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin
   ```

6. **Try Transcription Again**
   - Import new audio to new track
   - Add VoxScript extension
   - Should work normally now

**Verification:**
- [ ] **Recovery Successful:** Transcription works after model restored

**Success Criteria:**
- Error handled gracefully without crashes
- Clear error message shown
- Cleanup still happens
- Plugin recovers when issue resolved

---

### Test 4: Large Audio File (Optional but Recommended)

**Purpose:** Verify stability with longer audio (30+ minutes)

**Test Steps:**

1. **Prepare Large Audio File**
   ```bash
   # Option 1: Use existing podcast/interview (30-60 minutes)
   # Option 2: Concatenate test files
   ffmpeg -i test1.wav -i test2.wav -i test3.wav \
          -filter_complex concat=n=3:v=0:a=1 \
          ~/Desktop/VoxScript_Test_Audio/test_large.wav
   ```

2. **Import Large File to Reaper**
   - New project
   - Import large audio file
   - Add VoxScript extension

3. **Monitor Process**
   - Open Activity Monitor
   - Watch CPU and Memory usage
   - Monitor console for progress

**Expected Timeline:**
- 30-minute audio: ~10-15 minutes processing
- 60-minute audio: ~20-30 minutes processing

**Verification Checklist:**

- [ ] **Extraction Completes:** Large file extracted without errors
- [ ] **Memory Reasonable:** RAM usage < 1GB during extraction
- [ ] **Memory Reasonable:** RAM usage < 2GB during transcription
- [ ] **CPU Usage:** High but not 100% (shows threading works)
- [ ] **Progress Updates:** Smooth progression 0% → 100%
- [ ] **No Timeout:** Process completes (even if takes >30 min)
- [ ] **No Crash:** Plugin stable throughout
- [ ] **Cleanup:** Temp file deleted after completion

**Manual Checks:**

```bash
# During extraction, check temp file size
ls -lh /tmp/voxscript_*.wav
# Expected: ~size proportional to duration (1 minute ≈ 2MB)

# Monitor memory
top -pid $(pgrep REAPER)
# Watch RSIZE column (resident memory)
```

**Success Criteria:**
- Large file processes successfully
- No memory leaks (memory returns to normal after completion)
- No crashes or hangs
- Reasonable processing time (< 2x audio duration)

---

### Test 5: Stereo → Mono Conversion

**Purpose:** Verify proper stereo downmixing

**Test Steps:**

1. **Prepare Stereo Audio**
   ```bash
   # Ensure test file is stereo
   afinfo test_stereo.wav | grep channels
   # Should show: 2 channels
   ```

2. **Import and Process Stereo File**
   - Import stereo file to Reaper
   - Add VoxScript extension
   - Monitor console

**Expected Console Output:**

```
VoxScriptAudioSource: Properties updated
  Sample Rate: 44100 Hz
  Channels: 2  ← Note stereo input
  Duration: 30.5 seconds

AudioExtractor: Starting extraction
  Source: 44100 Hz, 2 ch, 1344900 samples  ← Stereo source
  Target: 16000 Hz, 1 ch  ← Mono target
  
AudioExtractor: Downmixing stereo to mono  ← Explicit downmix
AudioExtractor: Resampling 44100Hz → 16000Hz
AudioExtractor: Extraction complete

[... continues normally ...]
```

**Verification Checklist:**

- [ ] **Downmix Logged:** Console shows "Downmixing stereo to mono"
- [ ] **Correct Output:** Temp WAV is mono (1 channel)
- [ ] **No Clipping:** Audio not distorted from downmix
- [ ] **Transcription Works:** Mono audio transcribes successfully

**Manual Check:**
```bash
afinfo /tmp/voxscript_*.wav | grep channels
# Expected: 1 ch
```

**Success Criteria:**
- Stereo audio properly converted to mono
- No audio quality issues
- Transcription completes normally

---

### Test 6: Different Sample Rates

**Purpose:** Verify resampling works for various input sample rates

**Test Steps:**

1. **Prepare Audio at Different Sample Rates**
   ```bash
   # 48kHz (common for video)
   afconvert -f WAVE -d LEI16 -r 48000 test.wav test_48k.wav
   
   # 96kHz (high-res audio)
   afconvert -f WAVE -d LEI16 -r 96000 test.wav test_96k.wav
   
   # 22.05kHz (old recordings)
   afconvert -f WAVE -d LEI16 -r 22050 test.wav test_22k.wav
   ```

2. **Test Each Sample Rate**
   - Import each file separately
   - Add VoxScript extension
   - Verify console output shows resampling

**Expected Pattern:**

```
# For 48kHz input
AudioExtractor: Resampling 48000Hz → 16000Hz

# For 96kHz input
AudioExtractor: Resampling 96000Hz → 16000Hz

# For 22.05kHz input
AudioExtractor: Resampling 22050Hz → 16000Hz
```

**Verification Checklist:**

- [ ] **48kHz:** Downsamples correctly
- [ ] **96kHz:** Downsamples correctly (2x CPU for processing)
- [ ] **22.05kHz:** Upsamples correctly
- [ ] **All Output 16kHz:** All temp files are 16000 Hz

**Success Criteria:**
- All sample rates convert correctly to 16kHz
- Transcription works for all inputs
- No audio artifacts or errors

---

## Troubleshooting Guide

### Issue: No Console Output

**Symptoms:** Nothing appears in console after adding VoxScript

**Possible Causes:**
1. Build configuration issue (Release vs Debug)
2. Console filtering too restrictive
3. DBG() output disabled

**Solutions:**
```bash
# Rebuild in Debug mode
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
rm -rf build-Debug
mkdir build-Debug
cd build-Debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja

# Check console filter (should include "VoxScript", "AudioExtractor", "WhisperEngine")
```

---

### Issue: Transcription Never Starts

**Symptoms:** Extraction completes but transcription doesn't start

**Check:**
```bash
# 1. Verify temp file exists during extraction
ls -la /tmp/voxscript_*.wav

# 2. Check temp file format
afinfo /tmp/voxscript_*.wav

# 3. Verify model file
ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin
```

**Common Causes:**
- Temp file not created (permissions issue)
- Model file missing or corrupted
- WhisperEngine initialization failed

---

### Issue: Temp Files Not Cleaned Up

**Symptoms:** Multiple `/tmp/voxscript_*.wav` files accumulate

**Check:**
```bash
# List all temp files
ls -lh /tmp/voxscript_*.wav

# Check file age
stat -f "%Sm" /tmp/voxscript_*.wav
```

**If files persist:**
1. Check console for "Deleting temp file" messages
2. Verify transcriptionComplete() and transcriptionFailed() callbacks fire
3. Manual cleanup: `rm /tmp/voxscript_*.wav`

---

### Issue: Reaper Crashes or Freezes

**Symptoms:** Reaper becomes unresponsive during transcription

**Immediate Action:**
1. Force quit Reaper
2. Check crash log:
   ```bash
   open ~/Library/Logs/DiagnosticReports/
   # Look for REAPER_*.crash files
   ```

**Common Causes:**
- Audio thread allocation (check code)
- Excessive memory usage (check Activity Monitor)
- TLS violation (ARAAudioSourceReader on wrong thread)

**Report Issue:**
Include crash log, console output, and audio file details in bug report

---

## Test Result Template

After completing all tests, document results:

```markdown
# VoxScript Phase III Runtime Test Results

**Date:** [YYYY-MM-DD]
**Tester:** [Name]
**Build:** [Commit hash or date]
**Environment:** macOS [version], Reaper [version]

## Test Results Summary

| Test | Status | Duration | Notes |
|------|--------|----------|-------|
| 1. Single File Auto-Transcription | âœ…/âŒ | X min | |
| 2. Multiple Files Sequential | âœ…/âŒ | X min | |
| 3. Error Handling - Missing Model | âœ…/âŒ | X min | |
| 4. Large Audio File (Optional) | âœ…/âŒ/SKIP | X min | |
| 5. Stereo → Mono | âœ…/âŒ | X min | |
| 6. Different Sample Rates | âœ…/âŒ | X min | |

**Overall Status:** [PASS/FAIL]

## Detailed Results

### Test 1: Single File Auto-Transcription
**Status:** [âœ…/âŒ]
**Audio File:** test_short.aiff (30 sec, 44.1kHz, mono)
**Extraction Time:** X seconds
**Transcription Time:** X seconds
**Console Output:** [snippet or "See full log"]

**Issues:** [None / describe issues]

[Repeat for each test]

## Issues Found

1. [Issue description]
   - Severity: Critical/Major/Minor
   - Reproducibility: Always/Sometimes/Once
   - Workaround: [If any]

## Recommendations

1. [Suggestion for improvement]

## Conclusion

Phase III runtime verification: [PASS/FAIL]
Ready for ARCHITECT review: [YES/NO]
```

---

## Quick Reference: Essential Commands

```bash
# Check model exists
ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin

# Check temp files during transcription
ls -la /tmp/voxscript_*.wav

# Verify temp file format
afinfo /tmp/voxscript_*.wav

# Clean up orphaned temp files
rm /tmp/voxscript_*.wav

# Monitor Reaper process
top -pid $(pgrep REAPER)

# View recent crash logs
ls -lt ~/Library/Logs/DiagnosticReports/REAPER_*.crash | head -5

# Rebuild plugin
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja && echo "Build successful"
```

---

## Success Criteria Summary

**Minimum Requirements for PASS:**
- Test 1: âœ… PASS (critical)
- Test 2: âœ… PASS (critical)
- Test 3: âœ… PASS (critical)
- Test 4: SKIP acceptable
- Test 5: âœ… PASS (important)
- Test 6: âœ… PASS (important)

**Target:** 5/6 tests PASS (Test 4 optional)
**Excellent:** 6/6 tests PASS

---

## Timeline Estimate

- Setup (Prerequisites): 15 minutes
- Test 1: 15 minutes
- Test 2: 10 minutes
- Test 3: 10 minutes
- Test 4: 30 minutes (if included)
- Test 5: 10 minutes
- Test 6: 15 minutes
- Documentation: 15 minutes

**Total:** ~90 minutes (without Test 4) or ~2 hours (with Test 4)

---

**Document Version:** 1.0  
**Last Updated:** 2026-01-22  
**Status:** Ready for use
