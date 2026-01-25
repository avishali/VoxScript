# Phase III Mission - Planning Document

**Date:** 2026-01-21  
**Mission ID:** PHASE_III_ARA_AUDIO_EXTRACTION_001 (DRAFT)  
**Status:** Planning Stage  
**Phase:** III - Automatic Transcription Triggering

---

## 🎯 Mission Objective

Enable **automatic transcription** when audio is added to the DAW by extracting audio data from ARA's persistent sample access API and triggering WhisperEngine.

**Result:** When user drags audio into Reaper → VoxScript automatically transcribes it → text appears in UI.

---

## 📊 Current State vs Target State

### Current State (Phase II Complete ✅)

**What Works:**
- ✅ WhisperEngine can transcribe audio files
- ✅ VoxSequence stores transcription results
- ✅ ScriptView displays transcription text
- ✅ Thread-safe architecture in place
- ✅ Status updates work ("Transcribing: X%")

**What Doesn't Work:**
- ❌ No automatic transcription triggering
- ❌ User must manually call transcribeAudioFile()
- ❌ No way to extract audio from ARA objects

**FIXME Locations:**
- `VoxScriptDocumentController.cpp` lines 74-90
- `VoxScriptDocumentController.cpp` lines 119-138

### Target State (Phase III Goal 🎯)

**What Will Work:**
1. ✅ Audio added to DAW → **automatic transcription starts**
2. ✅ ARA audio data extracted to memory buffer
3. ✅ Buffer written to temporary WAV file
4. ✅ WhisperEngine.transcribeAudioFile(tempWAV) called
5. ✅ Progress updates visible in UI (0% → 100%)
6. ✅ Text appears in ScriptView automatically
7. ✅ Temp files cleaned up after transcription
8. ✅ Multiple audio files work sequentially

---

## 🔍 Technical Research Needed

### Question 1: How to Access ARA Audio Data?

**Options to Investigate:**

**A. ARA Persistent Sample Access API**
```cpp
// Potential approach (needs verification):
auto* audioSource = modification->getAudioSource();
auto sampleCount = audioSource->getSampleCount();
auto sampleRate = audioSource->getSampleRate();
auto channelCount = audioSource->getChannelCount();

// Access samples via ARA API
auto reader = audioSource->createReader(); // ???
reader->readSamples(buffer, startSample, numSamples);
```

**B. JUCE ARA Wrappers**
```cpp
// JUCE might provide helpers:
auto* juceAudioSource = dynamic_cast<juce::ARAAudioSource*>(araAudioSource);
auto reader = juceAudioSource->createReaderFor(...); // ???
```

**C. Host Callback Method**
```cpp
// ARA might require host callback:
araAudioSource->readAudioSamples(
    destinationBuffer,
    channelIndex,
    samplePosition,
    numSamples
);
```

**D. File Path Access (if available)**
```cpp
// Best case: ARA provides original file path
auto filePath = araAudioSource->getAudioFileInfo().file;
if (filePath.existsAsFile()) {
    whisperEngine.transcribeAudioFile(filePath);
}
```

**ACTION REQUIRED:** Research ARA SDK documentation to determine correct API.

### Question 2: When to Trigger Transcription?

**Option A: doCreateAudioSource()**
- ✅ Called when audio first added
- ✅ Earliest possible trigger point
- ❌ Sample data might not be available yet

**Option B: doCreateAudioModification()**
- ✅ Called after audio source established
- ✅ Sample data likely available
- ✅ Current FIXME location
- ⚠️ Might be called multiple times

**Option C: doCreatePlaybackRegion()**
- ✅ Called when region added to timeline
- ✅ Sample data definitely available
- ✅ Current FIXME location
- ⚠️ Could be called many times (duplicates)

**Option D: notifyPropertiesUpdated() in VoxScriptAudioSource**
- ✅ Called when source properties finalized
- ✅ Sample access should be ready
- ✅ Called once per source
- ✅ **RECOMMENDED**

**DECISION:** Use **Option D** - trigger in VoxScriptAudioSource::notifyPropertiesUpdated()

### Question 3: Memory Buffer vs Temp File?

**Option A: Memory Buffer Only**
```cpp
// Extract to memory
juce::AudioBuffer<float> buffer;
extractAudioToBuffer(araSource, buffer);

// Modify WhisperEngine to accept buffer
whisperEngine.transcribeAudioBuffer(buffer, sampleRate);
```
- ✅ No disk I/O
- ✅ Faster
- ❌ Requires WhisperEngine refactor
- ❌ Large files use lots of memory

**Option B: Temporary WAV File**
```cpp
// Extract to temp file
auto tempFile = juce::File::getSpecialLocation(
    juce::File::tempDirectory).getChildFile("voxscript_temp.wav");

extractAudioToFile(araSource, tempFile);
whisperEngine.transcribeAudioFile(tempFile);

// Clean up after transcription
tempFile.deleteFile();
```
- ✅ Works with existing WhisperEngine
- ✅ Handles large files
- ✅ Minimal code changes
- ⚠️ Disk I/O overhead
- ✅ **RECOMMENDED FOR PHASE III**

**DECISION:** Use **Option B** - temp WAV files (simpler, works with Phase II code)

---

## 📐 Proposed Architecture

### Components to Create

**1. AudioExtractor Class (NEW)**
```cpp
// Source/transcription/AudioExtractor.h/cpp

class AudioExtractor
{
public:
    /**
     * Extract audio from ARA audio source to temporary WAV file
     * Returns the temporary file, or invalid File if extraction failed
     */
    static juce::File extractToTempWAV(
        juce::ARAAudioSource* araSource,
        juce::String tempFilePrefix = "voxscript_"
    );
    
    /**
     * Extract audio from ARA audio source to memory buffer
     * (Phase IV optimization)
     */
    static bool extractToBuffer(
        juce::ARAAudioSource* araSource,
        juce::AudioBuffer<float>& buffer,
        double& sampleRate
    );
    
private:
    static juce::File getTempDirectory();
    static juce::String generateUniqueTempFileName();
};
```

**2. Update VoxScriptAudioSource (MODIFY)**
```cpp
// Source/ara/VoxScriptAudioSource.h

class VoxScriptAudioSource : public juce::ARAAudioSource
{
public:
    // ... existing methods ...
    
    void notifyPropertiesUpdated() noexcept;
    
private:
    void triggerTranscription(); // NEW - called from notifyPropertiesUpdated()
    
    // Phase III: Store transcription state
    VoxSequence currentTranscription;
    juce::String transcriptionStatus;
    juce::File tempAudioFile; // Store for cleanup
};
```

**3. Update VoxScriptDocumentController (MODIFY)**
```cpp
// Source/ara/VoxScriptDocumentController.h

class VoxScriptDocumentController : public juce::ARADocumentControllerSpecialisation
{
    // ... existing methods ...
    
    // NEW: Get audio source by ID for progress updates
    VoxScriptAudioSource* getAudioSourceByID(const juce::String& sourceID);
    
    // NEW: Notify UI of transcription progress
    void notifyTranscriptionProgress(VoxScriptAudioSource* source, float progress);
};
```

---

## 🔄 Execution Flow

### Scenario: User Drags Audio into Reaper

**Step 1: ARA Creates Audio Source**
```
Reaper → ARA API → VoxScriptDocumentController::doCreateAudioSource()
  → new VoxScriptAudioSource()
  → Audio source registered with ARA
```

**Step 2: ARA Notifies Properties Updated**
```
ARA API → VoxScriptAudioSource::notifyPropertiesUpdated()
  → Sample rate, channels, duration now available
  → Sample data is accessible
  → TRIGGER TRANSCRIPTION HERE ←
```

**Step 3: Extract Audio to Temp WAV**
```
VoxScriptAudioSource::triggerTranscription()
  → AudioExtractor::extractToTempWAV(this)
    → Read samples from ARA persistent sample access
    → Create temp WAV file: /tmp/voxscript_abc123.wav
    → Write samples to file
    → Return temp file path
```

**Step 4: Trigger WhisperEngine**
```
VoxScriptAudioSource::triggerTranscription() continued
  → whisperEngine.transcribeAudioFile(tempFile)
  → WhisperEngine starts background thread
  → Status updates sent to listeners
```

**Step 5: Progress Updates**
```
WhisperEngine (background thread)
  → notifyProgress(0.25) → VoxScriptAudioSource::transcriptionProgress()
    → Update UI status: "Transcribing: 25%"
  → notifyProgress(0.50) → "Transcribing: 50%"
  → notifyProgress(0.75) → "Transcribing: 75%"
```

**Step 6: Transcription Complete**
```
WhisperEngine (background thread)
  → notifyComplete(voxSequence) → VoxScriptAudioSource::transcriptionComplete()
    → Store voxSequence in audio source
    → Update UI: "Ready"
    → Delete temp WAV file
    → Notify ScriptView to display text
```

**Step 7: UI Updates**
```
ScriptView (message thread)
  → Receives notification
  → Calls audioSource->getTranscription()
  → Displays text: "Hello, this is a test recording..."
```

---

## 📝 Detailed Implementation Plan

### Task 1: Research ARA Sample Access API (4 hours)

**Deliverables:**
- [ ] Determine correct API for accessing audio samples
- [ ] Create proof-of-concept: extract 1 second of audio to buffer
- [ ] Document API usage patterns
- [ ] Identify any limitations or gotchas

**Resources:**
- ARA SDK documentation: `/Users/avishaylidani/DEV/SDK/ARA_SDK/SDK/Docs/`
- JUCE ARA examples: `/Users/avishaylidani/DEV/SDK/JUCE/examples/Plugins/ARAPluginDemo`
- ARA forum: https://www.aravst.org/

**Acceptance Criteria:**
- Can read audio samples from ARAAudioSource
- Can determine sample rate, channel count, duration
- Can access arbitrary sample ranges

### Task 2: Implement AudioExtractor Class (6 hours)

**Deliverables:**
- [ ] `Source/transcription/AudioExtractor.h` (100 lines)
- [ ] `Source/transcription/AudioExtractor.cpp` (200 lines)
- [ ] Unit tests (optional for Phase III)

**Implementation Details:**
```cpp
// AudioExtractor.cpp structure

juce::File AudioExtractor::extractToTempWAV(juce::ARAAudioSource* araSource, juce::String prefix)
{
    // 1. Get audio properties
    auto sampleRate = araSource->getSampleRate();
    auto channelCount = araSource->getChannelCount();
    auto totalSamples = araSource->getSampleCount();
    
    // 2. Create temp file
    auto tempFile = getTempDirectory().getChildFile(
        prefix + juce::Uuid().toString() + ".wav"
    );
    
    // 3. Create WAV writer
    juce::WavAudioFormat wavFormat;
    auto* writer = wavFormat.createWriterFor(
        new juce::FileOutputStream(tempFile),
        sampleRate,
        channelCount,
        16, // 16-bit PCM
        {},
        0
    );
    
    if (writer == nullptr)
        return juce::File(); // Failure
    
    // 4. Read samples from ARA in chunks
    const int chunkSize = 8192;
    juce::AudioBuffer<float> buffer(channelCount, chunkSize);
    
    for (int64 pos = 0; pos < totalSamples; pos += chunkSize)
    {
        int samplesToRead = juce::jmin((int64)chunkSize, totalSamples - pos);
        
        // Extract from ARA (API TBD from Task 1)
        bool success = readSamplesFromARA(araSource, buffer, pos, samplesToRead);
        
        if (!success)
        {
            delete writer;
            tempFile.deleteFile();
            return juce::File();
        }
        
        // Write to WAV
        writer->writeFromAudioSampleBuffer(buffer, 0, samplesToRead);
    }
    
    delete writer;
    return tempFile;
}
```

**Acceptance Criteria:**
- Can extract full audio file to temp WAV
- WAV file is valid (loadable in Audacity)
- Handles multi-channel audio correctly
- Cleans up on failure

### Task 3: Update VoxScriptAudioSource (4 hours)

**Deliverables:**
- [ ] Modify `VoxScriptAudioSource.h` (add transcription methods)
- [ ] Modify `VoxScriptAudioSource.cpp` (implement triggerTranscription)
- [ ] Implement WhisperEngine::Listener callbacks

**Key Changes:**
```cpp
// VoxScriptAudioSource.cpp

void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    DBG("VoxScriptAudioSource: Properties updated, triggering transcription");
    triggerTranscription();
}

void VoxScriptAudioSource::triggerTranscription()
{
    // Extract audio to temp file
    tempAudioFile = AudioExtractor::extractToTempWAV(this, "voxscript_");
    
    if (!tempAudioFile.existsAsFile())
    {
        DBG("VoxScriptAudioSource: Audio extraction failed");
        transcriptionStatus = "Extraction failed";
        return;
    }
    
    // Get WhisperEngine from DocumentController
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(
        getDocumentController()
    );
    
    if (docController == nullptr)
    {
        DBG("VoxScriptAudioSource: Cannot get DocumentController");
        return;
    }
    
    // Start transcription
    transcriptionStatus = "Starting transcription...";
    docController->whisperEngine.transcribeAudioFile(tempAudioFile);
}

// WhisperEngine::Listener implementation
void VoxScriptAudioSource::transcriptionProgress(float progress)
{
    transcriptionProgress = progress;
    transcriptionStatus = "Transcribing: " + juce::String(int(progress * 100)) + "%";
    
    // TODO: Notify UI to repaint
}

void VoxScriptAudioSource::transcriptionComplete(VoxSequence sequence)
{
    currentTranscription = sequence;
    transcriptionReady = true;
    transcriptionStatus = "Ready";
    
    // Clean up temp file
    if (tempAudioFile.existsAsFile())
        tempAudioFile.deleteFile();
    
    // TODO: Notify UI to display transcription
}

void VoxScriptAudioSource::transcriptionFailed(const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    
    // Clean up temp file
    if (tempAudioFile.existsAsFile())
        tempAudioFile.deleteFile();
}
```

**Acceptance Criteria:**
- triggerTranscription() called when audio added
- Temp file created and cleaned up
- WhisperEngine receives valid audio file
- Progress updates work
- Error handling robust

### Task 4: Update WhisperEngine for Multiple Sources (2 hours)

**Issue:** Current WhisperEngine can only transcribe one file at a time.

**Solution:** Add source tracking

**Changes:**
```cpp
// WhisperEngine.h

class WhisperEngine : public juce::Thread
{
public:
    // NEW: Associate transcription with source ID
    void transcribeAudioFile(
        const juce::File& audioFile,
        const juce::String& sourceID = ""
    );
    
private:
    juce::String currentSourceID;
    
    // Callbacks now include source ID
    void notifyProgress(const juce::String& sourceID, float progress);
    void notifyComplete(const juce::String& sourceID, VoxSequence sequence);
    void notifyFailed(const juce::String& sourceID, const juce::String& error);
};

// Listener interface update
class Listener
{
public:
    virtual void transcriptionProgress(const juce::String& sourceID, float progress) = 0;
    virtual void transcriptionComplete(const juce::String& sourceID, VoxSequence sequence) = 0;
    virtual void transcriptionFailed(const juce::String& sourceID, const juce::String& error) = 0;
};
```

**Acceptance Criteria:**
- Multiple audio sources can queue transcriptions
- Callbacks identify which source completed
- No race conditions

### Task 5: Update UI to Display Transcriptions (2 hours)

**Changes:**
```cpp
// PluginEditor.cpp

void VoxScriptAudioProcessorEditor::updateTranscriptionDisplay()
{
    // Get current audio source from DocumentController
    auto* docController = getDocumentController();
    auto* audioSource = docController->getCurrentAudioSource();
    
    if (audioSource == nullptr)
    {
        scriptView.setStatus("No audio source");
        scriptView.setTranscription(VoxSequence());
        return;
    }
    
    // Update status
    scriptView.setStatus(audioSource->getTranscriptionStatus());
    
    // Update transcription if ready
    if (audioSource->isTranscriptionReady())
    {
        scriptView.setTranscription(audioSource->getTranscription());
    }
}
```

**Acceptance Criteria:**
- UI updates when transcription progresses
- Text appears when transcription completes
- Status reflects current state
- No UI freezing

### Task 6: Testing & Verification (4 hours)

**Test Cases:**
1. **Single Audio File**
   - Drag audio into Reaper
   - Verify transcription starts automatically
   - Verify progress updates (0% → 100%)
   - Verify text appears

2. **Multiple Audio Files (Sequential)**
   - Drag 3 audio files into Reaper
   - Verify all transcribe correctly
   - Verify no crashes or memory leaks

3. **Large Audio File (60 minutes)**
   - Test with long podcast/lecture
   - Verify doesn't crash
   - Verify progress updates smooth

4. **Error Cases**
   - Audio file with no speech
   - Corrupted audio file
   - Missing model file
   - Disk full (temp file creation fails)

5. **Cleanup**
   - Verify temp files deleted after success
   - Verify temp files deleted after failure
   - Verify no temp file accumulation

**Acceptance Criteria:**
- All test cases pass
- No crashes
- No memory leaks
- Temp files cleaned up

---

## 📋 Mission Scope

### Files to Create (2 new)
- ✓ `Source/transcription/AudioExtractor.h` (NEW - ~100 lines)
- ✓ `Source/transcription/AudioExtractor.cpp` (NEW - ~200 lines)

### Files to Modify (4 existing)
- ✓ `Source/ara/VoxScriptAudioSource.h` (add transcription methods)
- ✓ `Source/ara/VoxScriptAudioSource.cpp` (implement triggerTranscription)
- ✓ `Source/transcription/WhisperEngine.h` (add source ID tracking)
- ✓ `Source/transcription/WhisperEngine.cpp` (update callbacks)

### Files Forbidden to Touch
- ✗ `Source/PluginProcessor.h/cpp` (ARA factory stable)
- ✗ `Source/PluginEditor.h/cpp` (UI container stable)
- ✗ `Source/ara/VoxScriptDocumentController.h/cpp` (minimize changes)
- ✗ `Source/ui/ScriptView.h/cpp` (already has display methods)
- ✗ `Source/ui/DetailView.h/cpp` (Phase IV)
- ✗ `CMakeLists.txt` (no new dependencies)

### Estimated Effort
- **Research:** 4 hours (ARA sample access API)
- **Implementation:** 14 hours (6 tasks)
- **Testing:** 4 hours
- **Total:** ~22 hours (~3 days)

---

## ⚠️ Risks & Unknowns

### Risk 1: ARA Sample Access API Unknown
**Impact:** HIGH - Blocks entire implementation  
**Mitigation:** Start with Task 1 research immediately  
**Fallback:** If API too complex, use file path access (if available)

### Risk 2: Performance with Large Files
**Impact:** MEDIUM - User experience degraded  
**Mitigation:** Extract audio in chunks, show progress  
**Fallback:** Warn user for files >30 minutes

### Risk 3: Temp File Disk Space
**Impact:** LOW - Extraction fails on full disk  
**Mitigation:** Check available space before extraction  
**Fallback:** Show error message to user

### Risk 4: Multiple Concurrent Transcriptions
**Impact:** MEDIUM - Could overwhelm system  
**Mitigation:** Queue transcriptions, process one at a time  
**Phase IV:** Add transcription queue management

---

## ✅ Success Criteria (Draft)

**Mission succeeds if:**
1. ✅ Audio dragged into Reaper → transcription starts automatically
2. ✅ Progress updates visible in UI (0% → 100%)
3. ✅ Text appears in ScriptView when complete
4. ✅ Multiple audio files work sequentially
5. ✅ Temp files cleaned up properly
6. ✅ No crashes or memory leaks
7. ✅ Error handling works (missing model, extraction failure)
8. ✅ All 3 SKIP criteria from Phase II now PASS:
   - [AC-22] Status appears during transcription: PASS ✓
   - [AC-23] Text appears after completion: PASS ✓
   - [AC-25] Multiple files work: PASS ✓

---

## 📊 Acceptance Criteria (Draft - ~25 criteria)

Will be finalized after Task 1 research completes.

**Categories:**
- ARA Integration (5 criteria)
- Audio Extraction (5 criteria)
- Transcription Triggering (4 criteria)
- Progress Updates (3 criteria)
- UI Display (3 criteria)
- Cleanup & Error Handling (5 criteria)

---

## 🚀 Next Steps

### Option A: Start Research Immediately
Execute Task 1 to understand ARA sample access API, then finalize mission spec.

### Option B: Review Planning Document
You review this plan, provide feedback, then we proceed to research.

### Option C: Create Formal Mission File
Convert this planning doc to `CURRENT_MISSION.txt` format for multi-agent execution.

**Which would you prefer?** 🤔
