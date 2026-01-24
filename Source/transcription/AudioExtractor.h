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

#include <JuceHeader.h>

namespace VoxScript
{

class AudioCache;

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
     * 
     * @example
     * @code
     * // On background thread:
     * auto tempFile = AudioExtractor::extractToTempWAV(araSource);
     * if (tempFile.existsAsFile()) {
     *     whisperEngine.transcribeAudioFile(tempFile);
     * }
     * @endcode
     */
    static juce::File extractToTempWAV (juce::ARAAudioSource* araSource, 
                                        AudioCache& audioCache,
                                        const juce::String& tempFilePrefix = "voxscript_");
    
    //==========================================================================
    /**
     * Check if sample access is available for the given ARA source
     * 
     * @param araSource  The ARA audio source to check
     * @return           true if samples can be read, false otherwise
     * 
     * @note Useful for pre-checking before spawning extraction thread
     */
    static bool isSampleAccessAvailable (juce::ARAAudioSource* araSource);
    
    //==========================================================================
    /**
     * Get the expected output size (in samples) for the extracted file
     * 
     * @param araSource  The ARA audio source to check
     * @return           Number of samples at 16kHz, or 0 on error
     * 
     * @note Useful for progress estimation and memory planning
     */
    static int64 getExpectedOutputSize (juce::ARAAudioSource* araSource);

private:
    //==========================================================================
    /**
     * Generate unique temp file path in system temp directory
     * 
     * @param prefix  Filename prefix
     * @return        Unique File with .wav extension
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
