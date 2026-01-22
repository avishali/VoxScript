/*
  ==============================================================================
    VoxScriptAudioSource.cpp
    
    Implementation of custom ARA Audio Source
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#include "VoxScriptAudioSource.h"
#include "../transcription/AudioExtractor.h"

namespace VoxScript
{

    VoxScriptAudioSource::VoxScriptAudioSource (juce::ARADocument* document,
             ARA::ARAAudioSourceHostRef hostRef)
    : juce::ARAAudioSource (document, hostRef)
{
    std::cerr << "\n\n******************************\n";
    std::cerr << "AUDIO SOURCE CREATED!!!\n";
    std::cerr << "******************************\n\n";

    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Source Created");
    DBG ("(Properties will be available after notifyPropertiesUpdated)");
    DBG ("================================================");
}


VoxScriptAudioSource::~VoxScriptAudioSource()
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Source DESTROYED");
    DBG ("================================================");
    
    // Phase II: Make sure to stop and join analysis thread here
}

//==============================================================================
// ARA Notifications

void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    std::cerr << "\n\n******************************\n";
    std::cerr << "PROPERTIES UPDATED!!!\n";
    std::cerr << "******************************\n\n";
    
    DBG ("VoxScriptAudioSource: Properties updated");
}

void VoxScriptAudioSource::notifyContentChanged (juce::ARAContentUpdateScopes scopeFlags) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Content Changed!");
    DBG ("Scope flags: " + juce::String (scopeFlags));
    DBG ("This would trigger re-transcription in Phase II");
    DBG ("================================================");
    
    juce::ignoreUnused (scopeFlags);
}

//==============================================================================
// Transcription API (Phase II placeholders)

juce::String VoxScriptAudioSource::getTranscription() const
{
    return transcriptionText.isEmpty() 
        ? "Transcription will appear here (Phase II)" 
        : transcriptionText;
}

bool VoxScriptAudioSource::isTranscriptionReady() const
{
    return transcriptionReady;
}

float VoxScriptAudioSource::getTranscriptionProgress() const
{
    return transcriptionProgress;
}

} // namespace VoxScript
