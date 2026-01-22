/*
  ==============================================================================
    VoxScriptPlaybackRenderer.h
    
    ARA Playback Renderer - processes audio with text-based edits applied
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#pragma once
                        
#include <juce_audio_processors/juce_audio_processors.h>

namespace VoxScript
{

/**
 * @brief ARA Playback Renderer for VoxScript
 * 
 * This is the audio processing component that runs on the high-priority audio thread.
 * It reads audio from ARAAudioSource and applies modifications from ARAAudioModification.
 * 
 * From PDF Section 3.3: "The audio processing callback (processPlaybackRegion) runs 
 * on the high-priority audio thread. It cannot wait for locks."
 * 
 * Phase I: Simple passthrough
 * Phase III: Apply VoxEditList (cuts, crossfades, room tone synthesis)
 */
class VoxScriptPlaybackRenderer : public juce::ARAPlaybackRenderer
{
public:
    //==========================================================================
    // JUCE 8: Constructor takes DocumentController*, not host ref
    using juce::ARAPlaybackRenderer::ARAPlaybackRenderer;
    
    ~VoxScriptPlaybackRenderer() override;
    
    //==========================================================================
    // Audio Processing
    
    /**
     * Prepare for audio processing - called before playback starts
     */
    void prepareToPlay (double sampleRate,
                       int maximumSamplesPerBlock,
                       int numChannels,
                       juce::AudioProcessor::ProcessingPrecision precision,
                       AlwaysNonRealtime alwaysNonRealtime) override;
    
    /**
     * Release resources after playback stops
     */
    void releaseResources() override;
    
    /**
     * Main audio processing function
     * Phase I: Simple passthrough
     * Phase III: Apply text-based edits (VoxEditList)
     */
    bool processBlock (juce::AudioBuffer<float>& buffer,
                      juce::AudioProcessor::Realtime realtime,
                      const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept override;
    
    //==========================================================================
    // ARA Notifications
    
    /**
     * Called when playback regions are added/removed
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void notifyPlaybackRegionsChanged() noexcept;

private:
    //==========================================================================
    // Phase I: Basic state
    double currentSampleRate { 44100.0 };
    int maxBlockSize { 512 };
    int channelCount { 2 };  // Renamed to avoid shadowing warning
    
    // Phase III: Will add VoxEditList reader, crossfade state, room tone generator
    
    // Pre-allocated buffer for RT processing
    juce::AudioBuffer<float> tempBuffer;

    /**
     * Helper to calculate the correct sample offset within the Audio Source
     * for a given playback position relative to a region.
     */
    juce::int64 getAudioSourceOffset(ARA::PlugIn::PlaybackRegion* region, juce::int64 offsetInRegion) const
    {
        return region->getStartInAudioModificationSamples() + offsetInRegion;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptPlaybackRenderer)
};

} // namespace VoxScript
