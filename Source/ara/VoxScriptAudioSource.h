#pragma once

#include <JuceHeader.h>

// --- FIX: ADDED INCLUDES ---
#include "../transcription/WhisperEngine.h"
#include "../transcription/VoxSequence.h"

namespace VoxScript
{

// Forward declaration to avoid circular dependency
class VoxScriptDocumentController;

// --- FIX: INHERIT FROM juce::ARAAudioSource ---
class VoxScriptAudioSource : public juce::ARAAudioSource,
                             public WhisperEngine::Listener
{
public:
    // Constructor
    VoxScriptAudioSource(juce::ARADocument* document, ARA::ARAAudioSourceHostRef hostRef);
    
    ~VoxScriptAudioSource() override;

    // ARA override - called when audio source properties are finalized
    // NOT virtual in JUCE 8+, so NO override keyword
    void notifyPropertiesUpdated() noexcept;
    
    // Phase III: Trigger transcription with controller (called from DocumentController)
    void triggerTranscriptionWithController(VoxScriptDocumentController* controller);
    
    // WhisperEngine::Listener overrides
    void transcriptionProgress(float progress) override;
    void transcriptionComplete(VoxSequence sequence) override;
    void transcriptionFailed(const juce::String& error) override;
    
    const VoxSequence& getTranscription() const { return currentTranscription; }
    bool isTranscriptionReady() const { return transcriptionReady; }
    juce::String getTranscriptionStatus() const { return transcriptionStatus; }

private:
    // NOTE: We get DocumentController when needed (in triggerTranscription)
    // instead of storing it, since it's not available in constructor
    
    // Phase III: Transcription triggering
    void triggerTranscription();
    
    // Phase III: Background transcription thread
    class TranscriptionThread : public juce::Thread
    {
    public:
        TranscriptionThread(VoxScriptAudioSource* source, VoxScriptDocumentController* controller)
            : juce::Thread("VoxScript Transcription"),
              audioSource(source),
              docController(controller)
        {
        }
        
        void run() override
        {
            if (audioSource && docController)
            {
                audioSource->runTranscriptionOnBackgroundThread(docController);
            }
        }
        
    private:
        VoxScriptAudioSource* audioSource;
        VoxScriptDocumentController* docController;
    };
    
    friend class TranscriptionThread;
    void runTranscriptionOnBackgroundThread(VoxScriptDocumentController* docController);
    
    std::unique_ptr<TranscriptionThread> transcriptionThread;
    
    // Phase III: Transcription state
    VoxSequence currentTranscription;
    bool transcriptionReady = false;
    bool transcriptionInProgress = false;  // Track if transcription is running
    juce::String transcriptionStatus = "Idle";
    juce::File tempAudioFile;
};

} // namespace VoxScript
