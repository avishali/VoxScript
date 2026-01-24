#pragma once

#include <JuceHeader.h>
#include "../transcription/VoxSequence.h"

namespace VoxScript
{

// Forward declaration to avoid circular dependency
class VoxScriptDocumentController;

// --- FIX: INHERIT FROM juce::ARAAudioSource ONLY ---
class VoxScriptAudioSource : public juce::ARAAudioSource
{
public:
    // Constructor
    VoxScriptAudioSource(juce::ARADocument* document, ARA::ARAAudioSourceHostRef hostRef);
    
    ~VoxScriptAudioSource() override;

    // ARA override - called when audio source properties are finalized
    void notifyPropertiesUpdated() noexcept;
    
    // Phase III: Trigger transcription with controller (called from DocumentController)
    void triggerTranscriptionWithController(VoxScriptDocumentController* controller);
    
    juce::String getTranscriptionStatus() const { return transcriptionStatus; }

private:
    // Phase III: Transcription state
    // We keep status string for debugging, but actual transcription is in Store
    juce::String transcriptionStatus = "Idle";
};

} // namespace VoxScript
