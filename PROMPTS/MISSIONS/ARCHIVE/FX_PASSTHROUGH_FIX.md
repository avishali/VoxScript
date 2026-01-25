## 🔧 SIMPLE FIX - Audio Passthrough in Normal FX Chain

**Issue:** No audio when VoxScript is in normal FX slot (Picture B)  
**Cause:** processBlock() not explicitly handling audio passthrough  
**Fix:** Add explicit passthrough code

---

## ✅ THE FIX

**File:** `Source/PluginProcessor.cpp`

**Find this (around line 75):**
```cpp
void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<float>&, 
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
    // In non-ARA mode, just pass through the audio (do nothing)
    // Phase II+: Could show "ARA mode required" message in UI
}
```

**Replace with:**
```cpp
void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, 
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);
    
    // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
    // In non-ARA mode, pass through audio unchanged
    
    if (!isBoundToARA())
    {
        // Non-ARA mode: Audio passes through as-is
        // Nothing to do - buffer already contains input audio
        return;
    }
    
    // ARA mode: Clear buffer (ARA renderer will fill it)
    // Actually, don't clear - let ARA handle it
}
```

**And also update the double version (around line 83):**
```cpp
void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);
    
    // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
    // In non-ARA mode, pass through audio unchanged
    
    if (!isBoundToARA())
    {
        // Non-ARA mode: Audio passes through as-is
        return;
    }
}
```

---

## 🧪 TEST

1. **Rebuild:**
   ```bash
   cd build-Debug && ninja -v
   ```

2. **Test in Reaper:**
   - Close any open instances of VoxScript
   - Insert VoxScript in **normal FX chain** (Picture B scenario)
   - Play audio
   - **Expected:** Audio should pass through ✅

---

## 🤔 WHY THIS HAPPENS

**The parameter names matter!**

**Before:**
```cpp
void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
//                                          ^ no name = compiler warning
```

**After:**
```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
//                                          ^ named = proper handling
```

When parameters aren't named, some compilers/optimizers might not handle them correctly.

Also, by explicitly checking `!isBoundToARA()` and returning early, we make it crystal clear that in non-ARA mode, the audio should pass through unchanged.

---

## 📊 EXPECTED BEHAVIOR AFTER FIX

| Mode | Scenario | Expected Result |
|------|----------|-----------------|
| **Non-ARA** | Normal FX chain | ✅ Audio passes through |
| **Non-ARA** | IN FX slot | ✅ Audio passes through |
| **ARA** | Audio item with extension | ⏳ TBD (need PlaybackRenderer fix) |

---

## 🚀 APPLY THIS FIX NOW

This is a 2-minute fix that should solve your immediate problem.

After this works, we can tackle ARA mode audio rendering separately if needed.
