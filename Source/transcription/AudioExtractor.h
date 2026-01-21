/*
  ==============================================================================
    AudioExtractor.h
    
    Extracts audio from ARA persistent sample access to temporary WAV file
    optimized for whisper.cpp transcription (16kHz mono)
    
    Part of VoxScript Phase III: Automatic Transcription
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace VoxScript
{

/**
 * @brief Extracts audio from ARA persistent sample access to a temporary WAV file
 * optimized for whisper.cpp transcription (16kHz mono).
 * 
 * This class handles the conversion of ARA audio sources to whisper-ready format:
 * - Downmixes multi-channel audio to mono (average all channels)
 * - Resamples to 16kHz (whisper.cpp requirement)
 * - Writes to temporary 16-bit PCM WAV file
 * - Thread-safe using local ARAAudioSourceReader instances
 * 
 * Performance optimizations:
 * - Downmix BEFORE resample (50% CPU savings)
 * - Chunk-based processing (8192 samples per chunk)
 * - Lagrange interpolation for resampling (good speed/quality balance)
 */
class AudioExtractor
{
public:
    AudioExtractor();
    ~AudioExtractor();
    
    /**
     * Extract audio from an ARA audio source to a temporary WAV file.
     * 
     * @param araAudioSource The ARA audio source to extract from
     * @param prefix Optional prefix for temp filename (default: "voxscript_")
     * @return Temporary WAV file, or invalid File() on error
     * 
     * Output format: 16kHz mono, 16-bit PCM WAV (optimized for whisper.cpp)
     * 
     * Thread safety: Safe to call from any thread. Creates local ARAAudioSourceReader
     * to avoid sharing reader instances across threads (prevents Steinberg glitches).
     * 
     * Lifecycle: Caller is responsible for deleting the temp file after use.
     */
    juce::File extractToTempWAV(juce::ARAAudioSource* araAudioSource, 
                                const juce::String& prefix = "voxscript_");
    
private:
    juce::File getTempDirectory();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExtractor)
};

} // namespace VoxScript
