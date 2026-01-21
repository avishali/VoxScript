/*
  ==============================================================================
    PluginProcessor.cpp
    
    Implementation of main Audio Processor
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

namespace VoxScript
{

//==============================================================================
VoxScriptAudioProcessor::VoxScriptAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    juce::Logger::writeToLog ("================================================");
    juce::Logger::writeToLog ("    VOXSCRIPT v0.1.0 - PHASE I LOADED");
    juce::Logger::writeToLog ("    ARA2 Text-Based Vocal Editing Plugin");
    juce::Logger::writeToLog ("================================================");
    juce::Logger::writeToLog ("ARA Mode: " + juce::String (isBoundToARA() ? "ACTIVE (ARA2)" : "Standalone"));
    juce::Logger::writeToLog ("================================================");
}

VoxScriptAudioProcessor::~VoxScriptAudioProcessor()
{
    juce::Logger::writeToLog ("VOXSCRIPT: Audio Processor DESTROYED");
}

//==============================================================================
// Audio Processing Setup

void VoxScriptAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::Logger::writeToLog ("VoxScriptAudioProcessor: Prepare to play - SR: " 
                              + juce::String (sampleRate) 
                              + " BlockSize: " + juce::String (samplesPerBlock));
    
    // Phase I: No processing needed
    // ARA rendering happens in the VoxScriptPlaybackRenderer
}

void VoxScriptAudioProcessor::releaseResources()
{
    juce::Logger::writeToLog ("VoxScriptAudioProcessor: Release resources");
}

bool VoxScriptAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Support mono and stereo in/out
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    
    // Must have same number of channels in and out
    if (mainInput != mainOutput)
        return false;
    
    // Support mono or stereo
    return mainInput == juce::AudioChannelSet::mono()
        || mainInput == juce::AudioChannelSet::stereo();
}

//==============================================================================
// Audio Processing

void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<float>&, 
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
    // In non-ARA mode, just pass through the audio (do nothing)
    // Phase II+: Could show "ARA mode required" message in UI
}

void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<double>&,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
    // In non-ARA mode, just pass through the audio (do nothing)
}

//==============================================================================
// Editor

juce::AudioProcessorEditor* VoxScriptAudioProcessor::createEditor()
{
    return new VoxScriptAudioProcessorEditor (*this);
}

//==============================================================================
// State Persistence

void VoxScriptAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Phase I: No state to save yet
    // Phase II+: Save user preferences, window size, etc.
    
    juce::ignoreUnused (destData);
}

void VoxScriptAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Phase I: No state to restore yet
    
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// Helper Methods

VoxScriptDocumentController* VoxScriptAudioProcessor::getVoxScriptDocumentController() const
{
    // Check if bound to ARA
    if (isBoundToARA())
    {
        if (auto* playbackRenderer = getPlaybackRenderer())
            if (auto* docController = playbackRenderer->getDocumentController())
                return dynamic_cast<VoxScriptDocumentController*> (
                    juce::ARADocumentControllerSpecialisation::getSpecialisedDocumentController (docController));
    }
    
    return nullptr;
}

} // namespace VoxScript

//==============================================================================
// ARA Factory (required for ARA2 support)
// This tells JUCE which Document Controller class to instantiate

const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return juce::ARADocumentControllerSpecialisation::createARAFactory<VoxScript::VoxScriptDocumentController>();
}

//==============================================================================
// Plugin Entry Point (required by JUCE)

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxScript::VoxScriptAudioProcessor();
}
