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
    
    // Enable sample access on the main thread (required by ARA SDK)
    if (auto* docController = audioSource->getDocumentController())
    {
        docController->enableAudioSourceSamplesAccess (ARA::PlugIn::toRef (audioSource), true);
    }
    
    // Trigger asynchronous transcription (extraction + inference)
    transcriptionStatus = "Starting analysis...";
    whisperEngine.transcribeAudioSource (audioSource);
    
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
    DBG ("VoxScriptDocumentController::doCreatePlaybackRegion called");
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

bool VoxScriptDocumentController::doRestoreObjectsFromStream (
    juce::ARAInputStream& input,
    const juce::ARARestoreObjectsFilter* filter) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Restoring state from stream");
    DBG ("================================================");
    
    // Phase I: Just return true (no data to restore yet)
    // Phase II/III: Restore VoxSequence transcription data and edits
    
    juce::ignoreUnused (input, filter);
    return true;
}

bool VoxScriptDocumentController::doStoreObjectsToStream (
    juce::ARAOutputStream& output,
    const juce::ARAStoreObjectsFilter* filter) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Storing state to stream");
    DBG ("================================================");
    
    // Phase I: Just return true (no data to store yet)
    // Phase II/III: Store VoxSequence transcription data and edits
    
    juce::ignoreUnused (output, filter);
    return true;
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
    currentTranscription = sequence;
    transcriptionStatus = "Ready";
    DBG ("Transcription complete: " + juce::String (sequence.getWordCount()) + " words");
    
    // Notify listeners (like the UI Editor)
    notifyTranscriptionUpdated (nullptr);
}

void VoxScriptDocumentController::transcriptionFailed (const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    DBG ("Transcription error: " + error);
}

} // namespace VoxScript
