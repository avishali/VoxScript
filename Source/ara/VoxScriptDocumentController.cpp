/*
  ==============================================================================
    VoxScriptDocumentController.cpp
    
    Implementation of the ARA Document Controller
    Fixed for JUCE 8.0+ ARA API
    Phase II: Integrated WhisperEngine for transcription
    Mission 3: Transcription Job Queue Integration
  ==============================================================================
*/

#include "VoxScriptDocumentController.h"
#include "VoxScriptAudioSource.h"
#include "VoxScriptPlaybackRenderer.h"
#include "../util/VoxLogger.h"

namespace VoxScript
{

//==============================================================================
// Construction
//==============================================================================

// Explicit constructor implementation for JUCE 8 ARA
// (Replaces 'using' to allow initialization of jobQueue)
// Note: We assume standard ARADocumentControllerSpecialisation signature.
// If this fails to compile due to signature mismatch, check JUCE ARA docs.
// Explicit constructor implementation for JUCE 8 ARA
// (Replaces 'using' to allow initialization of jobQueue)
// Note: We assume standard ARADocumentControllerSpecialisation signature.
VoxScriptDocumentController::VoxScriptDocumentController(const ARA::PlugIn::PlugInEntry* entry, 
                                                         const ARA::PlugIn::DocumentControllerInstance* instance)
    : juce::ARADocumentControllerSpecialisation(entry, instance)
{
    // Mission 3: Initialize TranscriptionJobQueue
    jobQueue.initialise(&documentStore, &whisperEngine);
    
    // Set callback to notify UI on completion
    jobQueue.setCompletionCallback([this](AudioSourceID id) { 
        // Notify all listeners that something changed
        // Ideally we would pass the specific source if we could map ID->Source safely
        notifyTranscriptionUpdated(nullptr); 
    });
    
    // Mission 2: Audio Cache
    whisperEngine.setAudioCache(&audioCache);
}

VoxScriptDocumentController::~VoxScriptDocumentController()
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Document Controller DESTROYED");
    DBG ("================================================");
}

//==============================================================================
// Audio Source Management
//==============================================================================

juce::ARAAudioSource* VoxScriptDocumentController::doCreateAudioSource (
    juce::ARADocument* document,
    ARA::ARAAudioSourceHostRef hostRef) noexcept
{
    DBG ("VOXSCRIPT: Creating Audio Source");
    
    // Create our custom audio source wrapper
    return new VoxScriptAudioSource (document, hostRef);
}

void VoxScriptDocumentController::didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource)
{
    juce::ignoreUnused (document);
    DBG ("VoxScriptDocumentController::didAddAudioSourceToDocument called");
    
    // Mission 3: Enqueue transcription immediately if possible
    // Note: Audio source usually doesn't have sample access enabled yet creates.
    // However, if it does, we can queue it.
    
    if (audioSource->isSampleAccessEnabled())
    {
        AudioSourceID id = documentStore.getOrCreateAudioSourceID(audioSource);
        
        TranscriptionJob job;
        job.sourceID = id;
        job.sourcePtr = audioSource; // Pass pointer for extraction
        
        DBG ("VoxScriptDocumentController: Enqueuing initial transcription for source " + juce::String(id));
        jobQueue.enqueueTranscription(job);
    }
}

void VoxScriptDocumentController::doDestroyAudioSource (
    juce::ARAAudioSource* audioSource) noexcept
{
    DBG ("VoxScriptDocumentController: Destroying audio source");
    
    // Cancel any pending jobs for this source
    // We need the ID. Since source is about to die, we can still look it up?
    // The Store might still have it.
    AudioSourceID id = documentStore.getOrCreateAudioSourceID(audioSource); // Lookup
    jobQueue.cancelForAudioSource(id);
    
    // Remove from cache and store
    audioCache.remove(audioSource);
    documentStore.removeAudioSource(audioSource); // Cleanup store mapping
    
    delete audioSource;
}

//==============================================================================
// Audio Modification Management
//==============================================================================

juce::ARAAudioModification* VoxScriptDocumentController::doCreateAudioModification (
    juce::ARAAudioSource* audioSource,
    ARA::ARAAudioModificationHostRef hostRef,
    const juce::ARAAudioModification* optionalModificationToClone) noexcept
{
    DBG ("VOXSCRIPT: Creating Audio Modification for source: " + juce::String (audioSource->getName()));
    
    // VoxScriptAudioSource::notifyPropertiesUpdated might trigger updates too
    
    return new juce::ARAAudioModification (audioSource, hostRef, optionalModificationToClone);
}

void VoxScriptDocumentController::doDestroyAudioModification (
    juce::ARAAudioModification* audioModification) noexcept
{
    DBG ("VoxScriptDocumentController: Destroying audio modification");
    delete audioModification;
}

//==============================================================================
// Playback Region Management
//==============================================================================

juce::ARAPlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion (
    juce::ARAAudioModification* modification,
    ARA::ARAPlaybackRegionHostRef hostRef) noexcept
{
    // ... logging omitted ...
    
    auto* audioSource = modification->getAudioSource();
    if (audioSource)
    {
        AudioSourceID id = documentStore.getOrCreateAudioSourceID(audioSource);
        
        // Check if we have transcription
        const auto* sequence = documentStore.makeSnapshot().getSequence(id);
        
        if (sequence == nullptr || sequence->getSegmentCount() == 0)
        {
             if (audioSource->isSampleAccessEnabled())
             {
                 TranscriptionJob job;
                 job.sourceID = id;
                 job.sourcePtr = audioSource;
                 DBG ("VoxScriptDocumentController: Enqueuing transcription (via PlaybackRegion creation)");
                 jobQueue.enqueueTranscription(job);
             }
        }
    }
    
    return new juce::ARAPlaybackRegion (modification, hostRef);
}

void VoxScriptDocumentController::doDestroyPlaybackRegion (
    juce::ARAPlaybackRegion* playbackRegion) noexcept
{
    delete playbackRegion;
}

juce::ARAPlaybackRenderer* VoxScriptDocumentController::doCreatePlaybackRenderer() noexcept
{
    return new VoxScriptPlaybackRenderer (getDocumentController());
}

//==============================================================================
// State Persistence
//==============================================================================

bool VoxScriptDocumentController::doRestoreObjectsFromStream (juce::ARAInputStream& input,
                                                              const juce::ARARestoreObjectsFilter* filter) noexcept
{
    // Read the entire stream into a memory block
    juce::MemoryBlock data;
    int64 bytesLeft = input.getTotalLength() - input.getPosition();
    if (bytesLeft > 0)
    {
        data.setSize((size_t)bytesLeft);
        if (!input.read(data.getData(), (size_t)bytesLeft))
            return false;
            
        // Deserialize the store
        if (documentStore.deserialize(data.getData(), data.getSize()))
        {
            DBG ("VOXSCRIPT: Document store deserialized successfully.");
            return true;
        }
    }
    
    juce::ignoreUnused (filter);
    return false;
}

bool VoxScriptDocumentController::doStoreObjectsToStream (juce::ARAOutputStream& output,
                                                            const juce::ARAStoreObjectsFilter* filter) noexcept
{
    // Serialize the store
    juce::MemoryBlock data = documentStore.serialize();
    juce::ignoreUnused (filter);
    return output.write(data.getData(), data.getSize());
}

//==============================================================================
// Custom VoxScript API
//==============================================================================

void VoxScriptDocumentController::notifyTranscriptionUpdated (juce::ARAAudioSource* source)
{
    // Notify all listeners
    listeners.call ([source] (Listener& l) { l.transcriptionUpdated (source); });
}

void VoxScriptDocumentController::enqueueTranscriptionForSource(juce::ARAAudioSource* source)
{
    if (source == nullptr) return;
    
    // Check sample access
    if (!source->isSampleAccessEnabled())
    {
        DBG ("VoxScriptDocumentController: Warning - enqueue requested but sample access disabled");
        return;
    }
    
    AudioSourceID id = documentStore.getOrCreateAudioSourceID(source);
    
    TranscriptionJob job;
    job.sourceID = id;
    job.sourcePtr = source;
    
    DBG ("VoxScriptDocumentController: Enqueuing transcription request for source " + juce::String(id));
    jobQueue.enqueueTranscription(job);
}

void VoxScriptDocumentController::addListener (Listener* listener)
{
    listeners.add (listener);
    
    // Mission 3: Late initialization hack
    // Since we avoid complex constructor overriding, we can lazy-init the specific components here
    // But better to just assume jobQueue is ready (default constructor).
    // We just need to ensure initialise() is called.
    // Initialization check?
}

void VoxScriptDocumentController::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

} // namespace VoxScript
