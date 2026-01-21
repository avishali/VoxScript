## 🔍 AUDIO PASSTHROUGH DEBUG

**Issue:** No audio passing through during playback in Reaper
**Expected:** Audio should play even without edits (passthrough)
**Current:** Silence during playback

---

## 🎯 Likely Causes

### 1. PlaybackRenderer Not Processing Correctly

**Current Code:**
```cpp
bool VoxScriptPlaybackRenderer::processBlock(...)
{
    // Just calls base class
    return juce::ARAPlaybackRenderer::processBlock(buffer, realtime, positionInfo);
}
```

**Problem:** Base class might not be rendering audio samples automatically.

**Solution:** Implement explicit audio reading from ARA sources.

---

### 2. No Playback Regions Created

**Check:** Are playback regions being created when you add audio?

**Debug:** Look for this in console:
```
VOXSCRIPT: Creating Playback Region
```

**If missing:** Regions aren't being created, audio won't play.

---

### 3. Audio Not Being Read from ARA Source

**Problem:** Even if regions exist, we're not reading samples from them.

**Solution:** Manually read from playback regions in processBlock().

---

## ✅ IMMEDIATE FIX TO TRY

Replace the `processBlock()` implementation with explicit audio rendering:

**File:** `Source/ara/VoxScriptPlaybackRenderer.cpp`

**Replace this:**
```cpp
bool VoxScriptPlaybackRenderer::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::AudioProcessor::Realtime realtime,
                                             const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept
{
    return juce::ARAPlaybackRenderer::processBlock (buffer, realtime, positionInfo);
}
```

**With this:**
```cpp
bool VoxScriptPlaybackRenderer::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::AudioProcessor::Realtime realtime,
                                             const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept
{
    // Clear buffer first
    buffer.clear();
    
    // Get playback regions
    const auto& regions = getPlaybackRegions();
    
    if (regions.isEmpty())
    {
        // No regions = no audio (silence)
        return true;
    }
    
    // Get current playback sample position from position info
    if (!positionInfo.getTimeInSamples().hasValue())
        return true;
    
    auto playbackSamplePosition = *positionInfo.getTimeInSamples();
    
    // Iterate through all playback regions and render them
    for (auto* region : regions)
    {
        // Get region's time range in samples
        auto regionStartInPlayback = region->getStartInPlaybackSamples();
        auto regionEndInPlayback = region->getEndInPlaybackSamples();
        
        // Check if this region overlaps with current buffer
        auto bufferStart = playbackSamplePosition;
        auto bufferEnd = playbackSamplePosition + buffer.getNumSamples();
        
        if (regionEndInPlayback <= bufferStart || regionStartInPlayback >= bufferEnd)
            continue; // Region not in range
        
        // Calculate overlap
        auto overlapStart = juce::jmax(regionStartInPlayback, bufferStart);
        auto overlapEnd = juce::jmin(regionEndInPlayback, bufferEnd);
        auto overlapLength = overlapEnd - overlapStart;
        
        if (overlapLength <= 0)
            continue;
        
        // Calculate offsets
        auto offsetInBuffer = (int)(overlapStart - bufferStart);
        auto offsetInRegion = (int)(overlapStart - regionStartInPlayback);
        
        // Get audio source
        auto* audioSource = region->getAudioModification()->getAudioSource();
        if (!audioSource)
            continue;
        
        // Read audio from source
        juce::AudioBuffer<float> tempBuffer(buffer.getNumChannels(), (int)overlapLength);
        
        // Use ARA's host audio reader to get samples
        if (auto* docController = getDocumentController())
        {
            ARA::PlugIn::HostAudioReader reader(audioSource);
            
            void* bufferPtrs[2];
            bufferPtrs[0] = tempBuffer.getWritePointer(0);
            bufferPtrs[1] = tempBuffer.getNumChannels() > 1 ? tempBuffer.getWritePointer(1) : nullptr;
            
            if (reader.readAudioSamples(offsetInRegion, (int)overlapLength, bufferPtrs))
            {
                // Add to output buffer
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    if (ch < tempBuffer.getNumChannels())
                    {
                        buffer.addFrom(ch, offsetInBuffer, tempBuffer, ch, 0, (int)overlapLength);
                    }
                }
            }
        }
    }
    
    return true;
}
```

---

## 🧪 TESTING STEPS

After applying the fix:

1. **Rebuild:**
   ```bash
   cd build-Debug && ninja -v
   ```

2. **Test in Reaper:**
   - Add audio clip to track with VoxScript
   - Press play
   - **Expected:** Audio should play through

3. **Check Console:**
   - Look for "Creating Playback Region" messages
   - Look for any error messages

---

## 📊 ALTERNATIVE: Simpler Debug First

Before implementing the full fix above, let's add debug output to see what's happening:

**Add this to the START of processBlock():**
```cpp
bool VoxScriptPlaybackRenderer::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::AudioProcessor::Realtime realtime,
                                             const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept
{
    // DEBUG: Log what's happening
    static int callCount = 0;
    if (callCount++ % 100 == 0)  // Log every 100th call
    {
        DBG("PlaybackRenderer: processBlock called, regions: " + 
            juce::String(getPlaybackRegions().size()) +
            ", buffer size: " + juce::String(buffer.getNumSamples()));
    }
    
    // ... rest of code ...
}
```

**Then test and check console output. Tell me what you see!**

---

## 🤔 QUESTIONS TO ANSWER

1. **Do you see "Creating Playback Region" in console when adding audio?**
   - YES → Regions are being created
   - NO → Problem is earlier in the chain

2. **Does the base class processBlock return true or false?**
   - TRUE → Should be rendering
   - FALSE → Something is wrong

3. **Are there any error messages in console during playback?**
   - Report any errors you see

---

## 📝 NEXT STEPS

**Option 1: Apply debug logging** (5 minutes)
- Add the debug DBG statement above
- Test and report what you see
- We'll diagnose from there

**Option 2: Apply full fix** (10 minutes)
- Replace processBlock() with explicit rendering code above
- This should definitely make audio work

**Which would you prefer to try first?**
