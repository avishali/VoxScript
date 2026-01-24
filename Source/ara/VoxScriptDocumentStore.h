/*
  ==============================================================================
    VoxScriptDocumentStore.h
    
    Persistent storage and state management for VoxScript.
    Owns the source-of-truth for all transcriptions and metadata.
    Handles stable ID mapping and ARA archive serialization.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <ARA_Library/PlugIn/ARAPlug.h>
#include <mutex>
#include <unordered_map>
#include "../transcription/VoxSequence.h"

namespace VoxScript
{

// Stable IDs for persistence
using AudioSourceID = uint64_t;
using RegionID = uint64_t;

// A snapshot of the document state for UI consumption (thread-safe, copyable)
struct DocumentSnapshot
{
    // Map of AudioSourceID -> VoxSequence
    std::unordered_map<AudioSourceID, VoxSequence> transcriptions;
    
    // Helper to get sequence for a source ID
    const VoxSequence* getSequence(AudioSourceID sourceID) const
    {
        auto it = transcriptions.find(sourceID);
        if (it != transcriptions.end())
            return &it->second;
        return nullptr;
    }
};

class VoxScriptDocumentStore
{
public:
    VoxScriptDocumentStore();
    ~VoxScriptDocumentStore() = default;

    //==============================================================================
    // ID Management
    
    /**
     * Get or create a stable ID for an ARA Audio Source.
     * Uses the host-provided persistent ID if available, otherwise generates one.
     */
    AudioSourceID getOrCreateAudioSourceID(const ARA::PlugIn::AudioSource* audioSource);
    
    /**
     * Remove an audio source from the store (e.g. when deleted from project)
     */
    void removeAudioSource(const ARA::PlugIn::AudioSource* audioSource);

    //==============================================================================
    // Data Access (Thread-safe)
    
    /**
     * Update the transcription for a specific audio source.
     */
    void updateTranscription(AudioSourceID sourceID, const VoxSequence& sequence);
    
    /**
     * Create a snapshot of the current state for the UI.
     * This is thread-safe and lock-free for the reader (after creation).
     */
    DocumentSnapshot makeSnapshot() const;
    
    //==============================================================================
    // Persistence
    
    /**
     * Serialize the entire store to a memory block for ARA archiving.
     */
    juce::MemoryBlock serialize() const;
    
    /**
     * Restore the store from archived data.
     * Rebuilds ID mappings based on persistent IDs.
     */
    bool deserialize(const void* data, size_t size);

    /**
     * Re-bind a live ARA object to its restored data using its persistent ID.
     * Called during document restoration when ARA objects are recreated.
     */
    void bindRestoredAudioSource(const ARA::PlugIn::AudioSource* audioSource);

private:
    mutable std::mutex storeMutex;
    
    // The core data: AudioSourceID -> VoxSequence
    std::unordered_map<AudioSourceID, VoxSequence> transcriptions;
    
    // Runtime mapping: ARA Pointer -> AudioSourceID
    // This is valid only for the current session lifetime
    std::unordered_map<const ARA::PlugIn::AudioSource*, AudioSourceID> runtimeParamsMap;
    
    // Persistent mapping: PersistentID String -> AudioSourceID
    // This is what strictly persists across sessions
    std::unordered_map<juce::String, AudioSourceID> persistentIdMap;
    
    // ID Generator
    AudioSourceID nextAudioSourceID = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxScriptDocumentStore)
};

} // namespace VoxScript
