/*
  ==============================================================================
    AudioExtractor.h
    Created: 21 Jan 2026
    Author: VoxScript Team (with Gemini 2.0 Flash research)
    
    Purpose: Extract audio from ARA persistent storage to temporary files
    suitable for Whisper transcription (16kHz, Mono, 16-bit PCM).
    
    Architecture Notes:
    - Uses juce::ARAAudioSourceReader for ARA sample access
    - Creates reader in local scope for Steinberg thread safety
    - Downmixes to mono BEFORE resampling (50% CPU savings)
    - Uses Lagrange interpolation (optimal for speech)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

namespace VoxScript
{

/**
 * @brief Utility class for extracting audio from ARA sources
 * 
 * Handles conversion from any ARA audio source format to Whisper-compatible
 * 16kHz mono WAV files. Thread-safe and optimized for speech processing.
 * 
 * Thread Safety:
 * - Must be called from a background thread (NOT message thread)
 * - Creates ARAAudioSourceReader locally to satisfy Steinberg hosts
 * 
 * Performance:
 * - Downmixes to mono before resampling (optimal CPU usage)
 * - Chunk-based processing (4096 samples) for memory efficiency
 * - Lagrange interpolation for speed/quality balance
 */
class AudioExtractor
{
public:
    //==========================================================================
    /**
     * Extract audio from ARA source and convert to 16kHz mono WAV file
     * 
     * @param araSource       The ARA audio source to read from
     * @param tempFilePrefix  Prefix for temp filename (default: "voxscript_")
     * @return                Valid File on success, invalid File() on failure
     * 
     * @note This function is BLOCKING - call from background thread only
     * @note Creates ARAAudioSourceReader internally for thread-locality
     * @note Checks isSampleAccessEnabled() before attempting extraction
     * @note Automatically cleans up temp file on extraction failure
     */
    static juce::File extractToTempWAV (juce::ARAAudioSource* araSource, 
                                        const juce::String& tempFilePrefix = "voxscript_");
    
    //==========================================================================
    /**
     * Check if sample access is available for the given ARA source
     */
    static bool isSampleAccessAvailable (juce::ARAAudioSource* araSource);
    
    //==========================================================================
    /**
     * Get the expected output size (in samples) for the extracted file
     */
    static juce::int64 getExpectedOutputSize (juce::ARAAudioSource* araSource);

private:
    //==========================================================================
    /**
     * Generate unique temp file path in system temp directory
     */
    static juce::File getUniqueTempFile (const juce::String& prefix);
    
    // Constants for audio processing
    static constexpr double TARGET_SAMPLE_RATE = 16000.0;
    static constexpr int TARGET_CHANNELS = 1;
    static constexpr int TARGET_BIT_DEPTH = 16;
    static constexpr int CHUNK_SIZE = 4096;
    
    // Prevent instantiation - static utility class only
    AudioExtractor() = delete;
    ~AudioExtractor() = delete;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioExtractor)
};

} // namespace VoxScript
