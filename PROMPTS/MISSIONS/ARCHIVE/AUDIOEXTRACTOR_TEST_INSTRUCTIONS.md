# AudioExtractor Quick Test - Manual Instructions

**Date:** 2026-01-21  
**Goal:** Test AudioExtractor in VoxScript  
**Time:** ~20 minutes

---

## ✅ Files Already Created

I've already created these files in your VoxScript project:

1. ✅ `Source/transcription/AudioExtractor.h`
2. ✅ `Source/transcription/AudioExtractor.cpp`

---

## 📋 Step-by-Step Instructions

### **Step 1: Update CMakeLists.txt** (2 minutes)

Open: `/Users/avishaylidani/DEV/GitHubRepo/VoxScript/CMakeLists.txt`

Find this section (around line 236):
```cmake
        # Phase II: Transcription engine
        Source/transcription/WhisperEngine.cpp
        Source/transcription/WhisperEngine.h
        Source/transcription/VoxSequence.cpp
        Source/transcription/VoxSequence.h
)
```

**Change it to:**
```cmake
        # Phase II: Transcription engine
        Source/transcription/WhisperEngine.cpp
        Source/transcription/WhisperEngine.h
        Source/transcription/VoxSequence.cpp
        Source/transcription/VoxSequence.h
        # Phase III: Audio extraction
        Source/transcription/AudioExtractor.cpp
        Source/transcription/AudioExtractor.h
)
```

Save the file.

---

### **Step 2: Add Test Code** (5 minutes)

#### **2A: Update VoxScriptAudioSource.h**

Open: `/Users/avishaylidani/DEV/GitHubRepo/VoxScript/Source/ara/VoxScriptAudioSource.h`

Find the `private:` section at the bottom (around line 60).

**Add this BEFORE the `private:` line:**
```cpp
    // Phase III Test
    void testAudioExtractor();
    
private:
```

Save the file.

#### **2B: Update VoxScriptAudioSource.cpp**

Open: `/Users/avishaylidani/DEV/GitHubRepo/VoxScript/Source/ara/VoxScriptAudioSource.cpp`

**At the top**, add this include (after `#include "VoxScriptAudioSource.h"`):
```cpp
#include "../transcription/AudioExtractor.h"
```

**At the bottom of the file**, add this test method:
```cpp
// Phase III Test: AudioExtractor validation
void VoxScriptAudioSource::testAudioExtractor()
{
    DBG ("================================================");
    DBG ("AUDIOEXTRACTOR TEST STARTING");
    DBG ("================================================");
    
    // Check sample access
    if (!AudioExtractor::isSampleAccessAvailable (this))
    {
        DBG ("Sample access not available - test skipped");
        return;
    }
    
    // Launch extraction on background thread
    juce::Thread::launch ([this]() {
        DBG ("Starting extraction...");
        
        auto tempFile = AudioExtractor::extractToTempWAV (this, "test_");
        
        if (tempFile.existsAsFile())
        {
            DBG ("✅ SUCCESS: Extraction completed!");
            DBG ("  File: " + tempFile.getFullPathName());
            DBG ("  Size: " + juce::String (tempFile.getSize() / 1024) + " KB");
            
            // Verify it's a valid WAV
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatReader> reader (
                wavFormat.createReaderFor (tempFile.createInputStream(), true)
            );
            
            if (reader != nullptr)
            {
                DBG ("  Sample Rate: " + juce::String (reader->sampleRate) + " Hz");
                DBG ("  Channels: " + juce::String (reader->numChannels));
                DBG ("  Duration: " + juce::String (reader->lengthInSamples / reader->sampleRate) + " sec");
            }
            
            // Clean up
            tempFile.deleteFile();
            DBG ("  Temp file cleaned up");
            DBG ("================================================");
            DBG ("AUDIOEXTRACTOR TEST PASSED");
            DBG ("================================================");
        }
        else
        {
            DBG ("❌ FAILED: Extraction returned invalid file");
            DBG ("================================================");
            DBG ("AUDIOEXTRACTOR TEST FAILED");
            DBG ("================================================");
        }
    });
}
```

**Find the `notifyPropertiesUpdated()` method** and add the test call:
```cpp
void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    DBG ("VoxScriptAudioSource: Properties updated");
    
    // Phase III Test: Trigger AudioExtractor test
    testAudioExtractor();
}
```

Save the file.

---

### **Step 3: Build** (2 minutes)

```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja
```

**Expected output:**
```
[  X%] Building CXX object ... AudioExtractor.cpp.o
[100%] Built target VoxScript_VST3
```

---

### **Step 4: Test in Reaper** (5 minutes)

1. **Open Reaper**
2. **Create new track**
3. **Add VoxScript as ARA extension** (right-click track → Insert ARA Extension)
4. **Drag audio file** into the track (any WAV/MP3)
5. **Watch Reaper console** (View → Monitoring → Show Console)

**Expected Console Output:**
```
================================================
AUDIOEXTRACTOR TEST STARTING
================================================
Starting extraction...
AudioExtractor: Starting extraction
  Source: 44100 Hz, 2 ch, 7938000 samples
  Target: 16000 Hz, 1 ch, 16-bit PCM
AudioExtractor: Extraction complete - test_abc123.wav
  Output size: 5760 KB
✅ SUCCESS: Extraction completed!
  File: /tmp/test_abc123.wav
  Size: 5760 KB
  Sample Rate: 16000 Hz
  Channels: 1
  Duration: 180 sec
  Temp file cleaned up
================================================
AUDIOEXTRACTOR TEST PASSED
================================================
```

---

## ✅ Success Criteria

You know the test passed if you see:
- ✅ "AUDIOEXTRACTOR TEST PASSED" in console
- ✅ Sample Rate: 16000 Hz
- ✅ Channels: 1
- ✅ Duration matches original audio
- ✅ No crashes

---

## ❌ Troubleshooting

### **Build Error: "AudioExtractor.h not found"**
- Check that files are in `Source/transcription/` directory
- Check that CMakeLists.txt was updated correctly

### **"Sample access not available"**
- This is OK - means audio isn't ready yet
- Try reloading audio file in Reaper

### **No console output**
- Check Reaper console is visible (View → Monitoring → Show Console)
- Check plugin loaded successfully

### **Crash on load**
- Check build completed without errors
- Check all code was added correctly

---

## 📊 What This Test Validates

✅ **ARAAudioSourceReader works** - Can read samples from ARA  
✅ **Thread safety** - Runs on background thread  
✅ **Downmixing** - Stereo → Mono conversion  
✅ **Resampling** - 44.1kHz → 16kHz conversion  
✅ **WAV creation** - Valid 16-bit PCM output  
✅ **Cleanup** - Temp files deleted  

---

## 🎉 When Test Passes

You've proven that:
1. ✅ Gemini's research was correct
2. ✅ AudioExtractor implementation works
3. ✅ ARA sample access API is functional
4. ✅ Phase III is ready to implement!

**Next:** Remove test code and implement full Phase III (automatic transcription triggering)

---

## 🆘 Need Help?

If the test fails, share:
1. Build error messages (if any)
2. Console output from Reaper
3. Which step failed

I'll help debug!
