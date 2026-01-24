# Phase III Diagnostic - No Console Output

**Issue:** Plugin loads but shows "Idle", no console output, no transcription

---

## Possible Causes

### 1. notifyPropertiesUpdated() Not Being Called

**Symptoms:**
- No "PROPERTIES UPDATED!!!" in console
- No "AUDIO SOURCE CREATED!!!" in console
- Plugin stuck on "Idle"

**Check:**
```bash
# Open Console.app and filter for "VoxScript" or "VOXSCRIPT"
# When you drag audio, you should see:
# "AUDIO SOURCE CREATED!!!"
# "PROPERTIES UPDATED!!!"
```

**If you DON'T see these messages:**
- notifyPropertiesUpdated() is not being called by ARA
- OR console output is not configured correctly

---

### 2. Console Output Not Visible

**Check Console Configuration:**

```cpp
// These should be in VoxScriptAudioSource.cpp:
std::cerr << "\n\n******************************\n";
std::cerr << "AUDIO SOURCE CREATED!!!\n";
std::cerr << "******************************\n\n";

DBG ("VoxScriptAudioSource: Created");
```

**Viewing Options:**
1. **Xcode Console:**
   - Open Xcode
   - Window → Devices and Simulators
   - Select Mac → Open Console
   - Filter: "VoxScript"

2. **System Console.app:**
   - `/Applications/Utilities/Console.app`
   - Device: Your Mac
   - Filter: process:REAPER

3. **Reaper Console:**
   - Extensions → ReaScript → Show Console output
   - May not show std::cerr output

---

### 3. Plugin Not Loading as ARA

**Symptoms:**
- No console output at all
- Plugin shows but doesn't respond

**Check in Reaper:**
1. Right-click waveform → Extensions
2. Is "VoxScript" listed?
3. When you click it, does plugin window open?

**If NOT showing as ARA extension:**
- Plugin not properly registered as ARA
- CMake build configuration issue
- Need to rescan plugins

---

### 4. Wrong Callback Being Used

**The current code uses:**
```cpp
void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    std::cerr << "PROPERTIES UPDATED!!!\n";
    testAudioExtractor();  // This doesn't trigger transcription!
}
```

**But maybe ARA isn't calling this method at all!**

---

## Diagnostic Steps

### Step 1: Add More Logging

**Open:** `Source/ara/VoxScriptDocumentController.cpp`

**Add to doCreateAudioSource():**

```cpp
juce::ARAAudioSource* VoxScriptDocumentController::doCreateAudioSource (
    juce::ARADocument* document,
    ARA::ARAAudioSourceHostRef hostRef) noexcept
{
    std::cerr << "\n========================================\n";
    std::cerr << "DOCUMENT CONTROLLER: CREATING AUDIO SOURCE\n";
    std::cerr << "========================================\n";
    
    DBG ("VoxScriptDocumentController: Creating Audio Source");
    
    auto* audioSource = new VoxScriptAudioSource (document, hostRef);
    
    std::cerr << "Audio Source Created: " << (void*)audioSource << "\n";
    std::cerr << "Sample Rate: " << audioSource->getSampleRate() << " Hz\n";
    std::cerr << "Channels: " << audioSource->getChannelCount() << "\n";
    std::cerr << "Sample Count: " << audioSource->getSampleCount() << "\n";
    std::cerr << "Sample Access Enabled: " << (audioSource->isSampleAccessEnabled() ? "YES" : "NO") << "\n";
    std::cerr << "========================================\n\n";
    
    whisperEngine.addListener (this);
    
    return audioSource;
}
```

### Step 2: Add Logging to doCreatePlaybackRegion()

```cpp
juce::ARAPlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion (
    juce::ARAAudioModification* modification,
    ARA::ARAPlaybackRegionHostRef hostRef) noexcept
{
    std::cerr << "\n========================================\n";
    std::cerr << "DOCUMENT CONTROLLER: CREATING PLAYBACK REGION\n";
    std::cerr << "========================================\n";
    
    auto* audioSource = modification->getAudioSource();
    if (audioSource)
    {
        std::cerr << "Audio Source: " << (void*)audioSource << "\n";
        std::cerr << "Sample Access NOW: " << (audioSource->isSampleAccessEnabled() ? "YES" : "NO") << "\n";
        
        // Try triggering transcription HERE instead
        if (audioSource->isSampleAccessEnabled())
        {
            std::cerr << "TRIGGERING TRANSCRIPTION FROM PLAYBACK REGION\n";
            
            auto* voxSource = dynamic_cast<VoxScriptAudioSource*>(audioSource);
            if (voxSource)
            {
                // Trigger transcription!
                juce::Thread::launch([voxSource, this]() {
                    auto tempFile = AudioExtractor::extractToTempWAV(voxSource, "voxscript_");
                    if (tempFile.existsAsFile())
                    {
                        whisperEngine.transcribeAudioFile(tempFile);
                    }
                });
            }
        }
    }
    std::cerr << "========================================\n\n";
    
    return new juce::ARAPlaybackRegion (modification, hostRef);
}
```

### Step 3: Rebuild and Test

```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja

# Watch console
tail -f ~/Library/Logs/VoxScript.log
# OR open Console.app
```

### Step 4: Test in Reaper

1. Open Reaper
2. Create new project
3. Drag audio file to track
4. Right-click waveform → Extensions → VoxScript
5. **Watch for console output**

---

## Expected Output (If Working)

```
========================================
DOCUMENT CONTROLLER: CREATING AUDIO SOURCE
========================================
Audio Source Created: 0x12345678
Sample Rate: 44100 Hz
Channels: 2
Sample Count: 7938000
Sample Access Enabled: NO  ← Key insight!
========================================

******************************
AUDIO SOURCE CREATED!!!
******************************

(Audio properties not yet finalized...)

========================================
DOCUMENT CONTROLLER: CREATING PLAYBACK REGION
========================================
Audio Source: 0x12345678
Sample Access NOW: YES  ← Sample access available HERE!
TRIGGERING TRANSCRIPTION FROM PLAYBACK REGION
========================================

AudioExtractor: Starting extraction
  Source: 44100 Hz, 2 ch, 7938000 samples
  Target: 16000 Hz, 1 ch
...
```

---

## Likely Scenario

**I suspect:**
- Sample access is **NOT** available in `notifyPropertiesUpdated()`
- Sample access **IS** available in `doCreatePlaybackRegion()`
- We need to trigger transcription from `doCreatePlaybackRegion()` instead!

**This would explain:**
- ✅ Why notifyPropertiesUpdated() isn't working
- ✅ Why plugin is stuck on "Idle"
- ✅ Why no transcription happens

---

## Quick Fix (Try This)

**If sample access is available in doCreatePlaybackRegion():**

**File:** `Source/ara/VoxScriptDocumentController.cpp`

**Replace doCreatePlaybackRegion():**

```cpp
juce::ARAPlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion (
    juce::ARAAudioModification* modification,
    ARA::ARAPlaybackRegionHostRef hostRef) noexcept
{
    DBG ("VoxScriptDocumentController: Creating Playback Region");
    
    // Get audio source
    auto* audioSource = modification->getAudioSource();
    
    // Check if this is the first playback region for this source
    // (to avoid multiple transcriptions)
    auto* voxSource = dynamic_cast<VoxScriptAudioSource*>(audioSource);
    
    if (voxSource && !voxSource->isTranscriptionReady() && audioSource->isSampleAccessEnabled())
    {
        DBG ("VoxScriptDocumentController: Triggering transcription for new playback region");
        
        // Trigger transcription on background thread
        juce::Thread::launch([voxSource, this]() {
            auto tempFile = AudioExtractor::extractToTempWAV(voxSource, "voxscript_");
            
            if (tempFile.existsAsFile())
            {
                DBG ("Temp file created: " + tempFile.getFullPathName());
                whisperEngine.transcribeAudioFile(tempFile);
                
                // Cleanup handled by WhisperEngine callbacks
                tempFile.deleteFile();
            }
        });
    }
    
    return new juce::ARAPlaybackRegion (modification, hostRef);
}
```

**Then rebuild and test!**

---

## Report Back

Please tell me:
1. Do you see **ANY** console output when dragging audio?
2. What messages appear (copy/paste)?
3. Does sample access show "YES" or "NO" in doCreateAudioSource()?
4. Does sample access show "YES" or "NO" in doCreatePlaybackRegion()?

This will tell us exactly where to trigger transcription!
