/*
  ==============================================================================
    WhisperEngine.cpp
    
    Implementation of WhisperEngine transcription system
    
    Part of VoxScript Phase II: Transcription Engine
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "WhisperEngine.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <whisper.h>

namespace VoxScript
{

WhisperEngine::WhisperEngine()
    : juce::Thread ("WhisperEngine")
{
    DBG ("WhisperEngine: Initializing");
}

WhisperEngine::~WhisperEngine()
{
    DBG ("WhisperEngine: Shutting down");
    
    // Signal thread to stop
    shouldCancel = true;
    
    // Wait for thread to finish (with timeout)
    stopThread (5000);
    
    // Free whisper context
    if (ctx != nullptr)
    {
        whisper_free (ctx);
        ctx = nullptr;
    }
    
    DBG ("WhisperEngine: Destroyed");
}

//==============================================================================
// Listener Management

void WhisperEngine::addListener (Listener* listener)
{
    listeners.add (listener);
}

void WhisperEngine::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
// Public API

void WhisperEngine::transcribeAudioSource (juce::ARAAudioSource* audioSource)
{
    if (audioSource == nullptr)
    {
        notifyFailed ("NULL audio source provided");
        return;
    }
    
    // Store source for processing
    currentAudioSource = audioSource;
    currentAudioFile = juce::File(); // Clear file if source provided
    
    // Reset cancel flag
    shouldCancel = false;
    
    // Start background thread if not already running
    if (!isThreadRunning())
    {
        startThread();
    }
    else
    {
        DBG ("WhisperEngine: Thread already running, queuing not implemented in Phase II");
        notifyFailed ("Transcription already in progress");
    }
}

void WhisperEngine::transcribeAudioFile (const juce::File& audioFile)
{
    if (!audioFile.existsAsFile())
    {
        notifyFailed ("Audio file does not exist: " + audioFile.getFullPathName());
        return;
    }
    
    // Store file for processing
    currentAudioFile = audioFile;
    currentAudioSource = nullptr; // Clear source if file provided
    
    // Reset cancel flag
    shouldCancel = false;
    
    // Start background thread if not already running
    if (!isThreadRunning())
    {
        startThread();
    }
    else
    {
        DBG ("WhisperEngine: Thread already running, queueing not implemented in Phase II");
        notifyFailed ("Transcription already in progress");
    }
}

void WhisperEngine::cancelTranscription()
{
    shouldCancel = true;
}

//==============================================================================
// Thread Implementation

void WhisperEngine::run()
{
    DBG ("================================================");
    DBG ("WhisperEngine: Background thread started");
    DBG ("================================================");
    
    // Step 1: Extract audio if source is provided
    if (currentAudioSource != nullptr)
    {
        DBG ("WhisperEngine: Extracting audio from source...");
        notifyProgress (0.0f); // Signal start
        
        currentAudioFile = AudioExtractor::extractToTempWAV (currentAudioSource);

        
        if (!currentAudioFile.existsAsFile())
        {
            notifyFailed ("Audio extraction failed");
            return;
        }
        
        DBG ("WhisperEngine: Audio extracted to: " + currentAudioFile.getFullPathName());
    }

    // Load model if not already loaded
    if (ctx == nullptr)
    {
        loadModel();
        
        if (ctx == nullptr)
        {
            // Model failed to load, error already reported
            // Cleanup temp file if we created one
            if (currentAudioSource != nullptr && currentAudioFile.existsAsFile())
                currentAudioFile.deleteFile();
            return;
        }
    }
    
    // Process the audio file
    processAudio();
    
    // Cleanup temp file if it was created by extraction
    if (currentAudioSource != nullptr && currentAudioFile.existsAsFile())
    {
        DBG ("WhisperEngine: Deleting temp file: " + currentAudioFile.getFullPathName());
        currentAudioFile.deleteFile();
    }
    
    DBG ("WhisperEngine: Background thread finished");
}

//==============================================================================
// Model Loading

void WhisperEngine::loadModel()
{
    DBG ("WhisperEngine: Loading whisper model");
    
    // Phase II: Hardcoded model path
    // Phase III: Make this configurable via settings
    juce::File appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    
#if JUCE_MAC
    appData = appData.getChildFile ("Application Support");
#endif

    juce::File modelFile = appData.getChildFile ("VoxScript")
                                 .getChildFile ("models")
                                 .getChildFile ("ggml-base.en.bin");
    
    if (!modelFile.existsAsFile())
    {
        juce::String errorMsg = "Whisper model not found. Please download ggml-base.en.bin to:\n" + 
                                modelFile.getFullPathName();
        DBG ("WhisperEngine: " + errorMsg);
        notifyFailed (errorMsg);
        return;
    }
    
    DBG ("WhisperEngine: Model file found: " + modelFile.getFullPathName());
    
    // Load model using whisper.cpp API
    whisper_context_params cparams = whisper_context_default_params();
    ctx = whisper_init_from_file_with_params (modelFile.getFullPathName().toRawUTF8(), cparams);
    
    if (ctx == nullptr)
    {
        notifyFailed ("Failed to initialize whisper model");
        return;
    }
    
    DBG ("WhisperEngine: Model loaded successfully");
}

//==============================================================================
// Audio Processing

void WhisperEngine::processAudio()
{
    if (ctx == nullptr)
    {
        notifyFailed ("Whisper model not loaded");
        return;
    }
    
    if (!currentAudioFile.existsAsFile())
    {
        notifyFailed ("Audio file not found");
        return;
    }
    
    DBG ("================================================");
    DBG ("WhisperEngine: Processing audio file");
    DBG ("File: " + currentAudioFile.getFullPathName());
    DBG ("================================================");
    
    // Read audio file using JUCE
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (currentAudioFile));
    
    if (reader == nullptr)
    {
        notifyFailed ("Failed to read audio file (unsupported format?)");
        return;
    }
    
    // Get audio properties
    auto sampleRate = reader->sampleRate;
    auto numChannels = reader->numChannels;
    auto numSamples = reader->lengthInSamples;
    
    DBG ("WhisperEngine: Audio properties:");
    DBG ("  Sample rate: " + juce::String (sampleRate) + " Hz");
    DBG ("  Channels: " + juce::String (numChannels));
    DBG ("  Samples: " + juce::String (numSamples));
    DBG ("  Duration: " + juce::String (numSamples / sampleRate) + " seconds");
    
    // Read audio into buffer
    juce::AudioBuffer<float> audioBuffer (static_cast<int> (numChannels), 
                                          static_cast<int> (numSamples));
    reader->read (&audioBuffer, 0, static_cast<int> (numSamples), 0, true, true);
    
    // Convert to mono if needed (whisper expects mono)
    std::vector<float> pcmData;
    pcmData.resize (static_cast<size_t> (numSamples));
    
    if (numChannels == 1)
    {
        // Already mono, just copy
        std::copy (audioBuffer.getReadPointer (0), 
                   audioBuffer.getReadPointer (0) + numSamples,
                   pcmData.begin());
    }
    else
    {
        // Mix to mono
        for (juce::int64 i = 0; i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < numChannels; ++ch)
                sum += audioBuffer.getSample (static_cast<int> (ch), static_cast<int> (i));
            pcmData[static_cast<size_t> (i)] = sum / static_cast<float> (numChannels);
        }
    }
    
    // Resample to 16kHz if needed (whisper expects 16kHz)
    if (sampleRate != 16000.0)
    {
        DBG ("WhisperEngine: Resampling from " + juce::String (sampleRate) + " to 16000 Hz");
        
        // Simple linear interpolation resampling
        double ratio = 16000.0 / sampleRate;
        size_t newSize = static_cast<size_t> (pcmData.size() * ratio);
        std::vector<float> resampled (newSize);
        
        for (size_t i = 0; i < newSize; ++i)
        {
            double srcIndex = static_cast<double> (i) / ratio;
            size_t idx0 = static_cast<size_t> (srcIndex);
            size_t idx1 = juce::jmin (idx0 + 1, pcmData.size() - 1);
            float frac = static_cast<float> (srcIndex - idx0);
            
            resampled[i] = pcmData[idx0] * (1.0f - frac) + pcmData[idx1] * frac;
        }
        
        pcmData = std::move (resampled);
    }
    
    DBG ("WhisperEngine: PCM data prepared: " + juce::String (pcmData.size()) + " samples at 16kHz");
    
    // Notify start
    notifyProgress (0.0f);
    
    // Configure whisper parameters - AGGRESSIVE ANTI-HALLUCINATION MODE
    whisper_full_params params = whisper_full_default_params (WHISPER_SAMPLING_GREEDY);
    
    params.print_realtime   = false;
    params.print_progress   = false;
    params.print_timestamps = true;
    params.print_special    = false;
    params.translate        = false;
    
    // LANGUAGE SETTINGS
    params.language         = "en";
    params.detect_language  = false;  // Force English, don't auto-detect
    
    params.n_threads        = 4;
    params.offset_ms        = 0;
    params.duration_ms      = 0;
    
    // MAXIMUM ANTI-HALLUCINATION SETTINGS
    params.suppress_blank   = true;
    params.suppress_non_speech_tokens = true;
    
    // CRITICAL: Disable audio context completely
    params.no_context       = true;
    params.audio_ctx        = 0;  // Zero audio context (maximum anti-hallucination)
    
    // AGGRESSIVE: Reject hallucinated content
    params.entropy_thold    = 2.0f;   // More aggressive (lower = more rejections)
    params.logprob_thold    = -0.5f;  // Much more aggressive (higher = more rejections)
    params.no_speech_thold  = 0.3f;   // Detect non-speech more aggressively
    
    // Word-level timestamps
    params.max_len          = 0;
    params.token_timestamps = true;
    params.split_on_word    = true;
    
    // Decoding parameters
    params.beam_search.beam_size = 5;
    params.greedy.best_of        = 5;
    params.temperature      = 0.0f;  // Deterministic
    params.temperature_inc  = 0.0f;  // Don't increase temp on failure
    
    // PROMPT ENGINEERING: Be extremely explicit
    params.initial_prompt   = "[LYRICS] This is a vocal recording. Transcribe only the sung or spoken words. Ignore background music.";
    
    DBG ("WhisperEngine: Running whisper inference...");
    
    // Run transcription
    int result = whisper_full (ctx, params, pcmData.data(), static_cast<int> (pcmData.size()));
    
    if (result != 0)
    {
        notifyFailed ("Whisper transcription failed with code: " + juce::String (result));
        return;
    }
    
    // Check for cancellation
    if (shouldCancel)
    {
        notifyFailed ("Transcription cancelled by user");
        return;
    }
    
    DBG ("WhisperEngine: Transcription complete, extracting results");
    
    // Extract results and build VoxSequence
    VoxSequence sequence;
    
    int numSegments = whisper_full_n_segments (ctx);
    DBG ("WhisperEngine: Processing " + juce::String (numSegments) + " segments");
    
    for (int i = 0; i < numSegments; ++i)
    {
        const char* text = whisper_full_get_segment_text (ctx, i);
        int64_t t0 = whisper_full_get_segment_t0 (ctx, i);
        int64_t t1 = whisper_full_get_segment_t1 (ctx, i);
        
        VoxSegment segment;
        segment.text = juce::String::fromUTF8 (text);
        DBG ("WhisperEngine: Segment " + juce::String(i) + " text: " + segment.text);
        segment.startTime = static_cast<double> (t0) / 100.0;  // Convert to seconds
        segment.endTime = static_cast<double> (t1) / 100.0;
        
        // FIXME Phase III: Extract word-level timestamps
        // For Phase II, we'll just create a single word per segment
        VoxWord word;
        word.text = segment.text;
        word.startTime = segment.startTime;
        word.endTime = segment.endTime;
        word.confidence = 1.0f;  // Whisper doesn't provide confidence scores directly
        
        segment.words.add (word);
        sequence.addSegment (segment);
        
        // Update progress
        notifyProgress (static_cast<float> (i + 1) / static_cast<float> (numSegments));
    }
    
    DBG ("================================================");
    DBG ("WhisperEngine: Transcription SUCCESS");
    DBG ("Segments: " + juce::String (sequence.getSegments().size()));
    DBG ("Words: " + juce::String (sequence.getWordCount()));
    DBG ("Duration: " + juce::String (sequence.getTotalDuration()) + " seconds");
    DBG ("================================================");
    
    // Notify completion
    notifyComplete (sequence);
}

//==============================================================================
// Notification Helpers

void WhisperEngine::notifyProgress (float progress)
{
    // Dispatch to message thread for UI safety
    juce::MessageManager::callAsync ([this, progress]()
    {
        listeners.call ([progress] (Listener& l)
        {
            l.transcriptionProgress (progress);
        });
    });
}

void WhisperEngine::notifyComplete (VoxSequence sequence)
{
    // Dispatch to message thread for UI safety
    juce::MessageManager::callAsync ([this, sequence]()
    {
        listeners.call ([sequence] (Listener& l)
        {
            l.transcriptionComplete (sequence);
        });
    });
}

void WhisperEngine::notifyFailed (const juce::String& error)
{
    // Dispatch to message thread for UI safety
    juce::MessageManager::callAsync ([this, error]()
    {
        listeners.call ([error] (Listener& l)
        {
            l.transcriptionFailed (error);
        });
    });
}

} // namespace VoxScript
