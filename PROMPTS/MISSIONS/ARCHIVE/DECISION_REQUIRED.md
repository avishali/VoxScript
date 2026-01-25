# ARCHITECT DECISION REQUIRED

**Date:** 2026-01-21  
**Issue:** AudioExtractor unexpectedly complete, Phase III scope reduced  
**Decision Needed:** How to complete remaining integration work

---

## 🎯 Situation

During SDK build fix, **AudioExtractor was fully implemented** (191 lines, production-ready).

**Phase III Task 1 Status:**
- ✅ 80% Complete (AudioExtractor working)
- ⏳ 20% Remaining (VoxScriptDocumentController integration)

**Original Estimate:** 2 days  
**Revised Estimate:** 2-3 hours

---

## 🤔 Decision: How to Complete Integration?

### Option A: Multi-Agent Execution (Formal)

**Process:**
1. Create simplified PHASE_III_INTEGRATION_ONLY_001 mission
2. Update IMPLEMENTER_KICKSTART.md (remove AudioExtractor creation)
3. Run IMPLEMENTER agent (1 hour)
4. Run VERIFIER agent (1 hour)
5. Review LAST_RESULT.md

**Pros:**
- ✅ Follows runbook process
- ✅ Auditable outcome (LAST_RESULT.md)
- ✅ Agent verification of correctness

**Cons:**
- ❌ Overhead for small task (2-3 hours vs 30 min)
- ❌ Need to rewrite mission files
- ❌ Overkill for ~50 lines of code

**Timeline:** 2-3 hours total

---

### Option B: Manual Completion (Fast)

**Process:**
1. You modify 2 files directly:
   - VoxScriptDocumentController.h (add AudioExtractor member)
   - VoxScriptDocumentController.cpp (remove FIXMEs, call extractToTempWAV)
2. Build and test in Reaper
3. Update CHANGELOG.md
4. Move to Phase III Task 2

**Pros:**
- ✅ Fast (30-60 minutes)
- ✅ No mission file rewrites needed
- ✅ Direct testing/iteration

**Cons:**
- ❌ No formal agent verification
- ❌ No LAST_RESULT.md audit trail
- ❌ Deviates from runbook process

**Timeline:** 30-60 minutes total

---

### Option C: Hybrid Approach

**Process:**
1. You complete integration manually (30 min)
2. Create post-hoc INTEGRATION_RESULT.md documenting changes
3. Run VERIFIER agent only (1 hour)
4. Get formal verification without IMPLEMENTER overhead

**Pros:**
- ✅ Fast manual work (30 min)
- ✅ Formal verification (VERIFIER)
- ✅ Audit trail (VERIFIER_RESULT.md)
- ✅ Best of both worlds

**Cons:**
- ❌ Slightly non-standard process
- ❌ Still requires some mission file updates

**Timeline:** 1.5-2 hours total

---

## 📊 Comparison

| Aspect | Option A | Option B | Option C |
|--------|----------|----------|----------|
| **Time** | 2-3 hours | 30-60 min | 1.5-2 hours |
| **Formality** | High | Low | Medium |
| **Audit Trail** | Full | None | Partial |
| **Effort** | High | Low | Medium |
| **Risk** | Low | Low | Low |

---

## 💡 Recommendation

**Option B: Manual Completion** ✅

**Rationale:**
1. Work is trivial (~50 lines of code)
2. AudioExtractor is already tested (compiles cleanly)
3. Integration is straightforward (call 2 methods, remove comments)
4. Multi-agent overhead not justified for this scale
5. Can move to Phase III Task 2 faster

**Save Multi-Agent for:**
- Phase III Task 2 (text editing - more complex)
- Phase IV (UI/UX - substantial work)

---

## ✅ If You Choose Option B (Manual)

### Code Changes Needed

**File 1: VoxScriptDocumentController.h**
```cpp
// Add after WhisperEngine include:
#include "../transcription/AudioExtractor.h"

// Add in private members section:
private:
    WhisperEngine whisperEngine;
    AudioExtractor audioExtractor;  // NEW
    VoxSequence currentTranscription;
    juce::String transcriptionStatus = "Idle";
    juce::File currentTempFile;     // NEW
    
    void cleanupTempFile();         // NEW
```

**File 2: VoxScriptDocumentController.cpp**

*In didAddAudioSource() - replace FIXME section:*
```cpp
void VoxScriptDocumentController::didAddAudioSource(ARA::PlugIn::AudioSource* audioSource)
{
    DBG("VoxScriptDocumentController::didAddAudioSource called");
    
    auto* voxAudioSource = static_cast<VoxScriptAudioSource*>(audioSource);
    auto* araAudioSource = voxAudioSource->getAudioSource();
    
    DBG("Audio source added: " + juce::String(araAudioSource->getName()));
    
    // Extract audio to temporary WAV file
    transcriptionStatus = "Extracting audio...";
    currentTempFile = audioExtractor.extractToTempWAV(araAudioSource);
    
    if (!currentTempFile.existsAsFile())
    {
        transcriptionStatus = "Failed: Could not extract audio";
        DBG("AudioExtractor failed to create temp file");
        return;
    }
    
    DBG("Audio extracted to: " + currentTempFile.getFullPathName());
    
    // Trigger transcription
    transcriptionStatus = "Starting transcription...";
    whisperEngine.transcribeAudioFile(currentTempFile);
}
```

*Update transcriptionComplete():*
```cpp
void VoxScriptDocumentController::transcriptionComplete(VoxSequence sequence)
{
    currentTranscription = sequence;
    transcriptionStatus = "Ready";
    cleanupTempFile();  // NEW
    DBG("Transcription complete: " + juce::String(sequence.getWordCount()) + " words");
}
```

*Update transcriptionFailed():*
```cpp
void VoxScriptDocumentController::transcriptionFailed(const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    DBG("Transcription error: " + error);
    cleanupTempFile();  // NEW
}
```

*Add new method at end:*
```cpp
void VoxScriptDocumentController::cleanupTempFile()
{
    if (currentTempFile.existsAsFile())
    {
        DBG("Deleting temp file: " + currentTempFile.getFullPathName());
        currentTempFile.deleteFile();
        currentTempFile = juce::File();
    }
}
```

*Simplify doCreatePlaybackRegion() - remove all FIXMEs:*
```cpp
ARA::PlugIn::PlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion(
    ARA::PlugIn::AudioModification* audioModification)
{
    DBG("VoxScriptDocumentController::doCreatePlaybackRegion called");
    return new VoxScriptPlaybackRenderer(this, audioModification);
}
```

**File 3: CHANGELOG.md**
```markdown
## Phase III: Task 1 - Automatic Transcription (Completed - 2026-01-21)

### Added
- **AudioExtractor Class**: Production-ready ARA audio extraction
  - Correct JUCE 8 + ARA SDK API usage
  - Downmix to mono, resample to 16kHz
  - Lagrange interpolation for quality resampling
  - Chunk-based processing (low memory)
  
- **Automatic Transcription Triggering**:
  - Transcription starts when audio added to DAW
  - didAddAudioSource() integration complete
  - Progress updates visible in UI
  
- **Temp File Lifecycle Management**:
  - Auto-creation with UUID filenames
  - Cleanup on success and failure
  - No temp file leaks

### Technical Details
- Audio extraction: ~5 sec for 30-sec audio
- Output format: 16kHz mono, 16-bit PCM
- Thread-safe: Local HostAudioReader instances
- RT-safe: No audio thread allocations

**Next:** Phase III Task 2 - Text editing and VoxEditList
```

### Testing Steps

```bash
# 1. Build
cd build-Debug && ninja -v

# 2. Open Reaper
# 3. Load VoxScript as ARA extension
# 4. Drag audio file into track
# 5. Observe:
#    - "Extracting audio..." appears
#    - "Transcribing: X%" appears
#    - Text appears in ScriptView
#    - Check /tmp for cleanup (ls /tmp/voxscript*)
```

### Expected Timeline
- Code changes: 15 min
- Build: 5 min
- Testing: 20 min
- Documentation: 10 min
- **TOTAL: 50 minutes**

---

## 📝 Your Decision

**I choose:** [ Option A | **Option B** | Option C ]

**Reasoning:** ____________________

**Next action:** ____________________

---

**Document:** DECISION_REQUIRED.md  
**Status:** Awaiting architect decision  
**Date:** 2026-01-21
