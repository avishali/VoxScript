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
    // M4: Strict gating to prevent crashes on insert/delete
    
    // 1. Prevent calls during static shutdown
    if (juce::MessageManager::getInstanceWithoutCreating() == nullptr) return;

    // 2. Prevent calls before sample access (ARA safety)
    if (!isSampleAccessEnabled()) return;
    
    auto* docController = dynamic_cast<VoxScriptDocumentController*>(getDocumentController());
    
    // 3. Prevent calls before Controller is fully ready
    if (docController == nullptr || !docController->isAraReadyForBackgroundWork()) 
        return;

    DBG ("VoxScriptAudioSource: Properties updated - Triggering Transcription");
    triggerTranscriptionWithController(docController);
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
