/*
  ==============================================================================
    WhisperEngine.h
    
    Background transcription engine using whisper.cpp
    Manages model loading, audio processing, and callback notifications
    
    Part of VoxScript Phase II: Transcription Engine
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "VoxSequence.h"
#include "AudioExtractor.h"
#include <atomic>

// Forward declare whisper_context from whisper.h (global namespace)
struct whisper_context;

namespace VoxScript
{

/**
 * @brief Background transcription engine using whisper.cpp
 * 
 * This class manages:
 * - Loading whisper.cpp model from disk
 * - Background thread for transcription (NOT audio thread, NOT message thread)
 * - Progress callbacks
 * - Conversion of whisper output to VoxSequence
 * 
 * Thread Safety:
 * - Transcription runs on dedicated juce::Thread
 * - Callbacks are dispatched to message thread via MessageManager
 * - No allocations on audio thread
 * 
 * Phase II: Basic transcription with ggml-base.en model
 * Phase III: Model selection, cancellation, queue management
 */
class WhisperEngine : public juce::Thread
{
public:
    WhisperEngine();
    ~WhisperEngine() override;
    
    //==========================================================================
    /**
     * @brief Listener interface for transcription callbacks
     * 
     * All callbacks are dispatched on the message thread for UI safety.
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /**
         * Called periodically during transcription
         * @param progress Value between 0.0 and 1.0
         */
        virtual void transcriptionProgress (float progress) = 0;
        
        /**
         * Called when transcription completes successfully
         * @param sequence The complete transcription result
         */
        virtual void transcriptionComplete (VoxSequence sequence) = 0;
        
        /**
         * Called when transcription fails
         * @param error Human-readable error message
         */
        virtual void transcriptionFailed (const juce::String& error) = 0;
    };
    
    //==========================================================================
    void addListener (Listener* listener);
    void removeListener (Listener* listener);
    
    /**
     * Start transcription of an ARA audio source
     * This is non-blocking - extraction and transcription happen on background thread
     * 
     * @param audioSource The ARA audio source to transcribe
     */
    void transcribeAudioSource (juce::ARAAudioSource* audioSource);
    
    /**
     * Start transcription of an audio file
     * This is non-blocking - transcription happens on background thread
     * 
     * @param audioFile Path to audio file (WAV, MP3, etc.)
     */
    void transcribeAudioFile (const juce::File& audioFile);
    
    /**
     * Cancel ongoing transcription
     * Thread will stop gracefully at next checkpoint
     */
    void cancelTranscription();
    
    /**
     * Thread implementation - runs transcription in background
     */
    void run() override;

private:
    //==========================================================================
    juce::ListenerList<Listener> listeners;
    juce::ARAAudioSource* currentAudioSource = nullptr;
    juce::File currentAudioFile;
    // AudioExtractor is static, removed member instance

    std::atomic<bool> shouldCancel {false};
    
    // Opaque pointer to whisper_context (forward declared above)
    // We don't include whisper.h in header to avoid polluting namespace
    ::whisper_context* ctx = nullptr;
    
    //==========================================================================
    /**
     * Load whisper model from disk
     * Phase II: Hardcoded path to ggml-base.en.bin
     * Phase III: User-configurable model selection
     */
    void loadModel();
    
    /**
     * Process the current audio file and generate VoxSequence
     */
    void processAudio();
    
    /**
     * Notify listeners of progress (dispatched to message thread)
     */
    void notifyProgress (float progress);
    
    /**
     * Notify listeners of completion (dispatched to message thread)
     */
    void notifyComplete (VoxSequence sequence);
    
    /**
     * Notify listeners of failure (dispatched to message thread)
     */
    void notifyFailed (const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhisperEngine)
};

} // namespace VoxScript
