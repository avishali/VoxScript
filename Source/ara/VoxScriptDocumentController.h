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
#include "../transcription/WhisperEngine.h"
#include "../transcription/VoxSequence.h"
#include "../transcription/AudioExtractor.h"

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
 * 
 * From PDF Section 3.1.1: "It receives notifications from the host when audio 
 * clips are added, removed, or modified in the DAW timeline."
 */
class VoxScriptDocumentController : public juce::ARADocumentControllerSpecialisation,
                                     private WhisperEngine::Listener
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
    // WhisperEngine::Listener overrides
    void transcriptionProgress (float progress) override;
    void transcriptionComplete (VoxSequence sequence) override;
    void transcriptionFailed (const juce::String& error) override;

    //==========================================================================
    // Phase II: Transcription API
    
    /** Get the current transcription result. */
    const VoxSequence& getTranscription() const { return currentTranscription; }
    
    /** Get the current transcription status message. */
    juce::String getTranscriptionStatus() const { return transcriptionStatus; }

    /** Accessor for the WhisperEngine instance (Phase III) */
    WhisperEngine& getWhisperEngine() { return whisperEngine; }

    private:
    //==========================================================================
    juce::ListenerList<Listener> listeners;
    
    // Phase II/III: Transcription and Audio Extraction
    WhisperEngine whisperEngine;
    VoxSequence currentTranscription;
    juce::String transcriptionStatus = "Idle";
    juce::ARAAudioSource* currentAudioSource = nullptr;  // Phase III: Track for sample access cleanup
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptDocumentController)
};

} // namespace VoxScript
