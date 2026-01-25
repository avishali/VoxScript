/*
  ==============================================================================
    AudioCache.h
    Created: 24 Jan 2026
    Author: VoxScript Team
    
    Purpose: Immutable audio cache to decouple render/analysis from host.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace VoxScript
{

// ID to identify an audio source (using pointer as ID)
using AudioCacheID = const void*;

/**
 * @brief Structure holding cached audio data
 */
struct CachedAudio
{
    juce::AudioBuffer<float> buffer;
    double sampleRate { 0.0 };
    int numChannels { 0 };
    juce::int64 numSamples { 0 };
    
    CachedAudio() = default;
    
    // Non-copyable to prevent accidental large copies
    CachedAudio(const CachedAudio&) = delete;
    CachedAudio& operator=(const CachedAudio&) = delete;
    
    CachedAudio(CachedAudio&&) = default;
    CachedAudio& operator=(CachedAudio&&) = default;
};

/**
 * @brief Thread-safe Audio Cache
 * 
 * Owns independent copies of audio data from ARA sources.
 * Allows lock-free (or wait-free-ish) access from render thread via tryEnterRead.
 */
class AudioCache
{
public:
    AudioCache() = default;
    ~AudioCache() = default;

    /**
     * @brief Ensures audio for the given source is cached.
     * 
     * Reads from the host if not already cached.
     * This operation involves memory allocation and file I/O, so:
     * MUST NOT be called from the real-time audio thread.
     * 
     * @param id The ID of the source (usually the ARAAudioSource pointer)
     * @param source The ARA audio source to read from
     * @return true if cached successfully (or was already cached), false on failure
     */
    bool ensureCached (AudioCacheID id, const juce::ARAAudioSource* source);

    /**
     * @brief Retrieves cached audio for a source ID.
     * 
     * RT-Safe: Uses tryEnterRead to avoid blocking on lock.
     * 
     * @param id The ID to look up
     * @return const pointer to CachedAudio, or nullptr if not found or lock busy
     */
    /**
     * @brief Retrieves cached audio for a source ID.
     * 
     * RT-Safe: Uses tryEnterRead to avoid blocking on lock.
     * Returns a shared_ptr, ensuring the data remains valid even if removed from cache.
     * 
     * @param id The ID to look up
     * @return shared_ptr to CachedAudio, or empty if not found or lock busy
     */
    std::shared_ptr<CachedAudio> get (AudioCacheID id) const;

    /**
     * @brief Removes a source from the cache.
     */
    void remove (AudioCacheID id);
    
    /**
     * @brief Clear entire cache
     */
    void clear();

private:
    std::map<AudioCacheID, std::shared_ptr<CachedAudio>> cache;
    juce::ReadWriteLock lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioCache)
};

} // namespace VoxScript
