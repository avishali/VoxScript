## 🔧 Quick Fix Instructions

**Issue:** Reaper crashes when adding audio file
**Cause:** Audio extraction happens before audio source is fully initialized
**Fix:** Move extraction to doCreateAudioModification()

---

### Apply These Changes

**File:** `Source/ara/VoxScriptDocumentController.cpp`

**Change 1:** Find `didAddAudioSourceToDocument()` (around line 48) and replace with:

```cpp
void VoxScriptDocumentController::didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource)
{
    juce::ignoreUnused (document, audioSource);
    DBG ("VoxScriptDocumentController::didAddAudioSourceToDocument called");
    
    // NOTE: Don't extract audio here - source not fully initialized yet!
    // Audio source may not have sample access enabled at this point.
    // Extraction will happen in doCreateAudioModification() instead.
}
```

**Change 2:** Find `doCreateAudioModification()` (around line 84) and replace with:

```cpp
juce::ARAAudioModification* VoxScriptDocumentController::doCreateAudioModification (
    juce::ARAAudioSource* audioSource,
    ARA::ARAAudioModificationHostRef hostRef,
    const juce::ARAAudioModification* optionalModificationToClone) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Creating Audio Modification");
    DBG ("Source: " + juce::String::toHexString ((juce::pointer_sized_int) audioSource));
    DBG ("Clone: " + juce::String (optionalModificationToClone ? "YES" : "NO"));
    DBG ("================================================");
    
    // Phase III: Extract audio and trigger transcription
    // Audio source is fully initialized at this point
    transcriptionStatus = "Extracting audio...";
    currentTempFile = audioExtractor.extractToTempWAV(audioSource);
    
    if (!currentTempFile.existsAsFile())
    {
        transcriptionStatus = "Failed: Could not extract audio";
        DBG("AudioExtractor failed to create temp file");
    }
    else
    {
        DBG("Audio extracted to: " + currentTempFile.getFullPathName());
        
        // Trigger transcription
        transcriptionStatus = "Starting transcription...";
        whisperEngine.transcribeAudioFile(currentTempFile);
    }
    
    return new juce::ARAAudioModification (audioSource, hostRef, optionalModificationToClone);
}
```

---

### Summary of Changes

1. **Removed** extraction code from `didAddAudioSourceToDocument()`
2. **Added** extraction code to `doCreateAudioModification()`
3. **Reason:** AudioModification is created after AudioSource is fully initialized

---

### After Making Changes

```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja -v
```

Then test in Reaper - should not crash!
