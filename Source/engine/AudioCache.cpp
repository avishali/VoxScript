/*
  ==============================================================================
    AudioCache.cpp
    Created: 24 Jan 2026
    Author: VoxScript Team
  ==============================================================================
*/

#include "AudioCache.h"

namespace VoxScript
{

bool AudioCache::ensureCached (AudioCacheID id, const juce::ARAAudioSource* source)
{
    if (source == nullptr)
        return false;

    // 1. Check if already exists (Reader Lock)
    {
        const juce::ScopedReadLock rl (lock);
        if (cache.find(id) != cache.end())
            return true;
    }

    // 2. Prepare new cache entry (No Lock yet - do I/O outside lock)
    if (!source->isSampleAccessEnabled())
    {
        juce::Logger::writeToLog("AudioCache: Sample access not enabled for source");
        return false;
    }

    // Create reader locally for thread safety vs host
    auto reader = std::make_unique<juce::ARAAudioSourceReader>(const_cast<juce::ARAAudioSource*>(source));
    
    if (reader == nullptr || reader->lengthInSamples == 0)
        return false;

    auto newCache = std::make_shared<CachedAudio>();
    newCache->sampleRate = reader->sampleRate;
    newCache->numChannels = (int)reader->numChannels;
    newCache->numSamples = reader->lengthInSamples;
    
    // Allocate buffer
    newCache->buffer.setSize(newCache->numChannels, (int)newCache->numSamples);
    
    // Read audio
    if (!reader->read(&newCache->buffer, 0, (int)newCache->numSamples, 0, true, true))
    {
        juce::Logger::writeToLog("AudioCache: Failed to read from host");
        return false;
    }

    // 3. Insert into map (Writer Lock)
    {
        const juce::ScopedWriteLock wl (lock);
        cache[id] = newCache;
    }

    juce::Logger::writeToLog("AudioCache: Cached " + juce::String(newCache->numSamples) + " samples for ID " + juce::String((uintptr_t)id));
    return true;
}

std::shared_ptr<CachedAudio> AudioCache::get (AudioCacheID id) const
{
    // RT-Safe: Try to acquire read lock. If we can't, return nullptr rather than block.
    // Return by value (copy shared_ptr) to ensure lifetime safety for the caller.
    if (lock.tryEnterRead())
    {
        std::shared_ptr<CachedAudio> result;
        auto it = cache.find(id);
        if (it != cache.end())
            result = it->second;
        
        lock.exitRead();
        return result;
    }
    
    return nullptr;
}

void AudioCache::remove (AudioCacheID id)
{
    const juce::ScopedWriteLock wl (lock);
    cache.erase(id);
}

void AudioCache::clear()
{
    const juce::ScopedWriteLock wl (lock);
    cache.clear();
}

} // namespace VoxScript
