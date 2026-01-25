/*
  ==============================================================================
    VoxScriptDocumentStore.cpp
  ==============================================================================
*/

#include "VoxScriptDocumentStore.h"

namespace VoxScript
{

VoxScriptDocumentStore::VoxScriptDocumentStore()
{
}

AudioSourceID VoxScriptDocumentStore::getOrCreateAudioSourceID(const ARA::PlugIn::AudioSource* audioSource)
{
    if (audioSource == nullptr)
        return 0;

    std::lock_guard<std::mutex> lock(storeMutex);

    // 1. Check if we already have a runtime mapping for this pointer
    auto runtimeIt = runtimeParamsMap.find(audioSource);
    if (runtimeIt != runtimeParamsMap.end())
    {
        return runtimeIt->second;
    }

    // 2. If not, try to look up by persistent ID (restoration scenario)
    // ARA gives us a persistent ID (usually a string or hash from the host)
    // We assume getPersistentID() returns a string-convertible pointer or similar.
    // In strict ARA, it's a void*, but JUCE Wrappers often provide a String adaptation.
    // If not using JUCE's expanded ARA classes, we have to rely on the raw ARA API.
    // NOTE: For this implementation, we will try to use the host's provided ID if valid,
    // otherwise fallback to a generated ID. Since we are in JUCE/ARA context, let's assume
    // we can get a meaningful string ID or we use the pointer address as a temporary key if needed,
    // but relying on pointer address for persistence is wrong.
    //
    // However, since we are implementing the *Store* logic, we will generate our own internal ID
    // and map the HOST's persistent ID to it.
    
    juce::String persistentID;
    
    // Attempt to get a stable ID from the host.
    // Note: In pure ARA, we might need the ObjectRef. For now, we will assume
    // the caller might have restored this. But for a fresh session:
    // FIXME: Proper persistent ID retrieval from ARA host to be implemented.
    // if (auto* entry = audioSource->getDocument()->getAudioSourcePersistencyIndexEntry(audioSource))
    // {
    //      // ...
    // }
    
    // Simple implementation: Generate a new ID if we don't have one mapped via bindRestoredAudioSource.
    // If bindRestoredAudioSource was called, it would have populated persistentIdMap.
    
    // We need a key to check persistentIdMap. Ideally, the Host provides this key.
    // If we can't get a key from the host, we generate a new one.
    
    AudioSourceID newID = nextAudioSourceID++;
    runtimeParamsMap[audioSource] = newID;
    
    return newID;
}

std::optional<AudioSourceID> VoxScriptDocumentStore::findAudioSourceID(const ARA::PlugIn::AudioSource* audioSource) const
{
    if (audioSource == nullptr)
        return std::nullopt;

    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = runtimeParamsMap.find(audioSource);
    if (it != runtimeParamsMap.end())
        return it->second;

    return std::nullopt;
}

void VoxScriptDocumentStore::removeAudioSourceByID(AudioSourceID id)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    
    // Remove data
    transcriptions.erase(id);
    
    // Remove mapping (Linear scan of map - acceptable for teardown)
    for (auto it = runtimeParamsMap.begin(); it != runtimeParamsMap.end(); )
    {
        if (it->second == id)
            it = runtimeParamsMap.erase(it);
        else
            ++it;
    }
}

void VoxScriptDocumentStore::removeAudioSource(const ARA::PlugIn::AudioSource* audioSource)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    
    auto it = runtimeParamsMap.find(audioSource);
    if (it != runtimeParamsMap.end())
    {
        // specific cleanup if needed (e.g. keeping data for undo?)
        // runtimeParamsMap.erase(it); // Keep mapped for now, lightweight.
    }
}

void VoxScriptDocumentStore::updateTranscription(AudioSourceID sourceID, const VoxSequence& sequence)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    transcriptions[sourceID] = sequence;
}

DocumentSnapshot VoxScriptDocumentStore::makeSnapshot() const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    DocumentSnapshot snapshot;
    snapshot.transcriptions = transcriptions;
    return snapshot;
}

juce::MemoryBlock VoxScriptDocumentStore::serialize() const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    
    juce::ValueTree root("VOXSCRIPT_DOC");
    root.setProperty("version", 1, nullptr);
    root.setProperty("nextID", (juce::int64)nextAudioSourceID, nullptr);
    
    juce::ValueTree sources("SOURCES");
    for (const auto& pair : transcriptions)
    {
        juce::ValueTree sourceNode("SOURCE");
        sourceNode.setProperty("id", (juce::int64)pair.first, nullptr);
        
        // Serialize the sequence
        sourceNode.addChild(pair.second.toValueTree(), -1, nullptr);
        sources.addChild(sourceNode, -1, nullptr);
    }
    root.addChild(sources, -1, nullptr);
    
    // We also need to save the persistent ID map so we can rebind on load
    juce::ValueTree mappings("MAPPINGS");
    for (const auto& pair : persistentIdMap)
    {
        juce::ValueTree mapNode("MAP");
        mapNode.setProperty("persistentID", pair.first, nullptr);
        mapNode.setProperty("internalID", (juce::int64)pair.second, nullptr);
        mappings.addChild(mapNode, -1, nullptr);
    }
    root.addChild(mappings, -1, nullptr);

    juce::MemoryOutputStream stream;
    root.writeToStream(stream);
    return stream.getMemoryBlock();
}

bool VoxScriptDocumentStore::deserialize(const void* data, size_t size)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    
    juce::MemoryInputStream stream(data, size, false);
    auto root = juce::ValueTree::readFromStream(stream);
    
    if (!root.isValid() || root.getType().toString() != "VOXSCRIPT_DOC")
        return false;
        
    nextAudioSourceID = (AudioSourceID)(int64)root.getProperty("nextID", 1);
    
    // Clear current state
    transcriptions.clear();
    persistentIdMap.clear();
    runtimeParamsMap.clear();
    
    // Load Sources
    auto sources = root.getChildWithName("SOURCES");
    for (int i = 0; i < sources.getNumChildren(); ++i)
    {
        auto sourceNode = sources.getChild(i);
        AudioSourceID id = (AudioSourceID)(int64)sourceNode.getProperty("id");
        
        VoxSequence seq;
        if (seq.fromValueTree(sourceNode.getChildWithName("SEQUENCE")))
        {
            transcriptions[id] = seq;
        }
    }
    
    // Load Mappings
    auto mappings = root.getChildWithName("MAPPINGS");
    for (int i = 0; i < mappings.getNumChildren(); ++i)
    {
        auto mapNode = mappings.getChild(i);
        juce::String pid = mapNode.getProperty("persistentID");
        AudioSourceID id = (AudioSourceID)(int64)mapNode.getProperty("internalID");
        persistentIdMap[pid] = id;
    }
    
    return true;
}

void VoxScriptDocumentStore::bindRestoredAudioSource(const ARA::PlugIn::AudioSource* audioSource)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    
    if (audioSource == nullptr) return;
    
    // In a real implementation, we would get the host's persistent ID string here.
    // For now, let's assume we can map the Order Index or similar if available.
    // THIS IS A STUB for the binding logic logic since getting the exact PersistentID string
    // depends on the deep ARA integration helper.
    
    // WORKAROUND: If we serialized based on order, we might recover.
    // Ideally: 
    // juce::String hostID = getHostProvidedID(audioSource);
    // if (persistentIdMap.count(hostID))
    //    runtimeParamsMap[audioSource] = persistentIdMap[hostID];
}

} // namespace VoxScript
