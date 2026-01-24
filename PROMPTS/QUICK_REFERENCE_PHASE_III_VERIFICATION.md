# Quick Reference: VoxScript Phase III Verification

## TL;DR - What To Do Right Now

```bash
# Step 1: Run VERIFIER agent
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
# Give agent: "Execute PROMPTS/MISSIONS/VERIFIER_MISSION_PHASE_III.txt"

# Step 2: Review results
cat PROMPTS/MISSIONS/LAST_RESULT.md

# Step 3: Make decision
# Write PROMPTS/MISSIONS/ARCHITECT_DECISION.md
```

## Files Created For You

| File | Purpose | When to Use |
|------|---------|-------------|
| `VERIFIER_MISSION_PHASE_III.txt` | Complete verification spec | Give to VERIFIER agent |
| `RUNTIME_TEST_PROTOCOL_PHASE_III.md` | Manual testing guide | Test in Reaper yourself |
| `PHASE_IV_TEXT_EDITING_ARCHITECTURE.md` | Next phase planning | After Phase III approved |
| `PHASE_III_TO_IV_TRANSITION_SUMMARY.md` | Overview document | Read first (this context) |

## Critical Commands

```bash
# Build verification
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript/build-Debug
ninja -v 2>&1 | tee build.log

# Check for errors/warnings
grep -i "error\|warning" build.log

# Check model exists
ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin

# Monitor temp files during transcription
watch -n 1 'ls -lh /tmp/voxscript_*.wav'

# Check temp file format
afinfo /tmp/voxscript_*.wav
# Expected: 16000 Hz, 1 ch, Linear PCM

# Clean up orphaned temp files
rm /tmp/voxscript_*.wav
```

## Test Sequence (Manual)

```
1. Open Reaper
2. Drag audio file to track
3. Right-click waveform → Extensions → VoxScript
4. Watch console for:
   ✓ "Properties updated"
   ✓ "Starting extraction"
   ✓ "Extraction complete"
   ✓ "Transcription progress: 25%"
   ✓ "Transcription complete!"
   ✓ "Deleting temp file"
5. Verify no /tmp/voxscript_*.wav files remain
```

## Success Criteria

**Minimum to PASS (30/35):**
- Build: 5/5
- Code Quality: 10/10
- Thread Safety: 5/5
- Functionality: 8/10
- Scope: 5/5

**Acceptance Threshold:** ≥86% (30/35 PASS)

## Key Acceptance Criteria

**Most Critical (Must PASS):**
- [AC-2] Build completes successfully
- [AC-16] No audio thread allocations
- [AC-17] ARAAudioSourceReader on worker thread
- [AC-21] Plugin loads in Reaper
- [AC-22] Audio auto-triggers transcription
- [AC-26] Transcription completes successfully
- [AC-31] Only allowed files modified

**Important (Should PASS):**
- [AC-13] Downmix before resample
- [AC-14] Output format 16kHz mono 16-bit
- [AC-15] Temp files cleaned up
- [AC-27] Temp files deleted after completion

**Nice-to-Have (Can SKIP):**
- [AC-30] Large files don't crash (optional test)

## Decision Tree

```
Build Success?
├─ NO → FAIL (P0: Fix build issues first)
└─ YES
    │
    Core Functionality Works? (Tests 1-3)
    ├─ NO → FAIL (P0: Core features broken)
    └─ YES
        │
        Scope Compliant? (Only allowed files modified)
        ├─ NO → FAIL (P1: Architecture drift)
        └─ YES
            │
            Thread-Safe? (No RT allocations)
            ├─ NO → FAIL (P0: RT violations)
            └─ YES
                │
                ≥30/35 Criteria PASS?
                ├─ NO → FAIL (Insufficient quality)
                └─ YES → ACCEPT ✅
```

## After Verification

**If PASSED:**
```bash
# 1. Update status
vim PROJECT_STATUS.md
# Change Phase III status to: ✅ COMPLETE

# 2. Commit Phase III
git add .
git commit -m "Phase III: Audio extraction + auto-transcription COMPLETE"

# 3. Move to Phase IV planning
# Review: PROMPTS/PLANNING/PHASE_IV_TEXT_EDITING_ARCHITECTURE.md
```

**If FAILED:**
```bash
# 1. Review issues in LAST_RESULT.md
cat PROMPTS/MISSIONS/LAST_RESULT.md

# 2. Create new mission (DO NOT patch in-place)
# Mission ID: PHASE_III_FIX_001

# 3. Address specific failures only
```

## Phase IV Preview

**Next Up:** Word Deletion (Phase IV-A)
- Timeline: 2-3 weeks
- Scope: Click word → press Delete → audio removes word
- Foundation: All text editing features

**Key Decisions Needed:**
1. Forced alignment tool? (SOFA vs MFA vs Gentle)
2. Word replacement strategy? (Silence vs audio reuse)
3. UI model? (JUCE TextEditor vs custom)

## Troubleshooting Quick Fixes

**Issue: No console output**
```bash
# Rebuild in Debug mode
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
rm -rf build-Debug
mkdir build-Debug && cd build-Debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

**Issue: Transcription never starts**
```bash
# Check model
ls -lh ~/Library/Application\ Support/VoxScript/models/ggml-base.en.bin

# Check temp file created
ls -la /tmp/voxscript_*.wav
```

**Issue: Temp files not cleaned up**
```bash
# Manual cleanup
rm /tmp/voxscript_*.wav

# Check callbacks firing
grep "Deleting temp file" console_output.log
```

**Issue: Reaper crashes**
```bash
# Check crash logs
open ~/Library/Logs/DiagnosticReports/
ls -lt | grep REAPER | head -5
```

## Contact Points

**Phase III Owner:** Avishay (ARCHITECT)  
**Implementation:** IMPLEMENTER (Cursor AI) ✅ COMPLETE  
**Verification:** VERIFIER (Cursor AI) ⏳ PENDING  
**Decision:** ARCHITECT (Avishay) ⏳ PENDING  

---

**Current Status:** Phase III Implementation Complete  
**Next Action:** Run VERIFIER  
**Time Required:** ~2 hours  
**Timeline:** On Track 🎯
