# IMPLEMENTER KICKSTART PROMPT
# Phase III - Automatic Transcription Triggering
# Mission: PHASE_III_ARA_AUDIO_EXTRACTION_001

================================================================================
ROLE: IMPLEMENTER
================================================================================

You are the IMPLEMENTER agent executing Phase III of the VoxScript project.

Your mission is defined in: `PROMPTS/MISSIONS/CURRENT_MISSION.txt`

**YOU MUST READ CURRENT_MISSION.txt COMPLETELY BEFORE STARTING.**

================================================================================
EXECUTIVE SUMMARY
================================================================================

**Objective**: Enable automatic transcription when audio is added to the DAW.

**What You're Building**:
1. Copy AudioExtractor class files from PROMPTS/MISSIONS/ to Source/transcription/
2. Integrate AudioExtractor into VoxScriptDocumentController
3. Remove FIXME comments from Phase II (lines 74-90, 119-138)
4. Implement automatic transcription triggering in didAddAudioSource()
5. Add temp file cleanup lifecycle management
6. Update ScriptView for automatic status updates
7. Update CHANGELOG.md

**Scope**: 6 files total (2 new, 4 modified)

**Forbidden**: DO NOT touch WhisperEngine, VoxSequence, or any of the 11 forbidden files listed in CURRENT_MISSION.txt

**Expected Time**: 1-2 hours

**When Done**: Write IMPLEMENTER_RESULT.md and STOP. Do NOT build, test, or verify.

================================================================================
PRE-FLIGHT CHECKLIST
================================================================================

Before you start, verify:

[ ] I have read PROMPTS/MISSIONS/CURRENT_MISSION.txt completely
[ ] I understand the scope: 6 files only
[ ] I understand forbidden files: WhisperEngine/VoxSequence are OFF LIMITS
[ ] I know where AudioExtractor reference files are: PROMPTS/MISSIONS/
[ ] I understand the STOP rule: Write result file, then STOP immediately

If any checkbox is unchecked, STOP and read CURRENT_MISSION.txt again.

================================================================================
IMPLEMENTATION SEQUENCE
================================================================================

Follow these steps IN ORDER:

## Step 1: Copy AudioExtractor Files

**Action**: Copy the reference implementation files to the correct location.

```bash
# Source files (reference implementation)
PROMPTS/MISSIONS/AudioExtractor.h
PROMPTS/MISSIONS/AudioExtractor.cpp

# Destination
Source/transcription/AudioExtractor.h
Source/transcription/AudioExtractor.cpp
```

**Requirements**:
- Copy files as-is (production-ready, validated by research)
- Add MelechDSP copyright header to both files (see existing files for format)
- Do NOT modify the implementation logic
- Keep all comments and documentation

**Verification**: Both files exist in Source/transcription/

---

## Step 2: Modify VoxScriptDocumentController.h

**File**: `Source/ara/VoxScriptDocumentController.h`

**Changes Required**:

1. Add include at top:
```cpp
#include "../transcription/AudioExtractor.h"
```

2. Add private members (around line 50-60, near WhisperEngine):
```cpp
private:
    // ... existing members ...
    WhisperEngine whisperEngine;
    AudioExtractor audioExtractor;  // NEW
    VoxSequence currentTranscription;
    juce::String transcriptionStatus = "Idle";
    juce::File currentTempFile;     // NEW: Track temp file for cleanup
```

3. Add private method declaration (near other private methods):
```cpp
private:
    // ... existing methods ...
    void cleanupTempFile();  // NEW
```

**Verification**: 
- AudioExtractor.h included
- audioExtractor member added
- currentTempFile member added
- cleanupTempFile() declared

---

## Step 3: Modify VoxScriptDocumentController.cpp

**File**: `Source/ara/VoxScriptDocumentController.cpp`

**This is the MAIN implementation work. Follow carefully.**

### 3A: Remove FIXME Comments (Lines 74-90)

**Current code** (approximately lines 74-90):
```cpp
void VoxScriptDocumentController::didAddAudioSource(ARA::PlugIn::AudioSource* audioSource)
{
    DBG("VoxScriptDocumentController::didAddAudioSource called");
    
    auto* voxAudioSource = static_cast<VoxScriptAudioSource*>(audioSource);
    DBG("Audio source added: " + juce::String(voxAudioSource->getAudioSource()->getName()));
    
    // FIXME Phase III: Extract audio from ARA persistent sample access
    // FIXME Phase III: Create temp WAV file: AudioExtractor.extractToTempWAV()
    // FIXME Phase III: Trigger transcription: whisperEngine.transcribeAudioFile(tempFile)
    // FIXME Phase III: Store temp file path for cleanup after transcription
}
```

**Replace with**:
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

### 3B: Remove FIXME Comments (Lines 119-138)

**Current code** (approximately lines 119-138):
```cpp
ARA::PlugIn::PlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion(
    ARA::PlugIn::AudioModification* audioModification)
{
    DBG("VoxScriptDocumentController::doCreatePlaybackRegion called");
    
    auto* playbackRegion = new VoxScriptPlaybackRenderer(this, audioModification);
    
    // FIXME Phase III: Consider triggering transcription here instead of didAddAudioSource()
    // FIXME Phase III: Might be more appropriate timing - region created when audio actually used
    // FIXME Phase III: Would need to check if transcription already done for this audio source
    
    return playbackRegion;
}
```

**Replace with** (simplified, FIXME comments removed):
```cpp
ARA::PlugIn::PlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion(
    ARA::PlugIn::AudioModification* audioModification)
{
    DBG("VoxScriptDocumentController::doCreatePlaybackRegion called");
    return new VoxScriptPlaybackRenderer(this, audioModification);
}
```

### 3C: Update transcriptionComplete() Callback

**Find** (approximately line 150-160):
```cpp
void VoxScriptDocumentController::transcriptionComplete(VoxSequence sequence)
{
    currentTranscription = sequence;
    transcriptionStatus = "Ready";
    // TODO Phase III: Notify UI to display new transcription
}
```

**Replace with**:
```cpp
void VoxScriptDocumentController::transcriptionComplete(VoxSequence sequence)
{
    currentTranscription = sequence;
    transcriptionStatus = "Ready";
    
    // Cleanup temp file after successful transcription
    cleanupTempFile();
    
    DBG("Transcription complete: " + juce::String(sequence.getWordCount()) + " words");
}
```

### 3D: Update transcriptionFailed() Callback

**Find** (approximately line 165-170):
```cpp
void VoxScriptDocumentController::transcriptionFailed(const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    DBG("Transcription error: " + error);
}
```

**Replace with**:
```cpp
void VoxScriptDocumentController::transcriptionFailed(const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    DBG("Transcription error: " + error);
    
    // Cleanup temp file after failed transcription
    cleanupTempFile();
}
```

### 3E: Implement cleanupTempFile() Method

**Add** at the end of the file (before the closing brace):
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

**Verification for Step 3**:
- [ ] FIXME comments removed from didAddAudioSource()
- [ ] FIXME comments removed from doCreatePlaybackRegion()
- [ ] didAddAudioSource() calls audioExtractor.extractToTempWAV()
- [ ] didAddAudioSource() calls whisperEngine.transcribeAudioFile()
- [ ] transcriptionComplete() calls cleanupTempFile()
- [ ] transcriptionFailed() calls cleanupTempFile()
- [ ] cleanupTempFile() method implemented

---

## Step 4: Modify ScriptView.cpp

**File**: `Source/ui/ScriptView.cpp`

### 4A: Update Constructor to Add Timer

**Find** the ScriptView constructor (approximately line 10-20):
```cpp
ScriptView::ScriptView()
{
    // ... existing initialization ...
}
```

**Add** at the end of constructor:
```cpp
    // Start timer for status polling (temporary Phase III solution)
    startTimer(100); // Poll every 100ms
```

### 4B: Implement timerCallback() Method

**Add** this new method (after existing methods):
```cpp
void ScriptView::timerCallback()
{
    // Poll document controller for status updates
    // Note: Proper observer pattern will be implemented in Phase IV
    
    if (auto* docController = getDocumentController())
    {
        auto newStatus = docController->getTranscriptionStatus();
        if (newStatus != statusText)
        {
            setStatus(newStatus);
        }
        
        // Check if transcription updated
        auto& transcription = docController->getTranscription();
        if (!transcription.getSegments().isEmpty() && displayText.isEmpty())
        {
            setTranscription(transcription);
        }
    }
}
```

### 4C: Add getDocumentController() Helper Method

**Add** this helper method:
```cpp
VoxScriptDocumentController* ScriptView::getDocumentController()
{
    // Navigate up component hierarchy to find plugin editor,
    // then get document controller from there
    // Implementation depends on your component hierarchy
    
    // Placeholder implementation:
    if (auto* editor = findParentComponentOfClass<PluginEditor>())
    {
        return editor->getDocumentController();
    }
    return nullptr;
}
```

**Note**: Adjust `getDocumentController()` based on actual component hierarchy. 
If you don't have direct access, you may need to store a pointer to the 
DocumentController in ScriptView's constructor. Use existing pattern from project.

### 4D: Update paint() Method

**Find** (approximately line 40-60):
```cpp
void ScriptView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
    
    // Draw status in top-left
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText(statusText, 10, 10, 200, 20, juce::Justification::left);
    
    // Draw transcription text
    if (displayText.isNotEmpty())
    {
        g.setColour(juce::Colours::black);
        g.setFont(14.0f);
        auto textArea = getLocalBounds().reduced(20).withTrimmedTop(40);
        g.drawMultiLineText(displayText, textArea.getX(), textArea.getY(), 
                             textArea.getWidth());
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.drawText("No transcription available", 
                    getLocalBounds(), juce::Justification::centred);
    }
}
```

**Replace** the else clause:
```cpp
    else
    {
        g.setColour(juce::Colours::lightgrey);
        g.setFont(14.0f);
        g.drawText("Ready - Awaiting audio source", 
                   getLocalBounds(), juce::Justification::centred);
    }
```

**Verification for Step 4**:
- [ ] Timer started in constructor (100ms)
- [ ] timerCallback() implemented
- [ ] getDocumentController() helper added
- [ ] paint() updated with better placeholder text

---

## Step 5: Update ScriptView.h (If Needed)

**File**: `Source/ui/ScriptView.h`

**Only if timerCallback() is not already declared**, add:

```cpp
class ScriptView : public juce::Component,
                   private juce::Timer  // Add this if not present
{
public:
    // ... existing methods ...
    
private:
    void timerCallback() override;  // Add this if not present
    VoxScriptDocumentController* getDocumentController();  // Add helper
    
    // ... existing members ...
};
```

**Verification**:
- [ ] ScriptView inherits from juce::Timer
- [ ] timerCallback() declared as override
- [ ] getDocumentController() helper declared

---

## Step 6: Update CHANGELOG.md

**File**: `CHANGELOG.md`

**Action**: Add Phase III Task 1 completion entry.

**Location**: After Phase II entry, before any Phase III section.

**Content**: Copy this template and paste it:

```markdown
## Phase III: Task 1 - Automatic Transcription (Completed - 2026-01-21)

### Added
- **AudioExtractor Class**: Converts ARA audio to whisper-ready WAV files
  - Thread-safe extraction using juce::ARAAudioSourceReader
  - Automatic downmix to mono (average all channels)
  - Automatic resample to 16kHz (Lagrange interpolation)
  - Chunk-based processing (low memory footprint)
  - Output: 16-bit PCM WAV optimized for whisper.cpp
  
- **Automatic Transcription Triggering**:
  - Transcription starts automatically when audio added to DAW
  - No manual intervention required
  - didAddAudioSource() callback integration
  - Progress updates visible in UI
  
- **Temporary File Management**:
  - Automatic temp WAV file creation
  - Unique filenames (UUID-based, no collisions)
  - Cleanup after transcription success
  - Cleanup after transcription failure
  - No temp file leaks

### Modified
- **VoxScriptDocumentController**:
  - Added AudioExtractor member
  - Implemented automatic extraction + transcription in didAddAudioSource()
  - Added cleanupTempFile() for lifecycle management
  - Removed FIXME comments from Phase II (lines 74-90, 119-138)
  
- **ScriptView**:
  - Added timer-based status polling (temporary Phase III solution)
  - Improved paint() for better transcription display
  - Status updates trigger automatic repaints

### Technical Details
- Extraction performance: ~5 seconds for 30-second audio (varies by CPU)
- Downmix before resample: 50% CPU savings vs resample-then-downmix
- Thread safety: Local ARAAudioSourceReader prevents Steinberg glitches
- Temp file location: System temp directory (/tmp on macOS)
- No audio thread allocations (RT-safety preserved)

### Known Limitations (Phase III Task 1)
- Timer-based UI updates (proper observer pattern in Phase IV)
- Sequential processing only (no queue for multiple files yet)
- No extraction progress indicator (extraction is fast, not critical)
- No cancellation support (Phase IV feature)

### What Now Works End-to-End
✅ User drags audio into DAW → Automatic extraction → Automatic transcription → Text appears in UI
✅ No manual steps required
✅ Temp files cleaned up automatically
✅ Multiple files work sequentially
✅ Error handling graceful (no crashes)

**Next:** Phase III Task 2 - Text editing and VoxEditList
```

**Verification**:
- [ ] CHANGELOG.md updated with Phase III Task 1 entry
- [ ] Entry placed after Phase II, before other Phase III sections
- [ ] Date is 2026-01-21

---

## Step 7: Final Code Review

Before writing your result, verify:

### Files Created (2):
- [ ] Source/transcription/AudioExtractor.h exists
- [ ] Source/transcription/AudioExtractor.cpp exists
- [ ] Both have MelechDSP copyright headers

### Files Modified (4):
- [ ] Source/ara/VoxScriptDocumentController.h modified
- [ ] Source/ara/VoxScriptDocumentController.cpp modified
- [ ] Source/ui/ScriptView.cpp modified (or .h if needed)
- [ ] CHANGELOG.md modified

### Scope Compliance:
- [ ] ZERO changes to WhisperEngine.h/cpp
- [ ] ZERO changes to VoxSequence.h/cpp
- [ ] ZERO changes to forbidden files
- [ ] ONLY 6 files touched (2 new + 4 modified)

### FIXME Removal:
- [ ] Lines 74-90 in VoxScriptDocumentController.cpp: NO FIXME comments
- [ ] Lines 119-138 in VoxScriptDocumentController.cpp: NO FIXME comments

### Implementation Quality:
- [ ] No TODO comments added
- [ ] No speculative features beyond scope
- [ ] All code follows JUCE naming conventions
- [ ] DBG statements added for debugging
- [ ] Error handling present (check for existsAsFile())

---

## Step 8: Write IMPLEMENTER_RESULT.md

**Create**: `PROMPTS/MISSIONS/IMPLEMENTER_RESULT.md`

**Use this template**:

```markdown
# IMPLEMENTER RESULT - PHASE_III_ARA_AUDIO_EXTRACTION_001

## Agent Info
- Role: IMPLEMENTER
- Model: [Your AI model name/version]
- Execution time: [Start time] to [End time]
- Date: 2026-01-21

## Summary
Implemented automatic transcription triggering for VoxScript Phase III Task 1. 
Integrated AudioExtractor class to convert ARA audio samples to whisper-ready 
WAV files, removed FIXME comments from Phase II, and established automatic 
temp file lifecycle management. UI now updates via timer-based polling.

## Files Created (2)
1. Source/transcription/AudioExtractor.h
   - Copied from PROMPTS/MISSIONS/AudioExtractor.h
   - Added MelechDSP copyright header
   - Production-ready class for ARA audio extraction

2. Source/transcription/AudioExtractor.cpp
   - Copied from PROMPTS/MISSIONS/AudioExtractor.cpp
   - Added MelechDSP copyright header
   - Implementation: downmix → resample → write to temp WAV

## Files Modified (4)
1. Source/ara/VoxScriptDocumentController.h
   - Added: #include "../transcription/AudioExtractor.h"
   - Added: AudioExtractor audioExtractor member
   - Added: juce::File currentTempFile member
   - Added: void cleanupTempFile() method declaration

2. Source/ara/VoxScriptDocumentController.cpp
   - Modified didAddAudioSource() (lines ~74-90):
     * Removed 4 FIXME comments
     * Implemented audioExtractor.extractToTempWAV()
     * Implemented whisperEngine.transcribeAudioFile()
     * Added error handling for extraction failure
   - Modified doCreatePlaybackRegion() (lines ~119-138):
     * Removed 3 FIXME comments
     * Simplified to single-line return statement
   - Modified transcriptionComplete():
     * Added cleanupTempFile() call
   - Modified transcriptionFailed():
     * Added cleanupTempFile() call
   - Implemented cleanupTempFile() method:
     * Checks file exists
     * Deletes temp file
     * Resets currentTempFile to empty

3. Source/ui/ScriptView.cpp
   - Modified constructor:
     * Added startTimer(100) for status polling
   - Added timerCallback() method:
     * Polls DocumentController for status updates
     * Polls DocumentController for transcription updates
     * Triggers UI repaints when data changes
   - Added getDocumentController() helper:
     * [Describe implementation based on your hierarchy]
   - Modified paint():
     * Changed placeholder text to "Ready - Awaiting audio source"

4. CHANGELOG.md
   - Added Phase III Task 1 completion entry
   - Documented AudioExtractor class
   - Documented automatic transcription triggering
   - Documented temp file management
   - Listed technical details and limitations

## FIXME Comments Removed
✅ VoxScriptDocumentController.cpp lines 74-90: All FIXME comments removed
✅ VoxScriptDocumentController.cpp lines 119-138: All FIXME comments removed

Total FIXME comments removed: 7

## Issues Encountered
[List any problems, ambiguities, or implementation challenges]
[Examples:]
- [If applicable] getDocumentController() implementation needed adjustment for component hierarchy
- [If applicable] Timer interval set to 100ms - may need tuning in Phase IV

[If none, write "None"]

## Deviations from Spec
[List any intentional deviations from CURRENT_MISSION.txt]
[Examples:]
- [If applicable] Added extra DBG statement for debugging temp file path
- [If applicable] Adjusted timer interval from 100ms to 150ms for performance

[If none, write "None - followed specification exactly"]

## Implementation Notes
1. AudioExtractor files copied as-is from reference implementation
2. Used local ARAAudioSourceReader pattern from Gemini research
3. Temp file cleanup called in both success and failure paths
4. Timer-based UI polling is temporary Phase III solution (observer pattern in Phase IV)
5. Error handling: Returns early if extraction fails, logs to console

## Code Statistics
- Lines added: ~150
- Lines removed: ~15 (FIXME comments)
- Net change: +135 lines
- Files touched: 6 (2 new, 4 modified)
- Forbidden files touched: 0 ✅

## STOP Confirmation
✅ I have completed all code changes as specified in CURRENT_MISSION.txt
✅ I have verified scope compliance (6 files, 0 forbidden files)
✅ I have removed all 7 FIXME comments
✅ I have written this IMPLEMENTER_RESULT.md file
✅ I am now STOPPING immediately

I did NOT:
- Run build commands
- Test in DAW
- Verify acceptance criteria
- Fix any issues discovered
- Continue beyond writing this file

Passing control to VERIFIER role for build and runtime verification.

---
END OF IMPLEMENTER RESULT
```

**Verification**:
- [ ] IMPLEMENTER_RESULT.md created
- [ ] All sections filled out completely
- [ ] STOP confirmation present
- [ ] File saved in PROMPTS/MISSIONS/

---

## Step 9: STOP IMMEDIATELY

**YOU MUST STOP NOW.**

Do NOT:
❌ Run `ninja` or any build command
❌ Test in Reaper or any DAW
❌ Verify acceptance criteria
❌ Check if code compiles
❌ Fix any issues you noticed
❌ Add "one more thing"
❌ Continue for any reason

**Your job is complete. STOP.**

The VERIFIER agent will:
✅ Audit your changes
✅ Run the build
✅ Perform runtime tests
✅ Evaluate acceptance criteria
✅ Write VERIFIER_RESULT.md
✅ Write LAST_RESULT.md

**ARCHITECT will review LAST_RESULT.md and approve/reject the mission.**

================================================================================
TROUBLESHOOTING
================================================================================

If you encounter issues during implementation:

### Issue: Can't find reference files
**Solution**: Check PROMPTS/MISSIONS/ directory for AudioExtractor.h/.cpp

### Issue: Unclear where to add code
**Solution**: Search for existing similar patterns in the file, follow those

### Issue: Component hierarchy unclear for getDocumentController()
**Solution**: Look at existing component relationships, follow existing pattern

### Issue: Ambiguous specification
**Solution**: Document in "Issues Encountered" section, make best judgment, continue

### Issue: Scope violation temptation
**Solution**: STOP, document in result file, do NOT modify forbidden files

### Issue: Want to improve something beyond scope
**Solution**: Document in "Implementation Notes", do NOT add features

**REMEMBER**: Your job is to implement the spec, not improve it. Document concerns
in IMPLEMENTER_RESULT.md and let ARCHITECT decide.

================================================================================
QUALITY CHECKLIST
================================================================================

Before Step 8 (writing result), verify:

**Correctness**:
- [ ] All 6 files modified/created
- [ ] AudioExtractor copied, not rewritten
- [ ] FIXME comments removed
- [ ] Temp file cleanup on both success/failure paths
- [ ] Timer started for UI polling

**Code Quality**:
- [ ] JUCE naming conventions followed
- [ ] Copyright headers added to new files
- [ ] DBG statements for debugging
- [ ] Error handling present
- [ ] No memory leaks (RAII pattern used)

**Scope Compliance**:
- [ ] WhisperEngine untouched
- [ ] VoxSequence untouched
- [ ] No forbidden files modified
- [ ] No speculative features added
- [ ] No TODO comments added

**Documentation**:
- [ ] CHANGELOG.md updated completely
- [ ] Code comments explain non-obvious logic
- [ ] IMPLEMENTER_RESULT.md comprehensive

If all checkboxes are ticked: Proceed to Step 8.
If any checkbox fails: Fix before Step 8.

================================================================================
SUCCESS CRITERIA
================================================================================

Your implementation is successful if:

1. ✅ All 6 files modified/created correctly
2. ✅ 0 forbidden files touched
3. ✅ 7 FIXME comments removed
4. ✅ AudioExtractor integrated
5. ✅ Temp file lifecycle managed
6. ✅ CHANGELOG.md updated
7. ✅ IMPLEMENTER_RESULT.md written
8. ✅ STOPPED immediately after Step 9

You'll know you succeeded when VERIFIER reports:
- Build: SUCCESS
- Scope compliance: PASS
- Runtime tests: 12/12 PASS
- Acceptance criteria: 35/35 PASS

**But that's not your concern right now.**

Your ONLY job: Steps 1-9, then STOP.

================================================================================
FINAL REMINDER
================================================================================

**READ CURRENT_MISSION.txt COMPLETELY BEFORE STARTING.**

This kickstart prompt is a guide, but CURRENT_MISSION.txt is the authority.

When in doubt:
1. Check CURRENT_MISSION.txt
2. Document ambiguity in result
3. Make best judgment
4. Continue

**DO NOT:**
- Guess at specifications
- Skip reading CURRENT_MISSION.txt
- Modify forbidden files
- Continue after Step 9

**Good luck! Execute with precision. 🎯**

Ready to begin? Start with Step 1: Copy AudioExtractor files.
