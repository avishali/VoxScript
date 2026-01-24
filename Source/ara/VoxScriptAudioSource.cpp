/*
  ==============================================================================
    VoxScriptAudioSource.cpp
    
    Implementation of the ARA Audio Source wrapper.
    Phase III: Added auto-transcription triggering via JobQueue.
    Mission 3: Isolate Whisper - Removed internal threading.
  ==============================================================================
*/

#include "VoxScriptAudioSource.h"
#include "VoxScriptDocumentController.h"
#include "../util/VoxLogger.h"

namespace VoxScript
{

VoxScriptAudioSource::VoxScriptAudioSource (juce::ARADocument* document,
                                           ARA::ARAAudioSourceHostRef hostRef)
    : juce::ARAAudioSource (document, hostRef)
{
    DBG ("VoxScriptAudioSource: Created");
}

VoxScriptAudioSource::~VoxScriptAudioSource()
{
    // Cleanup handled by DocumentController::doDestroyAudioSource
    DBG ("VoxScriptAudioSource: Destroyed");
}

void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    DBG ("VoxScriptAudioSource: Properties updated");
    
    // Check if sample access is available
    if (!isSampleAccessEnabled())
    {
        transcriptionStatus = "Waiting for access...";
        return;
    }
    
    // Trigger transcription via controller
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(getDocumentController());
    
    if (docController != nullptr)
    {
        triggerTranscriptionWithController(docController);
    }
}

void VoxScriptAudioSource::triggerTranscriptionWithController(VoxScriptDocumentController* docController)
{
    if (docController == nullptr) return;
    
    if (isSampleAccessEnabled())
    {
        transcriptionStatus = "Queued";
        docController->enqueueTranscriptionForSource(this);
        DBG ("VoxScriptAudioSource: Request queued via controller");
    }
    else
    {
        transcriptionStatus = "Waiting for access...";
    }
}

} // namespace VoxScript
