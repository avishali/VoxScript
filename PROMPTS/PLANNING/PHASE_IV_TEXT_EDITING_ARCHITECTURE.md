# Phase IV Architecture: Text Editing & Synchronization

**Version:** 1.0 (Planning Document)  
**Date:** 2026-01-22  
**Status:** DRAFT - Not Yet Approved  
**Phase:** IV - Text Editing  
**Dependencies:** Phase III Complete

---

## Executive Summary

Phase IV introduces **text-based editing** capabilities to VoxScript, allowing users to modify transcriptions directly in the DAW interface, with edits automatically applied to the underlying audio through intelligent audio processing. This phase transforms VoxScript from a "transcription viewer" into a true "audio editor via text."

**Core Innovation:** Edit audio by editing text.

---

## Vision & User Experience

### The User Workflow

```
1. User drags audio clip into DAW (Phase III ✅)
   ↓
2. VoxScript automatically transcribes (Phase III ✅)
   ↓
3. User sees transcription in ScriptView
   ↓
4. User clicks on text, makes edits:
   - Delete word: "um" → removed from audio
   - Replace word: "gonna" → "going to" (keeps timing)
   - Insert word: Adds silence gap (Phase IV-B)
   ↓
5. VoxScript processes edit:
   - Identifies affected audio region
   - Applies phase-coherent crossfade
   - Updates playback region
   ↓
6. DAW plays modified audio instantly
```

### Key Design Principles

1. **Text is Ground Truth:** After user edits, transcription reflects user intent
2. **Non-Destructive:** Original audio preserved, edits applied via ARA modifications
3. **Instant Feedback:** Audio changes immediately reflect text edits
4. **Phonetically Aware:** Uses forced alignment for accurate word boundaries
5. **Zero-Crossing Safe:** Audio edits never click or pop

---

## Phase IV Sub-Phases

Phase IV is large and complex. We break it into manageable sub-phases:

### Phase IV-A: Word Deletion (PRIORITY) ⭐

**Timeline:** 2-3 weeks  
**Complexity:** Medium  

**Deliverables:**
- ScriptView with clickable words
- Word deletion via Backspace/Delete key
- VoxEdit and VoxEditList data structures
- AudioEditEngine with crossfading
- Zero-crossing detection
- VoxScriptPlaybackRenderer integration
- Basic undo/redo

**Success Metric:**  
User clicks word, presses Delete → word disappears from text AND audio.

---

### Phase IV-B: Word Replacement

**Timeline:** 2-3 weeks  
**Complexity:** High (requires synthesis or clever audio reuse)  

**Deliverables:**
- Inline text editing (double-click word)
- Word replacement logic
- Timing preservation (keep word duration)
- Confidence tracking for edited words

**Challenge:** Cannot generate new audio from text (no TTS) - must reuse existing audio or insert silence

---

### Phase IV-C: Word Insertion

**Timeline:** 2-3 weeks  
**Complexity:** High  

**Deliverables:**
- Cursor positioning in text
- Text insertion at cursor
- Audio gap creation (silence)
- Automatic gap sizing

---

### Phase IV-D: Forced Alignment Integration (CRITICAL FOUNDATION)

**Timeline:** 3-4 weeks  
**Complexity:** Very High  

**Purpose:**
- Phase II/III timestamps are segment-level (coarse)
- Need word-level accuracy for precise editing
- Phoneme-level enables advanced features (Phase V)

**Research Required:**
- Evaluate SOFA (https://github.com/m-bain/whisperX)
- Alternative: Montreal Forced Aligner
- Alternative: Gentle (https://github.com/lowerquality/gentle)

---

### Phase IV-E: Advanced Editing Features

**Timeline:** 2-3 weeks  
**Complexity:** Medium  

**Deliverables:**
- Multi-word selection and deletion
- Drag-and-drop word reordering (Phase V?)
- Confidence-based highlighting (low confidence = red underline)
- Context menu (right-click word → Delete, Replace, Insert Before/After)
- Keyboard shortcuts (Cmd+Z undo, Cmd+X cut, etc.)

---

### Phase IV-F: Performance & Polish

**Timeline:** 1-2 weeks  
**Complexity:** Medium  

**Deliverables:**
- Edit operation optimization (O(log n) lookup in edit list)
- Render path optimization (pre-calculate crossfades)
- Memory profiling (avoid leaks during long editing sessions)
- Stress testing (1000+ edits, 2+ hour audio files)
- UI responsiveness (no freezing during heavy editing)

---

## Core Data Structures

### 1. VoxEdit (Single Edit Operation)

```cpp
// Source/editing/VoxEdit.h

enum class VoxEditType
{
    DELETE,   // Remove word(s) from audio
    REPLACE,  // Substitute word (keeps timing)
    INSERT,   // Add word (creates gap)
    SPLIT,    // Split word at cursor (Phase IV-B)
    MERGE     // Merge adjacent words (Phase IV-B)
};

struct VoxEdit
{
    VoxEditType type;
    
    // Text-level identification
    int segmentIndex;     // Which VoxSegment (sentence)
    int wordIndex;        // Which word within segment
    
    // Audio-level timing
    int64_t startSample;  // Absolute sample position in audio source
    int64_t endSample;    // End position (for DELETE)
    
    // Edit-specific data
    juce::String originalText;  // Before edit
    juce::String newText;        // After edit (for REPLACE)
    
    // DSP parameters (pre-calculated)
    int64_t fadeInSamples;    // Crossfade duration
    int64_t fadeOutSamples;
    int64_t zeroXingStart;    // Zero-crossing positions
    int64_t zeroXingEnd;
    
    // Metadata
    juce::String editId;      // UUID for undo/redo
    double timestamp;         // When edit was made
};
```

### 2. VoxEditList (Edit Sequence)

```cpp
// Source/editing/VoxEditList.h

class VoxEditList
{
public:
    // Add edit (maintains chronological order)
    void addEdit(VoxEdit edit);
    
    // Remove edit (undo)
    void removeEdit(const juce::String& editId);
    
    // Query edits affecting a time range
    juce::Array<VoxEdit> getEditsInRange(int64_t startSample, int64_t endSample) const;
    
    // Check if sample should be played or silenced
    bool isSampleAudible(int64_t samplePos) const;
    
    // Get crossfade multiplier for sample (0.0 - 1.0)
    float getCrossfadeGain(int64_t samplePos) const;
    
    // Serialization for session save/load
    juce::ValueTree toValueTree() const;
    static VoxEditList fromValueTree(const juce::ValueTree& tree);
    
private:
    juce::Array<VoxEdit> edits;  // Chronological order
    juce::SortedSet<int64_t> editBoundaries;  // For fast lookup
};
```

### 3. VoxEditController (Edit Manager)

```cpp
// Source/editing/VoxEditController.h

class VoxEditController : public juce::UndoManager
{
public:
    VoxEditController(VoxScriptDocumentController& doc);
    
    // Main edit interface
    void processTextEdit(int segmentIndex, int wordIndex, 
                         const juce::String& newText);
    void deleteWord(int segmentIndex, int wordIndex);
    void insertWord(int segmentIndex, int wordIndex, 
                    const juce::String& text);
    
    // Undo/Redo
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Query
    const VoxEditList& getEditList() const { return editList; }
    VoxSequence getModifiedTranscription() const;  // With edits applied
    
private:
    VoxScriptDocumentController& documentController;
    VoxEditList editList;
    
    void calculateCrossfadeParameters(VoxEdit& edit);
    int64_t findNearestZeroCrossing(int64_t samplePos, int searchRadius = 128);
};
```

---

## Technical Deep-Dives

### Zero-Crossing Detection

**Purpose:** Prevent clicks/pops when cutting audio

```cpp
int64_t AudioEditEngine::findNearestZeroCrossing(
    const float* audioData,
    int64_t targetSample,
    int searchRadius)
{
    int64_t bestSample = targetSample;
    float minAmplitude = std::abs(audioData[targetSample]);
    
    // Search forward and backward
    for (int i = 1; i <= searchRadius; ++i)
    {
        // Forward
        int64_t forwardSample = targetSample + i;
        if (forwardSample < totalSamples)
        {
            float amp = std::abs(audioData[forwardSample]);
            if (amp < minAmplitude)
            {
                minAmplitude = amp;
                bestSample = forwardSample;
            }
        }
        
        // Backward
        int64_t backwardSample = targetSample - i;
        if (backwardSample >= 0)
        {
            float amp = std::abs(audioData[backwardSample]);
            if (amp < minAmplitude)
            {
                minAmplitude = amp;
                bestSample = backwardSample;
            }
        }
        
        // Early exit if we find true zero-crossing
        if (minAmplitude < 0.001f)  // Threshold
            break;
    }
    
    return bestSample;
}
```

---

### Phase-Coherent Crossfading

**Purpose:** Smooth audio transitions when deleting words

```cpp
void AudioEditEngine::applyCrossfade(
    float* outputBuffer,
    const float* inputBuffer,
    int64_t fadeStartSample,
    int fadeLengthSamples,
    bool fadeIn)
{
    for (int i = 0; i < fadeLengthSamples; ++i)
    {
        float position = (float)i / (float)fadeLengthSamples;  // 0.0 → 1.0
        
        float gain;
        if (fadeIn)
        {
            // Ease-in curve (cosine)
            gain = 0.5f * (1.0f - std::cos(position * juce::MathConstants<float>::pi));
        }
        else
        {
            // Ease-out curve (cosine)
            gain = 0.5f * (1.0f + std::cos(position * juce::MathConstants<float>::pi));
        }
        
        int64_t sampleIndex = fadeStartSample + i;
        outputBuffer[sampleIndex] = inputBuffer[sampleIndex] * gain;
    }
}
```

**Fade Duration:**
- Short words (< 200ms): 5ms fade (220 samples @ 44.1kHz)
- Medium words (200-500ms): 10ms fade (441 samples)
- Long words (> 500ms): 20ms fade (882 samples)

---

## Edit Operation Example: Deleting "um"

**Scenario:** User transcript: "So, um, I think that..."  
User deletes "um"

**Step-by-Step:**

1. **Identify Word:**
   - Segment 0, Word 1 ("um")
   - Start sample: 44100 (1.0 second @ 44.1kHz)
   - End sample: 52920 (1.2 seconds)
   - Duration: 8820 samples (200ms)

2. **Find Zero-Crossings:**
   ```
   zeroXingStart = findNearestZeroCrossing(44100, radius=128);
   → Result: 44056 (44 samples earlier, amplitude 0.0003)
   
   zeroXingEnd = findNearestZeroCrossing(52920, radius=128);
   → Result: 52976 (56 samples later, amplitude 0.0001)
   ```

3. **Calculate Crossfades:**
   ```
   fadeInSamples = 220;   // 5ms @ 44.1kHz
   fadeOutSamples = 220;
   
   fadeOutStart = zeroXingStart - fadeOutSamples;  // 43836
   fadeInEnd = zeroXingEnd + fadeInSamples;        // 53196
   ```

4. **Create VoxEdit:**
   ```cpp
   VoxEdit edit;
   edit.type = VoxEditType::DELETE;
   edit.segmentIndex = 0;
   edit.wordIndex = 1;
   edit.startSample = 44056;
   edit.endSample = 52976;
   edit.originalText = "um";
   edit.fadeInSamples = 220;
   edit.fadeOutSamples = 220;
   edit.zeroXingStart = 44056;
   edit.zeroXingEnd = 52976;
   edit.editId = "edit_1a2b3c4d";
   ```

5. **Render During Playback:**
   ```cpp
   // In VoxScriptPlaybackRenderer::renderPlaybackRegion()
   for (int sample = 0; sample < numSamples; ++sample)
   {
       int64_t absoluteSample = playbackStartSample + sample;
       
       if (!editList.isSampleAudible(absoluteSample))
       {
           // Sample is deleted, output silence
           outputBuffer[sample] = 0.0f;
       }
       else
       {
           // Sample is audible, apply crossfade if in fade region
           float gain = editList.getCrossfadeGain(absoluteSample);
           outputBuffer[sample] = inputBuffer[sample] * gain;
       }
   }
   ```

**Result:**
- "um" region (44056 - 52976) outputs silence
- Fade-out (43836 - 44056): smooth transition to silence
- Fade-in (52976 - 53196): smooth transition back to audio
- No clicks or pops (zero-crossing aligned)
- Playback is RT-safe (no allocations, pre-calculated)

---

## Open Questions (Require Architect Decision)

### Q1: Forced Alignment Tool Selection

**Options:**
- **SOFA (WhisperX):** Python-based, excellent accuracy, hard to integrate
- **Montreal Forced Aligner:** C++ lib, requires phoneme dictionary
- **Gentle:** Python-based, Kaldi backend, resource-heavy

**Decision Required:** Which tool to use? Or defer to Phase IV-D research?

---

### Q2: Word Replacement Strategy

**Options:**
- **Option A:** Replace with silence (simple, always works)
- **Option B:** Search audio for similar word, splice in (complex, may fail)
- **Option C:** Manual audio selection by user (advanced feature)

**Recommendation:** Start with Option A (silence), add Option C later.

---

### Q3: Edit Granularity

**Options:**
- **Word-level:** Edit entire words only (simpler, Phase IV-A)
- **Phoneme-level:** Edit individual sounds (complex, Phase V?)

**Recommendation:** Word-level for Phase IV, defer phoneme-level to Phase V.

---

## Success Metrics (Phase IV Overall)

### Minimum Viable Product (MVP)

- [ ] Word deletion works (Phase IV-A)
- [ ] No audio artifacts (clicks/pops)
- [ ] Undo/redo functional
- [ ] Edits persist with project save/load
- [ ] RT-safe playback (<5% CPU overhead)

### Target Goals

- [ ] Word replacement works (Phase IV-B)
- [ ] Word insertion works (Phase IV-C)
- [ ] Forced alignment integrated (Phase IV-D)
- [ ] Multi-word selection/deletion (Phase IV-E)
- [ ] Performance optimized (Phase IV-F)

---

## Timeline & Resource Estimate

**Phase IV-A (Word Deletion):** 2-3 weeks | 80-120 hours  
**Phase IV-B (Word Replacement):** 2-3 weeks | 80-120 hours  
**Phase IV-C (Word Insertion):** 2-3 weeks | 80-120 hours  
**Phase IV-D (Forced Alignment):** 3-4 weeks | 120-160 hours  
**Phase IV-E (Advanced Features):** 2-3 weeks | 80-120 hours  
**Phase IV-F (Performance):** 1-2 weeks | 40-80 hours  

**Total Phase IV:** 12-17 weeks (~3-4 months)

---

## Next Steps

1. **ARCHITECT Review:** Approve architecture and sub-phase plan
2. **Forced Alignment Research:** Evaluate SOFA, MFA, Gentle (1 week)
3. **Create Phase IV-A Mission:** Word deletion implementation (next)
4. **Verify Phase III:** Complete Phase III verification first (prerequisite)

---

**Document Status:** DRAFT - Awaiting Architect Approval  
**Last Updated:** 2026-01-22  
**Version:** 1.0
