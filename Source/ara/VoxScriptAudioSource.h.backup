/*
  ==============================================================================
    VoxScriptAudioSource.h
    
    Custom ARA Audio Source - represents raw audio files
    This is where transcription will be triggered in Phase II
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace VoxScript
{

/**
 * @brief Custom ARA Audio Source for VoxScript
 * 
 * Represents the raw audio file on disk. This object is treated as immutable.
 * 
 * From PDF Section 3.1.2: "ARAAudioSource: This object represents the raw audio 
 * file on the user's hard drive. It provides access to the sample data. VoxScript 
 * treats this as immutable. We analyze this source to generate the transcription."
 * 
 * In Phase II, this is where we'll:
 * - Spawn background thread for VAD + STT
 * - Store the VoxSequence (transcription + timing data)
 * - Provide access to transcription for the UI
 */
class VoxScriptAudioSource : public juce::ARAAudioSource
{
public:
    //==========================================================================
    VoxScriptAudioSource (juce::ARADocument* document,
                         ARA::ARAAudioSourceHostRef hostRef);
    
    ~VoxScriptAudioSource() override;
    
    //==========================================================================
    // ARA Audio Source notifications
    
    /**
     * Called when the audio source properties are updated by the host
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void notifyPropertiesUpdated() noexcept;
    
    /**
     * Called when the audio source content (samples) is modified
     * This triggers re-analysis
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void notifyContentChanged (juce::ARAContentUpdateScopes scopeFlags) noexcept;
    
    //==========================================================================
    // Phase II: Transcription API (placeholder for now)
    
    /**
     * Get the transcribed text for this audio source
     * Returns empty string in Phase I
     */
    juce::String getTranscription() const;
    
    /**
     * Check if transcription is complete
     * Returns false in Phase I
     */
    bool isTranscriptionReady() const;
    
    /**
     * Get transcription progress (0.0 to 1.0)
     * Returns 0.0 in Phase I
     */
    float getTranscriptionProgress() const;
    
    //==========================================================================
    // Phase I: Placeholder for future transcription data
    // Phase II: Will add VoxSequence member, analysis thread, etc.
    
    bool transcriptionReady { false };
    float transcriptionProgress { 0.0f };
    juce::String transcriptionText;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptAudioSource)
};

} // namespace VoxScript
