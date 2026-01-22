/*
  ==============================================================================
    VoxScriptPlaybackRenderer.cpp
    
    Implementation of ARA Playback Renderer
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#include "VoxScriptPlaybackRenderer.h"

namespace VoxScript

{

// Constructor removed - using inherited constructor from base class

VoxScriptPlaybackRenderer::~VoxScriptPlaybackRenderer()
{
    juce::Logger::writeToLog ("VoxScriptPlaybackRenderer: Destroyed");
}

//==============================================================================
// Audio Processing Setup

void VoxScriptPlaybackRenderer::prepareToPlay (double sampleRate,
                                               int maximumSamplesPerBlock,
                                               int numChannels,
                                               juce::AudioProcessor::ProcessingPrecision precision,
                                               AlwaysNonRealtime alwaysNonRealtime)
{
    juce::Logger::writeToLog ("VoxScriptPlaybackRenderer: Prepare to play - SR: " 
                              + juce::String (sampleRate) 
                              + " MaxBlock: " + juce::String (maximumSamplesPerBlock) 
                              + " Channels: " + juce::String (numChannels));
    
    currentSampleRate = sampleRate;
    maxBlockSize = maximumSamplesPerBlock;
    channelCount = numChannels;
    
    juce::ignoreUnused (precision, alwaysNonRealtime);
    
    // Pre-allocate buffer for RT processing (max block size + some safety margin if desired, but exact is fine)
    tempBuffer.setSize (numChannels, maximumSamplesPerBlock);
    
    // Phase III: Initialize crossfade buffers, room tone generator, etc.
}

void VoxScriptPlaybackRenderer::releaseResources()
{
    juce::Logger::writeToLog ("VoxScriptPlaybackRenderer: Release resources");
    
    // Phase III: Clean up processing state
}

//==============================================================================
// Main Audio Processing

bool VoxScriptPlaybackRenderer::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::AudioProcessor::Realtime realtime,
                                             const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept
{
    // Clear buffer first
    buffer.clear();
    
    // Get playback regions
    const auto& regions = getPlaybackRegions();
    
    if (regions.empty())
        return true;
    
    // Get current playback sample position from position info
    if (!positionInfo.getTimeInSamples().hasValue())
        return true;
    
    auto playbackSamplePosition = *positionInfo.getTimeInSamples();
    
    // Iterate through all playback regions and render them
    for (auto* region : regions)
    {
        // Get region's timeframe in playback time
        auto regionStartInPlayback = region->getStartInPlaybackSamples (currentSampleRate);
        auto regionEndInPlayback = region->getEndInPlaybackSamples (currentSampleRate);
        
        // Check overlap
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
        // 1. Where in the buffer do we write?
        auto offsetInBuffer = (int)(overlapStart - bufferStart);
        
        // 2. Where in the region (relative to region start) are we reading?
        auto offsetInRegion = (int)(overlapStart - regionStartInPlayback);

        // 3. Where in the underlying AUDIO SOURCE is this?
        auto startPosInAudioSource = getAudioSourceOffset(region, offsetInRegion);
        
        // Get audio source
        auto* audioSource = region->getAudioModification()->getAudioSource();
        if (!audioSource)
            continue;
        
        // Use ARA's host audio reader to get samples
        ARA::PlugIn::HostAudioReader reader(audioSource);
        
        // RT-SAFE: Use pre-allocated member buffer
        if (overlapLength > tempBuffer.getNumSamples())
            continue;
            
        // RT-SAFE: Use fixed array
        void* bufferPtrs[16]; 
        int numChans = juce::jmin(tempBuffer.getNumChannels(), 16);
        
        for (int ch = 0; ch < numChans; ++ch)
            bufferPtrs[ch] = tempBuffer.getWritePointer(ch);
        
        // Read from the calculated SOURCE position
        if (reader.readAudioSamples(startPosInAudioSource, (int)overlapLength, bufferPtrs))
        {
            // Add to output buffer
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                if (ch < numChans)
                {
                    buffer.addFrom(ch, offsetInBuffer, tempBuffer, ch, 0, (int)overlapLength);
                }
            }
        }
    }
    
    return true;
}

//==============================================================================
// ARA Notifications

void VoxScriptPlaybackRenderer::notifyPlaybackRegionsChanged() noexcept
{
    juce::Logger::writeToLog ("VoxScriptPlaybackRenderer: Playback regions changed - Count: " 
                              + juce::String (getPlaybackRegions().size()));
    
    // Called when clips are added/removed/moved in the DAW timeline
    // Phase III: Update internal edit list cache
}

} // namespace VoxScript
