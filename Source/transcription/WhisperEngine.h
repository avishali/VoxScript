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
#include "../engine/AudioCache.h"
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
class WhisperEngine
{
public:
    WhisperEngine();
    ~WhisperEngine();
    
    //==========================================================================
    // Phase III: Synchronous API (driven by TranscriptionJobQueue)
    
    /**
     * @brief Process an audio file synchronously.
     * This blocks until transcription is complete or cancelled.
     * 
     * @param audioFile Path to audio file
     * @return VoxSequence Resulting transcription (empty on failure/cancel)
     */
    VoxSequence processSync (const juce::File& audioFile);

    /**
     * @brief Process an audio source synchronously.
     * Extracts audio to temp file, processes it, then deletes temp file.
     * 
     * @param source ARA Audio Source to process
     * @return VoxSequence Resulting transcription
     */
    VoxSequence processSync (juce::ARAAudioSource* source);

    /**
     * Cancel ongoing transcription
     * Thread-safe.
     */
    void cancelTranscription();
    
    /** Set the AudioCache to use for extraction */
    void setAudioCache(AudioCache* cache) { audioCache = cache; }

private:
    //==========================================================================
    // Internal state
    ::whisper_context* ctx = nullptr;
    
    //==========================================================================
    /**
     * Load whisper model from disk
     */
    void loadModel();
    
    //==========================================================================
    // Member Variables
    
    std::atomic<bool> shouldCancel { false };
    AudioCache* audioCache = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WhisperEngine)
};

} // namespace VoxScript
