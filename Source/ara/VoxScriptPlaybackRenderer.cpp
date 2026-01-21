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
        auto regionStartInPlayback = region->getStartInPlaybackSamples (currentSampleRate);
        auto regionEndInPlayback = region->getEndInPlaybackSamples (currentSampleRate);
        
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
        ARA::PlugIn::HostAudioReader reader(audioSource);
        
        // Prepare write pointers for reader
        std::vector<void*> bufferPtrs(static_cast<size_t>(tempBuffer.getNumChannels()));
        for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch)
            bufferPtrs[static_cast<size_t>(ch)] = tempBuffer.getWritePointer(ch);
        
        if (reader.readAudioSamples(offsetInRegion, (int)overlapLength, bufferPtrs.data()))
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
