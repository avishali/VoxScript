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
    DBG ("================================================");
    DBG ("VOXSCRIPT: Creating Audio Source");
    DBG ("Document: " + juce::String::toHexString ((juce::pointer_sized_int) document));
    DBG ("HostRef: " + juce::String::toHexString ((juce::pointer_sized_int) hostRef));
    DBG ("================================================");
    
    // Create our custom audio source wrapper
    auto* audioSource = new VoxScriptAudioSource (document, hostRef);
    
    // Phase II: Trigger transcription when audio source added
    // Note: We need to access the file path from the audio source
    // For now, we'll defer transcription until we have persistent sample access
    whisperEngine.addListener (this);
    
    DBG ("VoxScriptDocumentController: Audio source created, transcription engine ready");
    
    return audioSource;
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
    DBG ("================================================");
    DBG ("VOXSCRIPT: Creating Audio Modification");
    DBG ("Source: " + juce::String::toHexString ((juce::pointer_sized_int) audioSource));
    DBG ("Clone: " + juce::String (optionalModificationToClone ? "YES" : "NO"));
    DBG ("================================================");
    
    // Phase II: Try to trigger transcription when modification is created
    // At this point, the audio source should have persistent sample access
    // FIXME Phase III: Implement audio extraction from ARA persistent sample access
    // The API for accessing properties in JUCE 8 ARA is different
    // For now, this is a placeholder for future implementation
    juce::ignoreUnused (audioSource);
    
    // TODO: Extract audio from ARA persistent sample reader
    // TODO: Write to temporary WAV file
    // TODO: Call whisperEngine.transcribeAudioFile(tempFile)
    
    // For now, use the default JUCE implementation
    // In Phase III, we'll create a custom modification that stores VoxEditList
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
    DBG ("================================================");
    DBG ("VOXSCRIPT: Creating Playback Region");
    DBG ("Modification: " + juce::String::toHexString ((juce::pointer_sized_int) modification));
    DBG ("================================================");
    
    // Phase II: At this point we definitely have audio data available
    // Try to extract file path and trigger transcription
    // FIXME Phase III: Implement audio extraction from ARA persistent sample access
    // The API for accessing properties in JUCE 8 ARA is different
    // For now, this is a placeholder for future implementation
    juce::ignoreUnused (modification);
    
    // TODO: Get audio source from modification
    // TODO: Extract audio from ARA persistent sample reader
    // TODO: Write to temporary WAV file
    // TODO: Call whisperEngine.transcribeAudioFile(tempFile)
    
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
    
    // FIXME Phase III: Notify UI to repaint
}

void VoxScriptDocumentController::transcriptionComplete (VoxSequence sequence)
{
    currentTranscription = sequence;
    transcriptionStatus = "Ready";
    
    DBG ("================================================");
    DBG ("VOXSCRIPT: Transcription COMPLETE");
    DBG ("Words: " + juce::String (sequence.getWordCount()));
    DBG ("Duration: " + juce::String (sequence.getTotalDuration()) + " seconds");
    DBG ("================================================");
    
    // FIXME Phase III: Notify UI to display new transcription
    // For now, just log the text
    DBG ("Transcription text: " + sequence.getFullText());
}

void VoxScriptDocumentController::transcriptionFailed (const juce::String& error)
{
    transcriptionStatus = "Failed: " + error;
    
    DBG ("================================================");
    DBG ("VOXSCRIPT: Transcription FAILED");
    DBG ("Error: " + error);
    DBG ("================================================");
}

} // namespace VoxScript
