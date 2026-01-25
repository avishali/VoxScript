# Phase III Task 2 - Text Editing Implementation (PRELIMINARY PLAN)

**Date:** 2026-01-21  
**Status:** Planning Phase  
**Prerequisites:** ✅ Phase III Task 1 Complete

---

## 🎯 Objective

Enable text-based editing of audio by detecting transcription changes and applying corresponding audio modifications.

**User Story:** _"As a user, I can edit the transcription text in ScriptView, and when I press play, the audio reflects my text edits (deletions, insertions, substitutions)."_

---

## 📊 Scope & Complexity

### Complexity Assessment

**Phase III Task 2 is 10x more complex than Task 1:**

| Aspect | Task 1 | Task 2 |
|--------|--------|--------|
| Lines of Code | ~300 | ~1500-2000 |
| Estimated Time | 16h (1h actual) | 40-60 hours |
| Components | 2 new classes | 5 new classes |
| DSP Required | None | Phase-coherent crossfading |
| State Management | Simple | Complex (original vs edited) |
| Real-time Processing | Background only | Audio thread modifications |

### Recommended Approach

**Option A: Incremental Delivery** ⭐ RECOMMENDED
- Split into 3 sub-tasks (A, B, C)
- Deliver working functionality at each stage
- Lower risk, faster feedback loop

**Option B: Complete Implementation**
- Single large mission
- All functionality delivered together
- Higher risk, longer timeline

**Option C: Defer to Phase IV**
- Current automatic transcription is valuable standalone
- Gather user feedback first
- Plan editing based on real needs

---

## 📋 Option A: Incremental Delivery (RECOMMENDED)

### **Sub-Task 2A: Editable Text UI** (8-12 hours)

**Goal:** Make ScriptView editable, store modified text

**Deliverables:**
1. Replace simple text rendering with juce::TextEditor
2. Load transcription text into editor on completion
3. Detect when user modifies text
4. Store modified text in DocumentController
5. Visual indicator showing edited vs original state

**Files to Modify:**
- Source/ui/ScriptView.h (.cpp) - Add TextEditor component
- Source/ara/VoxScriptDocumentController.h (.cpp) - Store edited text

**Acceptance Criteria:**
- [x] User can click and edit transcription text
- [x] Text changes are detected
- [x] Edited text stored in DocumentController
- [x] Visual indicator shows "modified" state
- [x] No audio changes yet (audio plays original)

**Risk:** LOW - Pure UI work, no audio processing

---

### **Sub-Task 2B: VoxEditList & Edit Detection** (16-20 hours)

**Goal:** Detect text changes and generate edit operations

**Deliverables:**
1. **VoxEdit Struct:**
   ```cpp
   struct VoxEdit {
       enum Type { Insert, Delete, Substitute };
       Type type;
       int originalWordIndex;
       juce::String newText;
       double audioStartTime;
       double audioEndTime;
   };
   ```

2. **VoxEditList Class:**
   ```cpp
   class VoxEditList {
       void addEdit(VoxEdit edit);
       const juce::Array<VoxEdit>& getEdits();
       void clear();
       void serialize/deserialize for project save;
   };
   ```

3. **Edit Detection Algorithm:**
   - Diff original transcription vs edited text
   - Map text changes to word indices
   - Look up audio timing from VoxSequence
   - Generate VoxEdit operations

4. **Integration:**
   - DocumentController stores VoxEditList
   - Generate edits when user commits changes
   - Serialize/deserialize for project save/load

**Files to Create:**
- Source/editing/VoxEdit.h
- Source/editing/VoxEditList.h (.cpp)
- Source/editing/EditDetector.h (.cpp)

**Files to Modify:**
- Source/ara/VoxScriptDocumentController.h (.cpp)
- Source/ui/ScriptView.cpp (trigger edit detection)

**Acceptance Criteria:**
- [x] Text diff algorithm works correctly
- [x] Edits mapped to audio time ranges
- [x] VoxEditList stored in DocumentController
- [x] Edit list survives project save/load
- [x] No audio changes yet (audio still plays original)

**Risk:** MEDIUM - Algorithm complexity, edge cases

---

### **Sub-Task 2C: Audio Edit Application** (20-28 hours)

**Goal:** Apply text edits to audio playback

**Deliverables:**
1. **VoxScriptPlaybackRenderer Updates:**
   - Read VoxEditList from DocumentController
   - Skip deleted audio regions during playback
   - Crossfade at edit boundaries
   - Handle insertions (silence or room tone)

2. **Phase-Coherent Crossfading:**
   - Zero-crossing detection
   - Smooth amplitude envelope (Hann window)
   - Prevent clicks/pops at edit points

3. **Real-Time Processing:**
   - Apply edits in processBlock()
   - Maintain RT-safety (no allocations)
   - Efficient edit lookup (binary search)

4. **Room Tone Synthesis (Optional):**
   - Analyze background noise
   - Generate matching room tone for insertions
   - Or use silence if no room tone available

**Files to Modify:**
- Source/ara/VoxScriptPlaybackRenderer.h (.cpp)

**Files to Create (Optional):**
- Source/dsp/Crossfader.h (.cpp)
- Source/dsp/RoomToneAnalyzer.h (.cpp)

**Acceptance Criteria:**
- [x] Deleted text regions don't play audio
- [x] Edit boundaries are smooth (no clicks)
- [x] Insertions use silence or room tone
- [x] Substitutions crossfade between regions
- [x] RT-safe implementation verified
- [x] Edit list updates reflected immediately in playback

**Risk:** HIGH - Real-time DSP, RT-safety, crossfading complexity

---

## 🗓️ Timeline Estimates

### Incremental Approach (Option A)

```
Sub-Task 2A: Editable UI
├── Planning & Design ............. 2 hours
├── Implementation ................ 6 hours
├── Testing ....................... 2 hours
└── Documentation ................. 2 hours
    TOTAL: 12 hours (1.5 days)

Sub-Task 2B: Edit Detection
├── Planning & Design ............. 4 hours
├── VoxEdit/VoxEditList ........... 8 hours
├── EditDetector algorithm ........ 6 hours
├── Integration & Testing ......... 4 hours
└── Documentation ................. 2 hours
    TOTAL: 24 hours (3 days)

Sub-Task 2C: Audio Application
├── Planning & Design ............. 4 hours
├── PlaybackRenderer updates ...... 12 hours
├── Crossfading DSP ............... 8 hours
├── RT-safety verification ........ 4 hours
├── Testing & Polish .............. 6 hours
└── Documentation ................. 2 hours
    TOTAL: 36 hours (4.5 days)

GRAND TOTAL: 72 hours (~9 days)
```

### Single Mission Approach (Option B)

```
Phase III Task 2: Complete
├── Architecture & Planning ....... 8 hours
├── Implementation ................ 48 hours
├── Integration ................... 8 hours
├── Testing ....................... 12 hours
└── Documentation ................. 4 hours
    TOTAL: 80 hours (~10 days)
```

---

## 🔑 Key Technical Challenges

### 1. Text Diff Algorithm

**Challenge:** Detect insertions, deletions, substitutions in edited text

**Approaches:**
- **Myers Diff Algorithm** (industry standard, used by git)
- **Longest Common Subsequence (LCS)**
- **Edit Distance (Levenshtein)**

**Recommendation:** Use Myers Diff - well-tested, efficient

### 2. Audio Timing Alignment

**Challenge:** Map text edits to precise audio time ranges

**Approach:**
```
Original: "The quick brown fox"
Edited:   "The brown fox"
         
Delete: word[1] = "quick"
Audio time: VoxSequence.words[1].startTime → words[1].endTime
Result: Skip audio from 0.5s to 0.8s
```

### 3. Phase-Coherent Crossfading

**Challenge:** Smooth transitions without clicks/pops

**Requirements:**
- Find zero-crossings near edit boundaries
- Apply Hann window for amplitude envelope
- Maintain phase continuity if possible

**DSP Code:**
```cpp
// Simplified crossfade
for (int i = 0; i < fadeLength; ++i)
{
    float fadeOut = std::cos(i * M_PI / (2.0f * fadeLength));
    float fadeIn = std::sin(i * M_PI / (2.0f * fadeLength));
    
    output[i] = audioA[i] * fadeOut + audioB[i] * fadeIn;
}
```

### 4. Real-Time Edit Application

**Challenge:** Process edits in audio thread without allocations

**Approach:**
- Pre-compute edit boundaries on message thread
- Store in lock-free structure (juce::AudioBuffer or array)
- Audio thread reads (never writes) edit data
- Binary search for current playback position
- Apply crossfade if near edit boundary

---

## 📦 Data Structures

### VoxEdit

```cpp
struct VoxEdit
{
    enum Type { Delete, Insert, Substitute, Move };
    
    Type type;
    int originalWordIndex;       // -1 for insertions
    juce::String newText;        // Empty for deletions
    double audioStartTime;       // Seconds
    double audioEndTime;         // Seconds
    int fadeLengthSamples;       // Crossfade length
    
    bool operator< (const VoxEdit& other) const {
        return audioStartTime < other.audioStartTime;
    }
};
```

### VoxEditList

```cpp
class VoxEditList
{
public:
    void addEdit(const VoxEdit& edit);
    void removeEdit(int index);
    void clear();
    
    const juce::Array<VoxEdit>& getEdits() const;
    
    // Find edits affecting time range
    juce::Array<VoxEdit> getEditsInRange(double startTime, double endTime) const;
    
    // Serialization
    void writeToStream(juce::OutputStream& stream) const;
    bool readFromStream(juce::InputStream& stream);
    
private:
    juce::Array<VoxEdit> edits;  // Kept sorted by audioStartTime
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxEditList)
};
```

---

## 🎓 Recommended Execution Strategy

### For Sub-Task 2A (Editable UI)

**Complexity:** LOW  
**Approach:** Manual implementation (3-4 hours work)  
**Reason:** Simple UI changes, not worth multi-agent overhead

### For Sub-Task 2B (Edit Detection)

**Complexity:** MEDIUM  
**Approach:** Multi-agent execution (IMPLEMENTER + VERIFIER)  
**Reason:** Algorithm complexity justifies formal verification

### For Sub-Task 2C (Audio Application)

**Complexity:** HIGH  
**Approach:** Multi-agent execution with DSP expert consultation  
**Reason:** RT-safety critical, DSP complexity high

---

## 🚀 Next Steps

### Immediate (Today)

1. ✅ **Celebrate Task 1 completion!**
2. 📝 **Review this preliminary plan**
3. 🤔 **Decide on approach:**
   - Incremental (2A → 2B → 2C)?
   - Single mission?
   - Defer to Phase IV?

### If Proceeding with 2A (Editable UI)

1. Manual implementation (simple UI work)
2. Estimated time: 3-4 hours
3. Deliverable: Editable ScriptView with change detection

### If Proceeding with Full Task 2

1. Create comprehensive mission file (PHASE_III_TASK_2_MISSION.txt)
2. Split into 3 sub-missions (2A, 2B, 2C)
3. Execute each with multi-agent process
4. Estimated total: 9-10 days

---

## ❓ Questions for Architect

1. **Urgency:** Do you need text editing now, or can it wait?
2. **Approach:** Incremental (A→B→C) or single mission?
3. **Scope:** Full editing or start with deletions only?
4. **Timeline:** 9 days acceptable, or need faster delivery?

---

## 💡 My Recommendation

**Start with Sub-Task 2A (Editable UI) manually:**

**Why:**
- Low risk, high value (users can edit text immediately)
- Quick win (3-4 hours)
- Provides foundation for 2B and 2C
- Lets you test user interaction before committing to full editing

**Then:**
- Gather feedback on editable UI
- Plan 2B based on real usage patterns
- Decide if full editing is needed or if simpler approach works

**This de-risks the investment** - you'll know if text editing is valuable before building complex audio application logic.

---

**Status:** Planning complete, awaiting architect decision  
**Date:** 2026-01-21
