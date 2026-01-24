/*
  ==============================================================================
    VoxScriptAudioSource.cpp
    Created: 22 Jan 2026
    Author: VoxScript Team
    
    Implementation of the ARA Audio Source wrapper.
    Phase III: Added auto-transcription triggering with file-based logging.
  ==============================================================================
*/

#include "VoxScriptAudioSource.h"
#include "VoxScriptDocumentController.h"
#include "../transcription/AudioExtractor.h"
#include "../transcription/WhisperEngine.h"
#include "../util/VoxLogger.h"

namespace VoxScript
{

VoxScriptAudioSource::VoxScriptAudioSource (juce::ARADocument* document,
                                           ARA::ARAAudioSourceHostRef hostRef)
    : juce::ARAAudioSource (document, hostRef)
{
    VOXLOG("========================================");
    VOXLOG("AUDIO SOURCE CREATED!!!");
    VOXLOG("Document: " + juce::String::toHexString((juce::pointer_sized_int)document));
    VOXLOG("========================================");
    
    DBG ("VoxScriptAudioSource: Created");
    
    // Don't get controller here - it may not be set up yet
    // We'll get it when we actually need it (in triggerTranscription)
}

VoxScriptAudioSource::~VoxScriptAudioSource()
{
    VOXLOG("Audio Source being destroyed");
    
    // CRITICAL: Stop and wait for transcription thread
    if (transcriptionThread != nullptr)
    {
        VOXLOG("Stopping transcription thread...");
        transcriptionThread->stopThread(30000);  // Wait up to 30 seconds
        VOXLOG("Transcription thread stopped");
    }
    
    transcriptionInProgress = false;
    
    // Clean up temp file if it still exists
    if (tempAudioFile.existsAsFile())
    {
        VOXLOG("Cleaning up temp file in destructor: " + tempAudioFile.getFullPathName());
        tempAudioFile.deleteFile();
    }
}

void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    VOXLOG("========================================");
    VOXLOG("PROPERTIES UPDATED CALLED!!!");
    VOXLOG("Sample Rate: " + juce::String(getSampleRate()) + " Hz");
    VOXLOG("Channels: " + juce::String(getChannelCount()));
    VOXLOG("Duration: " + juce::String(getDuration()) + " seconds");
    VOXLOG("Sample Access Enabled: " + juce::String(isSampleAccessEnabled() ? "YES" : "NO"));
    VOXLOG("========================================");
    
    DBG ("VoxScriptAudioSource: Properties updated");
    DBG ("  Sample Rate: " + juce::String(getSampleRate()) + " Hz");
    DBG ("  Channels: " + juce::String(getChannelCount()));
    DBG ("  Duration: " + juce::String(getDuration()) + " seconds");
    
    // Check if sample access is available
    if (!isSampleAccessEnabled())
    {
        VOXLOG("Sample access NOT available - skipping transcription");
        DBG ("VoxScriptAudioSource: Sample access not available yet");
        transcriptionStatus = "Sample access unavailable";
        return;
    }
    
    VOXLOG("Sample access IS available - launching transcription thread");
    DBG ("VoxScriptAudioSource: Sample access enabled, triggering transcription");
    
    // Note: This may not work because dynamic_cast fails
    // Better to use triggerTranscriptionWithController() from DocumentController
    
    // Try to get controller safely
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(getDocumentController());
    
    if (docController != nullptr)
    {
        VOXLOG("Controller found - triggering transcription safely");
        triggerTranscriptionWithController(docController);
    }
    else
    {
        VOXLOG("WARNING: notifyPropertiesUpdated - DocumentController not available yet");
        transcriptionStatus = "Controller unavailable";
    }
}

void VoxScriptAudioSource::triggerTranscriptionWithController(VoxScriptDocumentController* docController)
{
    VOXLOG("========================================");
    VOXLOG("triggerTranscriptionWithController() called");
    VOXLOG("Controller: " + juce::String::toHexString((juce::pointer_sized_int)docController));
    VOXLOG("========================================");
    
    if (docController == nullptr)
    {
        VOXLOG("ERROR: Controller is null!");
        return;
    }
    
    if (transcriptionInProgress)
    {
        VOXLOG("WARNING: Transcription already in progress - skipping");
        return;
    }
    
    DBG ("VoxScriptAudioSource: Triggering transcription with controller");
    
    // Mark transcription as in progress
    transcriptionInProgress = true;
    transcriptionStatus = "Starting...";
    VOXLOG("Transcription marked as IN PROGRESS");
    
    // Create and start background thread
    transcriptionThread = std::make_unique<TranscriptionThread>(this, docController);
    transcriptionThread->startThread();
    VOXLOG("Background thread started");
}

void VoxScriptAudioSource::runTranscriptionOnBackgroundThread(VoxScriptDocumentController* docController)
{
    VOXLOG("Running transcription on background thread");
    VOXLOG("Thread ID: " + juce::String::toHexString((juce::pointer_sized_int)juce::Thread::getCurrentThreadId()));
    
    // Extract audio to temp WAV
    transcriptionStatus = "Extracting audio...";
    
    VOXLOG("Calling AudioExtractor::extractToTempWAV()...");
    tempAudioFile = AudioExtractor::extractToTempWAV(this, "voxscript_");
    
    if (!tempAudioFile.existsAsFile())
    {
        VOXLOG("ERROR: Audio extraction FAILED - no temp file created");
        DBG ("VoxScriptAudioSource: Audio extraction failed");
        transcriptionStatus = "Extraction failed";
        transcriptionInProgress = false;
        return;
    }
    
    VOXLOG("Extraction SUCCESS!");
    VOXLOG("Temp file: " + tempAudioFile.getFullPathName());
    VOXLOG("File size: " + juce::String(tempAudioFile.getSize() / 1024) + " KB");
    
    DBG ("VoxScriptAudioSource: Extraction complete: " + tempAudioFile.getFullPathName());
    DBG ("  File size: " + juce::String(tempAudioFile.getSize() / 1024) + " KB");
    
    // CRITICAL: Register listener on message thread (ListenerList not thread-safe)
    VOXLOG("Registering as WhisperEngine listener on message thread");
    juce::MessageManager::callAsync([this, docController]()
    {
        docController->getWhisperEngine().addListener(this);
        VOXLOG("Listener registered on message thread");
    });
    
    // Wait a moment for listener registration to complete
    juce::Thread::sleep(50);
    
    // Start transcription
    VOXLOG("Starting WhisperEngine transcription...");
    transcriptionStatus = "Starting transcription...";
    docController->getWhisperEngine().transcribeAudioFile(tempAudioFile);
    VOXLOG("WhisperEngine.transcribeAudioFile() called");
}

void VoxScriptAudioSource::triggerTranscription()
{
    VOXLOG("triggerTranscription() called");
    DBG ("VoxScriptAudioSource: Triggering transcription");
    
    // Get DocumentController NOW (when we actually need it)
    VOXLOG("Getting DocumentController...");
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(
        getDocumentController()
    );
    
    if (docController == nullptr)
    {
        VOXLOG("ERROR: Cannot get DocumentController - aborting transcription");
        VOXLOG("  getDocumentController() returned: " + juce::String::toHexString((juce::pointer_sized_int)getDocumentController()));
        DBG ("VoxScriptAudioSource: No DocumentController available");
        transcriptionStatus = "Internal error";
        return;
    }
    
    VOXLOG("DocumentController obtained successfully: " + juce::String::toHexString((juce::pointer_sized_int)docController));
    
    // Extract audio to temp WAV
    transcriptionStatus = "Extracting audio...";
    
    VOXLOG("Calling AudioExtractor::extractToTempWAV()...");
    tempAudioFile = AudioExtractor::extractToTempWAV(this, "voxscript_");
    
    if (!tempAudioFile.existsAsFile())
    {
        VOXLOG("ERROR: Audio extraction FAILED - no temp file created");
        DBG ("VoxScriptAudioSource: Audio extraction failed");
        transcriptionStatus = "Extraction failed";
        return;
    }
    
    VOXLOG("Extraction SUCCESS!");
    VOXLOG("Temp file: " + tempAudioFile.getFullPathName());
    VOXLOG("File size: " + juce::String(tempAudioFile.getSize() / 1024) + " KB");
    
    DBG ("VoxScriptAudioSource: Extraction complete: " + tempAudioFile.getFullPathName());
    DBG ("  File size: " + juce::String(tempAudioFile.getSize() / 1024) + " KB");
    
    // CRITICAL: Register listener on message thread (ListenerList not thread-safe)
    VOXLOG("Registering as WhisperEngine listener on message thread");
    juce::MessageManager::callAsync([this, docController]()
    {
        docController->getWhisperEngine().addListener(this);
        VOXLOG("Listener registered on message thread");
    });
    
    // Wait a moment for listener registration to complete
    juce::Thread::sleep(50);
    
    // Start transcription
    VOXLOG("Starting WhisperEngine transcription...");
    transcriptionStatus = "Starting transcription...";
    docController->getWhisperEngine().transcribeAudioFile(tempAudioFile);
    VOXLOG("WhisperEngine.transcribeAudioFile() called");
}

void VoxScriptAudioSource::transcriptionProgress(float progress)
{
    int percentage = juce::roundToInt(progress * 100.0f);
    transcriptionStatus = "Transcribing: " + juce::String(percentage) + "%";
    
    VOXLOG("Transcription progress: " + juce::String(percentage) + "%");
    DBG ("VoxScriptAudioSource: Transcription progress: " + juce::String(percentage) + "%");
}

void VoxScriptAudioSource::transcriptionComplete(VoxSequence sequence)
{
    VOXLOG("========================================");
    VOXLOG("TRANSCRIPTION COMPLETE!!!");
    VOXLOG("Segments: " + juce::String(sequence.getSegments().size()));
    VOXLOG("========================================");
    
    DBG ("VoxScriptAudioSource: Transcription complete!");
    DBG ("  Segments: " + juce::String(sequence.getSegments().size()));
    
    currentTranscription = sequence;
    transcriptionReady = true;
    transcriptionInProgress = false;  // Clear in-progress flag
    transcriptionStatus = "Ready";
    
    VOXLOG("Transcription marked as COMPLETE (no longer in progress)");
    
    // Clean up temp file
    if (tempAudioFile.existsAsFile())
    {
        VOXLOG("Deleting temp file: " + tempAudioFile.getFullPathName());
        DBG ("VoxScriptAudioSource: Deleting temp file: " + tempAudioFile.getFullPathName());
        tempAudioFile.deleteFile();
    }
    
    // Remove listener
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(
        getDocumentController()
    );
    if (docController != nullptr)
    {
        VOXLOG("Removing WhisperEngine listener");
        docController->getWhisperEngine().removeListener(this);
    }
    
    VOXLOG("Transcription complete - ready for display");
}

void VoxScriptAudioSource::transcriptionFailed(const juce::String& error)
{
    VOXLOG("========================================");
    VOXLOG("TRANSCRIPTION FAILED!!!");
    VOXLOG("Error: " + error);
    VOXLOG("========================================");
    
    DBG ("VoxScriptAudioSource: Transcription failed: " + error);
    transcriptionStatus = "Failed: " + error;
    transcriptionInProgress = false;  // Clear in-progress flag
    
    VOXLOG("Transcription marked as FAILED (no longer in progress)");
    
    // Clean up temp file
    if (tempAudioFile.existsAsFile())
    {
        VOXLOG("Deleting temp file after failure");
        DBG ("VoxScriptAudioSource: Deleting temp file after failure");
        tempAudioFile.deleteFile();
    }
    
    // Remove listener
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(
        getDocumentController()
    );
    if (docController != nullptr)
    {
        docController->getWhisperEngine().removeListener(this);
    }
}

} // namespace VoxScript
