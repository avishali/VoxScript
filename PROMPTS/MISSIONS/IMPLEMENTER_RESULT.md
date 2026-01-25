
# IMPLEMENTER REPORT

**Mission ID:** MISSION 4: Deferred UI Updates
**Role:** Implementer (Antigravity)

## Changes Implemented

### Source/ara/VoxScriptDocumentController.h
- Added `std::atomic<bool> storeDirty` to track pending updates.

### Source/ara/VoxScriptDocumentController.cpp
- **`ensureTranscriptionInfraInitialised`**: Completion callback now strictly sets `storeDirty = true`. No ARA/UI calls.
- **`didAddAudioSourceToDocument`**: Checks `storeDirty`. If true, calls `notifyTranscriptionUpdated(nullptr)`.
- **`doCreatePlaybackRegion`**: Checks `storeDirty`. If true, calls `notifyTranscriptionUpdated(nullptr)`.

## Modified Files

```cpp
// FILE: Source/ara/VoxScriptDocumentController.h
/*
  ==============================================================================
    VoxScriptDocumentController.h
    
    ARA Document Controller - Central nervous system of VoxScript
    Manages the state of the entire ARA session and coordinates between
    audio sources, modifications, and the UI.
    
    Part of VoxScript Phase I: ARA Skeleton
    Fixed for JUCE 8.0+ ARA API
    Phase II: Integrated WhisperEngine for transcription
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include "../transcription/WhisperEngine.h" // Phase II
#include "../transcription/VoxSequence.h"
#include "../transcription/AudioExtractor.h" // Phase II
#include "VoxScriptDocumentStore.h"
#include "../engine/AudioCache.h" // Mission 2
#include "../engine/TranscriptionJobQueue.h" // Mission 3

namespace VoxScript
{

/**
 * @brief Main document controller for VoxScript ARA2 integration
 * 
 * This is the central manager for all ARA operations. It:
 * - Receives notifications when audio clips are added/removed in the DAW
 * - Creates and manages ARAAudioSource and ARAAudioModification objects
 * - Coordinates between the UI thread and audio processing thread
 * - Persists transcription data in the ARA archive
 * - Manages background transcription via WhisperEngine (Phase II)
 * - Uses TranscriptionJobQueue for serializing transcription tasks (Mission 3)
 * 
 * From PDF Section 3.1.1: "It receives notifications from the host when audio 
 * clips are added, removed, or modified in the DAW timeline."
 */
class VoxScriptDocumentController : public juce::ARADocumentControllerSpecialisation
{
public:
    //==========================================================================
    // Use inherited constructors (JUCE 8 pattern)
    using juce::ARADocumentControllerSpecialisation::ARADocumentControllerSpecialisation;
    
    ~VoxScriptDocumentController() override;

    //==========================================================================
    // ARA Document Controller overrides
    
    /**
     * Called after an audio source is added to the document
     * This is where we spawn the analysis thread for transcription
     */
    void didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource) override;

    /**
     * Called when a new audio source is created
     */
    juce::ARAAudioSource* doCreateAudioSource (juce::ARADocument* document,
                                               ARA::ARAAudioSourceHostRef hostRef) noexcept override;
    
    /**
     * Called when an audio source is about to be destroyed
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void doDestroyAudioSource (juce::ARAAudioSource* audioSource) noexcept;
    
    /**
     * Called when a new audio modification is created
     * This represents an "edited" state of the audio
     * FIXED: 3rd parameter is ARAAudioModification*, not ARAAudioSource*
     */
    juce::ARAAudioModification* doCreateAudioModification (juce::ARAAudioSource* audioSource,
                                                           ARA::ARAAudioModificationHostRef hostRef,
                                                           const juce::ARAAudioModification* optionalModificationToClone) noexcept override;
    
    /**
     * Called when an audio modification is about to be destroyed
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void doDestroyAudioModification (juce::ARAAudioModification* audioModification) noexcept;
    
    /**
     * Called when a playback region is created (represents a clip on the timeline)
     */
    juce::ARAPlaybackRegion* doCreatePlaybackRegion (juce::ARAAudioModification* modification,
                                                      ARA::ARAPlaybackRegionHostRef hostRef) noexcept override;
    
    /**
     * Called when a playback region is about to be destroyed
     * Note: Not virtual in JUCE 8, so no override keyword
     */
    void doDestroyPlaybackRegion (juce::ARAPlaybackRegion* playbackRegion) noexcept;
    
    /**
     * Factory method: create the playback renderer
     * This is called by JUCE/ARA when the plugin is bound to ARA
     */
    juce::ARAPlaybackRenderer* doCreatePlaybackRenderer() noexcept override;
    
    //==========================================================================
    // State persistence (for saving/loading projects)
    
    /**
     * Serialize VoxSequence data and edits into the DAW project file
     * From PDF Section 3.2: Store transcription and edits in ARA archive
     */
    bool doRestoreObjectsFromStream (juce::ARAInputStream& input,
                                     const juce::ARARestoreObjectsFilter* filter) noexcept override;
    
    /**
     * Deserialize VoxSequence data when loading a project
     */
    bool doStoreObjectsToStream (juce::ARAOutputStream& output,
                                const juce::ARAStoreObjectsFilter* filter) noexcept override;
    
    //==========================================================================
    // Custom VoxScript API (for communication with UI)
    
    /**
     * Notify the UI that transcription has been updated
     * Called from the background analysis thread
     */
    void notifyTranscriptionUpdated (juce::ARAAudioSource* source);
    
    /**
     * Register a listener for transcription updates
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void transcriptionUpdated (juce::ARAAudioSource* source) = 0;
    };
    
    void addListener (Listener* listener);
    void removeListener (Listener* listener);
    
    //==========================================================================
    // WhisperEngine::Listener overrides (Note: Removed override keyword to fix build if inheritance is missing)
    void transcriptionProgress (float progress);
    void transcriptionComplete (VoxSequence sequence);
    void transcriptionFailed (const juce::String& error);

    //==========================================================================
    // Phase II: Transcription API
    
    /** Get the current transcription result. */
    const VoxSequence& getTranscription() const { return currentTranscription; }
    
    /** Get the current transcription status message. */
    juce::String getTranscriptionStatus() const { return transcriptionStatus; }

    /** Accessor for the WhisperEngine instance (Phase III) */
    WhisperEngine& getWhisperEngine() { return whisperEngine; }

    /** Accessor for the Document Store (Mission 1) */
    VoxScriptDocumentStore& getStore() { return documentStore; }

    /** Accessor for the Audio Cache (Mission 2) */
    AudioCache& getAudioCache() { return audioCache; }
    
    /**
     * @brief Enqueue a transcription job for the given source.
     * Thread-safe. Called by VoxScriptAudioSource or internally.
     */
    void enqueueTranscriptionForSource(juce::ARAAudioSource* source);

    //==========================================================================
    // Mission 4: Crash Prevention
    /** 
     * Check if it is safe to perform background work or queue jobs.
     * This gates callbacks that might occur before the controller is fully ready.
     */
    bool isAraReadyForBackgroundWork() const noexcept { return araReadyForBackgroundWork.load(); }

    private:
    void ensureTranscriptionInfraInitialised();

    //==========================================================================
    juce::ListenerList<Listener> listeners;
    
    // Mission 1: Document Persistence
    VoxScriptDocumentStore documentStore;

    // Mission 2: Audio Cache
    AudioCache audioCache;
    
    // Mission 3: Transcription Job Queue
    TranscriptionJobQueue jobQueue;
    std::atomic<bool> transcriptionInfraInitialised { false };
    std::shared_ptr<std::atomic<bool>> controllerAlive;
    
    // Mission 4: Readiness Flag
    std::atomic<bool> araReadyForBackgroundWork { false };
    std::atomic<bool> storeDirty { false };

    // Phase II/III: Transcription and Audio Extraction
    WhisperEngine whisperEngine;
    VoxSequence currentTranscription;
    juce::String transcriptionStatus = "Idle";
    juce::ARAAudioSource* currentAudioSource = nullptr;  // Phase III: Track for sample access cleanup
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptDocumentController)
};

} // namespace VoxScript
```

```cpp
// FILE: Source/ara/VoxScriptDocumentController.cpp
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
```

**Implementer Stopped.**
