/*
  ==============================================================================
    VoxScriptDocumentController.cpp
    
    Implementation of the ARA Document Controller
    Fixed for JUCE 8.0+ ARA API
    Phase II: Integrated WhisperEngine for transcription
  ==============================================================================
*/

#include "VoxScriptDocumentController.h"
#include "VoxScriptAudioSource.h"
#include "VoxScriptPlaybackRenderer.h"
#include "../util/VoxLogger.h"

namespace VoxScript
{

VoxScriptDocumentController::~VoxScriptDocumentController()
{
    whisperEngine.removeListener (this);
    
    DBG ("================================================");
    DBG ("VOXSCRIPT: Document Controller DESTROYED");
    DBG ("================================================");
}

//==============================================================================
// Audio Source Management

juce::ARAAudioSource* VoxScriptDocumentController::doCreateAudioSource (
    juce::ARADocument* document,
    ARA::ARAAudioSourceHostRef hostRef) noexcept
{
    DBG ("VOXSCRIPT: Creating Audio Source");
    
    // Create our custom audio source wrapper
    auto* audioSource = new VoxScriptAudioSource (document, hostRef);
    
    // Register as listener for transcription events
    whisperEngine.addListener (this);
    
    // Mission 2: Ensure WhisperEngine has access to AudioCache
    whisperEngine.setAudioCache(&audioCache);
    
    return audioSource;
}

void VoxScriptDocumentController::didAddAudioSourceToDocument (juce::ARADocument* document, juce::ARAAudioSource* audioSource)
{
    juce::ignoreUnused (document, audioSource);
    DBG ("VoxScriptDocumentController::didAddAudioSourceToDocument called");
    
    // NOTE: Don't extract audio here - source not fully initialized yet!
    // Audio source may not have sample access enabled at this point.
    // Extraction will happen in doCreateAudioModification() instead.
}

void VoxScriptDocumentController::doDestroyAudioSource (
    juce::ARAAudioSource* audioSource) noexcept
{
    DBG ("VoxScriptDocumentController: Destroying audio source");
    audioCache.remove(audioSource);
    delete audioSource;
}

//==============================================================================
// Audio Modification Management

juce::ARAAudioModification* VoxScriptDocumentController::doCreateAudioModification (
    juce::ARAAudioSource* audioSource,
    ARA::ARAAudioModificationHostRef hostRef,
    const juce::ARAAudioModification* optionalModificationToClone) noexcept
{
    DBG ("VOXSCRIPT: Creating Audio Modification for source: " + juce::String (audioSource->getName()));
    
    // Phase III: Audio extraction is handled by VoxScriptAudioSource::notifyPropertiesUpdated()
    // which is called automatically by ARA when the audio source properties are finalized.
    // This ensures sample access is available before extraction begins.
    
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

juce::ARAPlaybackRegion* VoxScriptDocumentController::doCreatePlaybackRegion (
    juce::ARAAudioModification* modification,
    ARA::ARAPlaybackRegionHostRef hostRef) noexcept
{
    VOXLOG("========================================");
    VOXLOG("DOCUMENT CONTROLLER: Creating Playback Region");
    VOXLOG("========================================");
    
    DBG ("========================================");
    DBG ("VoxScriptDocumentController: Creating Playback Region");
    DBG ("========================================");
    
    // Phase III: Trigger transcription when playback region is created
    // (when audio is actually placed on the timeline)
    
    auto* audioSource = modification->getAudioSource();
    if (audioSource)
    {
        VOXLOG("Audio Source: " + juce::String::toHexString((juce::pointer_sized_int)audioSource));
        VOXLOG("Sample Access Enabled: " + juce::String(audioSource->isSampleAccessEnabled() ? "YES" : "NO"));
        
        DBG ("  Audio Source: " + juce::String::toHexString((juce::pointer_sized_int)audioSource));
        DBG ("  Sample Access Enabled: " + juce::String(audioSource->isSampleAccessEnabled() ? "YES" : "NO"));
        
        auto* voxSource = dynamic_cast<VoxScriptAudioSource*>(audioSource);
        if (voxSource && !voxSource->isTranscriptionReady() && audioSource->isSampleAccessEnabled())
        {
            VOXLOG("Transcription NOT ready yet - triggering transcription now");
            VOXLOG("Passing controller pointer directly to avoid dynamic_cast issue");
            DBG ("VoxScriptDocumentController: Triggering transcription for new playback region");
            
            // Trigger transcription (will start its own background thread internally)
            voxSource->triggerTranscriptionWithController(this);
        }
        else if (voxSource && voxSource->isTranscriptionReady())
        {
            VOXLOG("Transcription already ready - skipping");
        }
        else if (!audioSource->isSampleAccessEnabled())
        {
            VOXLOG("Sample access NOT enabled - cannot transcribe yet");
        }
    }
    else
    {
        VOXLOG("ERROR: No audio source in modification!");
    }
    
    VOXLOG("========================================");
    
    return new juce::ARAPlaybackRegion (modification, hostRef);
}

void VoxScriptDocumentController::doDestroyPlaybackRegion (
    juce::ARAPlaybackRegion* playbackRegion) noexcept
{
    DBG ("VoxScriptDocumentController: Destroying playback region");
    delete playbackRegion;
}

juce::ARAPlaybackRenderer* VoxScriptDocumentController::doCreatePlaybackRenderer() noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Creating Playback Renderer");
    DBG ("================================================");
    return new VoxScriptPlaybackRenderer (getDocumentController());
}

//==============================================================================
// State Persistence

bool VoxScriptDocumentController::doRestoreObjectsFromStream (juce::ARAInputStream& input,
                                                              const juce::ARARestoreObjectsFilter* filter) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Restoring state from stream");
    DBG ("================================================");
    
    // Read the entire stream into a memory block
    juce::MemoryBlock data;
    
    // Read strictly what is available
    int64 bytesLeft = input.getTotalLength() - input.getPosition();
    if (bytesLeft > 0)
    {
        data.setSize((size_t)bytesLeft);
        if (!input.read(data.getData(), (size_t)bytesLeft))
            return false;
            
        // Deserialize the store
        if (documentStore.deserialize(data.getData(), data.getSize()))
        {
            // Propagate updates to listeners/UI if needed
            // For now, just logging
            DBG ("VOXSCRIPT: Document store deserialized successfully.");
            return true;
        }
        DBG ("VOXSCRIPT: Document store deserialization FAILED.");
    }
    
    juce::ignoreUnused (filter); // Filter is not used in this implementation
    return false; // Return false if no data or deserialization failed
}

bool VoxScriptDocumentController::doStoreObjectsToStream (juce::ARAOutputStream& output,
                                                            const juce::ARAStoreObjectsFilter* filter) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Storing state to stream");
    DBG ("================================================");
    
    // Serialize the store
    juce::MemoryBlock data = documentStore.serialize();
    
    // Write to ARA stream
    juce::ignoreUnused (filter); // Filter is not used in this implementation
    return output.write(data.getData(), data.getSize());
}

//==============================================================================
// Custom VoxScript API

void VoxScriptDocumentController::notifyTranscriptionUpdated (juce::ARAAudioSource* source)
{
    // Notify all listeners (typically the UI) that transcription is ready
    listeners.call ([source] (Listener& l) { l.transcriptionUpdated (source); });
}

void VoxScriptDocumentController::addListener (Listener* listener)
{
    listeners.add (listener);
}

void VoxScriptDocumentController::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
// WhisperEngine::Listener implementation

void VoxScriptDocumentController::transcriptionProgress (float progress)
{
    transcriptionStatus = juce::String ("Transcribing: ") + 
                          juce::String (int (progress * 100)) + "%";
    
    DBG ("VoxScriptDocumentController: Transcription progress: " + transcriptionStatus);
}

void VoxScriptDocumentController::transcriptionComplete (VoxSequence sequence)
{
    // Update the store
    if (currentAudioSource != nullptr)
    {
        AudioSourceID id = documentStore.getOrCreateAudioSourceID(currentAudioSource);
        documentStore.updateTranscription(id, sequence);
        
        // Notify UI via legacy method (will update later)
        currentTranscription = sequence;
        transcriptionStatus = "Ready";
        DBG ("Transcription complete: " + juce::String (sequence.getWordCount()) + " words");
        
        // Disable sample access now that transcription is done
        if (auto* docController = currentAudioSource->getDocumentController())
        {
            docController->enableAudioSourceSamplesAccess (ARA::PlugIn::toRef (currentAudioSource), false);
            DBG ("Sample access disabled for source: " + juce::String (currentAudioSource->getName()));
        }
        currentAudioSource = nullptr;
    }
    
    // Notify listeners (like the UI Editor)
    notifyTranscriptionUpdated (nullptr);
}

void VoxScriptDocumentController::transcriptionFailed (const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    DBG ("Transcription error: " + error);
    
    // Disable sample access even on failure
    if (currentAudioSource != nullptr)
    {
        if (auto* docController = currentAudioSource->getDocumentController())
        {
            docController->enableAudioSourceSamplesAccess (ARA::PlugIn::toRef (currentAudioSource), false);
            DBG ("Sample access disabled after error for source: " + juce::String (currentAudioSource->getName()));
        }
        currentAudioSource = nullptr;
    }
}

} // namespace VoxScript
