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
// Constructor removed: using base class constructor (lazy init)

VoxScriptDocumentController::~VoxScriptDocumentController()
{
    if (controllerAlive)
        controllerAlive->store(false);

    DBG ("================================================");
    DBG ("VOXSCRIPT: Document Controller DESTROYED");
    DBG ("================================================");
}

void VoxScriptDocumentController::ensureTranscriptionInfraInitialised()
{
    // If already initialised, return immediately (fast path)
    if (transcriptionInfraInitialised.load())
        return;

    if (!controllerAlive)
        controllerAlive = std::make_shared<std::atomic<bool>>(true);

    // Use exchange to ensure only one thread performs initialization
    // (though in ARA, these calls are usually main thread, but safety first)
    if (transcriptionInfraInitialised.exchange(true))
        return;

    DBG ("VoxScriptDocumentController: Initialising Transcription Infrastructure (Lazy)");

    // Mission 3: Initialize TranscriptionJobQueue
    jobQueue.initialise(&documentStore);
    
    auto alive = controllerAlive;

    // Set callback to notify UI on completion
    // Mission 4: DEFERRED UPDATE PATTERN
    // We do NOT call notifyTranscriptionUpdated() here because it touches ARA objects.
    // We just mark the store as dirty. The update will happen in the next safe ARA call.
    jobQueue.setCompletionCallback([alive, this](AudioSourceID id) {
        if (!alive || !alive->load())
            return;
             
        storeDirty.store(true);
    });
    
    // Mission 2: Audio Cache configured (WhisperEngine setup removed as it is now internal to worker)
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
    
    ensureTranscriptionInfraInitialised();
    
    // Mission 3: Enqueue transcription immediately if possible
    // Note: Audio source usually doesn't have sample access enabled yet creates.
    // However, if it does, we can queue it.
    
    if (audioSource->isSampleAccessEnabled())
    {
        AudioSourceID id = documentStore.getOrCreateAudioSourceID(audioSource);
        
        // Extract audio to temp file immediately
        // Note: This blocks momentarily but ensures we have a file before background job starts
        // or we could push this to the background too? 
        // Mission says: "extract to temp WAV immediately ... enqueue job with audioFile only"
        
        // Ensure availability in cache first (optional, but good for other parts of app)
        audioCache.ensureCached(audioSource, audioSource);
        
        juce::File jobFile = AudioExtractor::extractToTempWAV(audioSource, audioCache);
        
        if (jobFile.existsAsFile())
        {
            TranscriptionJob job;
            job.sourceID = id;
            job.audioFile = jobFile;
            
            DBG ("VoxScriptDocumentController: Enqueuing initial transcription for source " + juce::String(id));
            jobQueue.enqueueTranscription(job);
        }
    }
    
    // Mission 4: Signal that we are ready for background work
    // We only enable this after the source is fully added and infra is ready.
    araReadyForBackgroundWork.store(true);

    // Mission 4: Check for deferred updates
    if (storeDirty.exchange(false))
        notifyTranscriptionUpdated(nullptr);
}

void VoxScriptDocumentController::doDestroyAudioSource (
    juce::ARAAudioSource* audioSource) noexcept
{
    DBG ("VoxScriptDocumentController: Destroying audio source");
    
    // Mission: Safe teardown without creating new IDs
    auto idOpt = documentStore.findAudioSourceID(audioSource);
    
    if (idOpt.has_value())
    {
        AudioSourceID id = *idOpt;
        
        // Cancel pending jobs
        jobQueue.cancelForAudioSource(id);
    
        // Remove from cache (by pointer, which is what the cache expects)
        audioCache.remove(audioSource);
        
        // Remove from store by ID and cleanup mapping
        documentStore.removeAudioSourceByID(id);
    }
    else
    {
        // If not found, it might be a partial create or already gone. 
        // Just delete the object.
    }
    
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
    
    ensureTranscriptionInfraInitialised();

    auto* audioSource = modification->getAudioSource();
    if (audioSource)
    {
        AudioSourceID id = documentStore.getOrCreateAudioSourceID(audioSource);
        
        // Check if we have transcription
        const auto* sequence = documentStore.makeSnapshot().getSequence(id);
        
        if (sequence == nullptr || sequence->getWordCount() == 0)
        {
             if (audioSource->isSampleAccessEnabled())
             {
                 // Delegate to main enqueue method to reuse safe extraction logic
                 enqueueTranscriptionForSource(audioSource);
             }
        }
    }
    
    // Mission 4: Check for deferred updates
    if (storeDirty.exchange(false))
        notifyTranscriptionUpdated(nullptr);

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
    
    ensureTranscriptionInfraInitialised();

    // Check sample access
    if (!source->isSampleAccessEnabled())
    {
        DBG ("VoxScriptDocumentController: Warning - enqueue requested but sample access disabled");
        return;
    }
    
    AudioSourceID id = documentStore.getOrCreateAudioSourceID(source);
    
    // Ensure caching (good practice)
    audioCache.ensureCached(source, source);
    
    // Extract to temp WAV immediately
    juce::File jobFile = AudioExtractor::extractToTempWAV(source, audioCache);
    
    if (jobFile.existsAsFile())
    {
        TranscriptionJob job;
        job.sourceID = id;
        job.audioFile = jobFile;
        
        DBG ("VoxScriptDocumentController: Enqueuing transcription request (safe file) for source " + juce::String(id));
        jobQueue.enqueueTranscription(job);
    }
    else
    {
         DBG("VoxScriptDocumentController: Failed to create temp job file");
    }
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
