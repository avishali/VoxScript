/*
  ==============================================================================
    WhisperEngine.cpp
    
    Implementation of WhisperEngine transcription system
    
    Part of VoxScript Phase II/III: Transcription Engine
    Mission 3: Isolate Whisper (Refactored to synchronous API)
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "WhisperEngine.h"
#include "../engine/AudioCache.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <whisper.h>

namespace VoxScript
{

WhisperEngine::WhisperEngine()
{
    DBG ("WhisperEngine: Initializing");
}

WhisperEngine::~WhisperEngine()
{
    DBG ("WhisperEngine: Shutting down");
    
    // Signal to stop any ongoing process (if called from another thread)
    cancelTranscription();
    
    // Free whisper context
    if (ctx != nullptr)
    {
        whisper_free (ctx);
        ctx = nullptr;
    }
    
    DBG ("WhisperEngine: Destroyed");
}

//==============================================================================
// Public API

VoxSequence WhisperEngine::processSync (const juce::File& audioFile)
{
    // Reset cancel flag at start of new job
    shouldCancel = false;

    if (!audioFile.existsAsFile())
    {
        DBG ("WhisperEngine: Audio file does not exist: " + audioFile.getFullPathName());
        return {};
    }

    // Load model if not already loaded (Lazy Loading)
    if (ctx == nullptr)
    {
        loadModel();
        if (ctx == nullptr)
        {
            // Error already logged
            return {}; 
        }
    }
    
    DBG ("================================================");
    DBG ("WhisperEngine: Processing audio file");
    DBG ("File: " + audioFile.getFullPathName());
    DBG ("================================================");
    
    // Read audio file using JUCE
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (audioFile));
    
    if (reader == nullptr)
    {
        DBG ("WhisperEngine: Failed to read audio file (unsupported format?)");
        return {};
    }
    
    // Get audio properties
    auto sampleRate = reader->sampleRate;
    auto numChannels = reader->numChannels;
    auto numSamples = reader->lengthInSamples;
    
    DBG ("WhisperEngine: Audio properties:");
    DBG ("  Sample rate: " + juce::String (sampleRate) + " Hz");
    DBG ("  Channels: " + juce::String (numChannels));
    DBG ("  Samples: " + juce::String (numSamples));
    
    // Read audio into buffer
    juce::AudioBuffer<float> audioBuffer (static_cast<int> (numChannels), 
                                          static_cast<int> (numSamples));
    reader->read (&audioBuffer, 0, static_cast<int> (numSamples), 0, true, true);
    
    // Convert to mono
    std::vector<float> pcmData;
    pcmData.resize (static_cast<size_t> (numSamples));
    
    if (numChannels == 1)
    {
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
    
    // Resample to 16kHz
    if (sampleRate != 16000.0)
    {
        DBG ("WhisperEngine: Resampling from " + juce::String (sampleRate) + " to 16000 Hz");
        
        double ratio = 16000.0 / sampleRate;
        size_t newSize = static_cast<size_t> (pcmData.size() * ratio);
        std::vector<float> resampled (newSize);
        
        for (size_t i = 0; i < newSize; ++i)
        {
            if (shouldCancel) return {}; 

            double srcIndex = static_cast<double> (i) / ratio;
            size_t idx0 = static_cast<size_t> (srcIndex);
            size_t idx1 = juce::jmin (idx0 + 1, pcmData.size() - 1);
            float frac = static_cast<float> (srcIndex - idx0);
            
            resampled[i] = pcmData[idx0] * (1.0f - frac) + pcmData[idx1] * frac;
        }
        
        pcmData = std::move (resampled);
    }
    
    if (shouldCancel) return {}; 
    
    // Configure whisper parameters
    // Change 1: Revert to Greedy (Mission 5 fix caused crash with Beam)
    whisper_full_params params = whisper_full_default_params (WHISPER_SAMPLING_GREEDY);
    
    params.print_realtime   = false;
    params.print_progress   = false;
    params.print_timestamps = true;
    params.print_special    = false;
    params.translate        = false;
    params.language         = "en";
    params.detect_language  = false;
    params.n_threads        = 4;
    params.offset_ms        = 0;
    params.duration_ms      = 0;
    
    // Anti-Hallucination settings (Updated for Mission 5, Kept for Mission 5.1)
    params.suppress_blank   = true;
    params.suppress_non_speech_tokens = true;
    params.no_context       = true;
    params.audio_ctx        = 0; 
    params.entropy_thold    = 2.0f;

    // Change 4: Stricter gating (Kept)
    params.logprob_thold    = -1.0f; // More conservative (was -0.5f)
    params.no_speech_thold  = 0.6f;  // Higher threshold for silence (was 0.3f)

    params.max_len          = 0;
    params.token_timestamps = true;
    params.split_on_word    = true;
    
    // Change 2: Beam search settings (Reverted/Ignored for Greedy)
    params.beam_search.beam_size = -1; // Default/Disabled
    params.greedy.best_of        = 5; 
    
    params.temperature      = 0.0f;
    params.temperature_inc  = 0.0f;
    
    // Change 3: Neutral prompt (Kept)
    params.initial_prompt   = "Transcribe the vocal words you can clearly hear. If unsure, output nothing.";
    
    DBG ("WhisperEngine: Running whisper inference...");
    
    // Run transcription
    // Note: whisper_full is blocking. We can't easily cancel it mid-inference unless we use a callback
    // or if whisper supports an abort flag in params.
    // For now, we check cancellation before.
    
    int result = whisper_full (ctx, params, pcmData.data(), static_cast<int> (pcmData.size()));
    
    if (result != 0)
    {
        DBG ("WhisperEngine: Transcription failed with code: " + juce::String (result));
        return {};
    }
    
    if (shouldCancel) return {}; 
    
    DBG ("WhisperEngine: Transcription complete, extracting results");
    
    int numSegments = whisper_full_n_segments (ctx);

    // Change 5: Post-run guard for empty/junk results
    if (numSegments == 0)
    {
        DBG ("WhisperEngine: No segments found.");
        return {};
    }
    
    // Extract results
    VoxSequence sequence;
    juce::String combinedText;
    
    for (int i = 0; i < numSegments; ++i)
    {
        if (shouldCancel) return {};
        
        const char* text = whisper_full_get_segment_text (ctx, i);
        int64_t t0 = whisper_full_get_segment_t0 (ctx, i);
        int64_t t1 = whisper_full_get_segment_t1 (ctx, i);
        
        VoxSegment segment;
        segment.text = juce::String::fromUTF8 (text);
        combinedText += segment.text;

        segment.startTime = static_cast<double> (t0) / 100.0;
        segment.endTime = static_cast<double> (t1) / 100.0;
        
        // Single word approximation for Phase II/III parity to previous impl
        VoxWord word;
        word.text = segment.text;
        word.startTime = segment.startTime;
        word.endTime = segment.endTime;
        word.confidence = 1.0f;
        
        segment.words.add (word);
        sequence.addSegment (segment);
    }
    
    // Change 5b: Check total text length to filter out noise/junk
    // Using a minimal length check (e.g. < 2 chars)
    if (combinedText.trim().length() < 2)
    {
        DBG ("WhisperEngine: Result too short ('" + combinedText + "'), treating as silence.");
        return {};
    }
    
    DBG ("WhisperEngine: Success. " + juce::String(sequence.getWordCount()) + " words.");
    return sequence;
}

VoxSequence WhisperEngine::processSync (juce::ARAAudioSource* source)
{
    // Reset cancel flag
    shouldCancel = false;

    if (source == nullptr) return {};
    if (audioCache == nullptr)
    {
        DBG ("WhisperEngine: AudioCache not set!");
        return {};
    }
    
    DBG ("WhisperEngine: Extracting audio from source...");
    
    // Use AudioExtractor to get a temp file
    // Note: This extracts synchronously
    juce::File tempFile = AudioExtractor::extractToTempWAV (source, *audioCache);
    
    if (!tempFile.existsAsFile())
    {
        DBG ("WhisperEngine: Extraction failed.");
        return {};
    }
    
    if (shouldCancel)
    {
         tempFile.deleteFile();
         return {};
    }

    // Process the file
    VoxSequence result = processSync(tempFile);
    
    // Cleanup
    if (tempFile.existsAsFile())
    {
        tempFile.deleteFile();
    }
    
    return result;
}

void WhisperEngine::cancelTranscription()
{
    shouldCancel = true;
}

//==============================================================================
// Internal
void WhisperEngine::loadModel()
{
    DBG ("WhisperEngine: Loading whisper model");
    
    juce::File appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    
#if JUCE_MAC
    appData = appData.getChildFile ("Application Support");
#endif

    juce::File modelFile = appData.getChildFile ("VoxScript")
                                 .getChildFile ("models")
                                 .getChildFile ("ggml-base.en.bin");
    
    if (!modelFile.existsAsFile())
    {
        DBG ("WhisperEngine: Model not found at " + modelFile.getFullPathName());
        return;
    }
    
    whisper_context_params cparams = whisper_context_default_params();
    ctx = whisper_init_from_file_with_params (modelFile.getFullPathName().toRawUTF8(), cparams);
    
    if (ctx == nullptr)
    {
        DBG ("WhisperEngine: Failed to init whisper context.");
    }
    else
    {
        DBG ("WhisperEngine: Model loaded.");
    }
}

} // namespace VoxScript
