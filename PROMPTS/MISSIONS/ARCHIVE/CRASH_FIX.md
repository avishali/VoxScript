================================================================================
CRASH FIX - AudioExtractor Safety Improvements
================================================================================

**Issue:** Reaper crashes when adding audio file to track with VoxScript
**Root Cause:** AudioExtractor attempts to access audio samples before they're ready
**Solution:** Add validation and safety checks

================================================================================
DIAGNOSIS
================================================================================

**Likely Crash Location:** AudioExtractor.cpp line 127
```cpp
ARA::PlugIn::HostAudioReader reader(araAudioSource);
```

**Possible Causes:**
1. Audio source not fully initialized when extraction is triggered
2. Sample access enabled but host hasn't provided samples yet
3. HostAudioReader constructor fails (no error checking)
4. Audio properties return invalid values (zero samples, zero rate)

**Evidence:**
- Crash happens when audio file added to track
- Extraction is triggered in didAddAudioSourceToDocument()
- This callback might fire before audio source is fully ready

================================================================================
PROPOSED FIX
================================================================================

### Option A: Defer Extraction Until Ready (RECOMMENDED)

Move extraction from `didAddAudioSourceToDocument()` to `doCreateAudioModification()`.

**Rationale:**
- AudioModification is created AFTER AudioSource is fully initialized
- Host has loaded audio data by this point
- Sample access is guaranteed to be available

**Change in VoxScriptDocumentController.cpp:**

```cpp
// REMOVE extraction from didAddAudioSourceToDocument():
void VoxScriptDocumentController::didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource)
{
    juce::ignoreUnused (document);
    DBG ("VoxScriptDocumentController::didAddAudioSourceToDocument called");
    
    // DON'T extract here - audio source not ready yet!
    // Just register listener
    whisperEngine.addListener (this);
}

// ADD extraction to doCreateAudioModification():
juce::ARAAudioModification* VoxScriptDocumentController::doCreateAudioModification (
    juce::ARAAudioSource* audioSource,
    ARA::ARAAudioModificationHostRef hostRef,
    const juce::ARAAudioModification* optionalModificationToClone) noexcept
{
    DBG ("VoxScriptDocumentController::doCreateAudioModification called");
    
    // Extract audio NOW - source is fully initialized
    transcriptionStatus = "Extracting audio...";
    currentTempFile = audioExtractor.extractToTempWAV (audioSource);
    
    if (!currentTempFile.existsAsFile())
    {
        transcriptionStatus = "Failed: Could not extract audio";
        DBG ("AudioExtractor failed to create temp file");
    }
    else
    {
        DBG ("Audio extracted to: " + currentTempFile.getFullPathName());
        
        // Trigger transcription
        transcriptionStatus = "Starting transcription...";
        whisperEngine.transcribeAudioFile (currentTempFile);
    }
    
    return new juce::ARAAudioModification (audioSource, hostRef, optionalModificationToClone);
}
```

### Option B: Add Safety Checks to AudioExtractor

Add validation before attempting to read samples.

**Change in AudioExtractor.cpp before line 127:**

```cpp
// Validate audio source is ready for sample access
if (!araAudioSource->isSampleAccessEnabled())
{
    DBG("AudioExtractor: Sample access not enabled");
    return juce::File();
}

// Check if audio source has persistent ID (fully initialized)
if (araAudioSource->getPersistentID().empty())
{
    DBG("AudioExtractor: Audio source not fully initialized (no persistent ID)");
    return juce::File();
}

// THEN create reader:
ARA::PlugIn::HostAudioReader reader(araAudioSource);
```

### Option C: Try-Catch Around HostAudioReader

Wrap risky code in exception handler (not ideal, but safer).

```cpp
try
{
    ARA::PlugIn::HostAudioReader reader(araAudioSource);
    
    // ... rest of reading code ...
}
catch (const std::exception& e)
{
    DBG("AudioExtractor: Exception creating audio reader: " + juce::String(e.what()));
    tempFile.deleteFile();
    return juce::File();
}
catch (...)
{
    DBG("AudioExtractor: Unknown exception creating audio reader");
    tempFile.deleteFile();
    return juce::File();
}
```

================================================================================
RECOMMENDATION
================================================================================

**Implement Option A (defer extraction) + Option B (safety checks)**

1. Move extraction from didAddAudioSourceToDocument() to doCreateAudioModification()
2. Add isSampleAccessEnabled() check in AudioExtractor
3. Add getPersistentID() validation in AudioExtractor

**Why both?**
- Option A fixes the timing issue (extraction too early)
- Option B adds defense-in-depth (won't crash even if called early)

**Implementation order:**
1. Option A first (quick, should fix crash immediately)
2. Test in Reaper
3. If still crashes, add Option B
4. If STILL crashes, add Option C

================================================================================
QUICK FIX TO TRY NOW
================================================================================

**File:** Source/ara/VoxScriptDocumentController.cpp

**Change 1 - Remove extraction from didAddAudioSourceToDocument():**

Find this method (around line 45-65) and CHANGE TO:

```cpp
void VoxScriptDocumentController::didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource)
{
    juce::ignoreUnused (document, audioSource);
    DBG ("VoxScriptDocumentController::didAddAudioSourceToDocument called");
    
    // Register as listener for transcription events
    whisperEngine.addListener (this);
    
    // NOTE: Don't extract audio here - source not fully initialized yet
    // Extraction will happen in doCreateAudioModification() instead
}
```

**Change 2 - Add extraction to doCreateAudioModification():**

Find this method (around line 80-90) and CHANGE TO:

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

**Then rebuild and test:**
```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja -v
```

================================================================================
EXPECTED OUTCOME
================================================================================

After fix:
✅ No crash when adding audio file
✅ Extraction happens at correct time (when modification created)
✅ Transcription proceeds normally

If still crashes:
→ Check console output for DBG messages
→ Add Option B safety checks
→ Report back with error messages

================================================================================
END OF CRASH FIX
================================================================================
