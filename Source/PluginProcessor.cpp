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
    std::cerr << "[VoxScript] VoxScriptAudioProcessor constructor starting..." << std::endl;
    // Try to initialize file logger with multiple fallback locations
    juce::File logDir;
    
    // Try location 1: ~/Library/Logs/VoxScript (macOS standard)
    logDir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                 .getChildFile ("Library")
                 .getChildFile ("Logs")
                 .getChildFile ("VoxScript");
    
    // Create directory if it doesn't exist
    if (!logDir.exists())
        logDir.createDirectory();
    
    // If still can't create, try Desktop as fallback
    if (!logDir.exists() || !logDir.isDirectory())
    {
        logDir = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                     .getChildFile ("VoxScript_Logs");
        logDir.createDirectory();
    }
    
    // Create the logger with explicit file path
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
    auto logFile = logDir.getChildFile ("VoxScript_" + timestamp + ".log");
    
    // Use FileLogger constructor directly instead of createDateStampedLogger
    fileLogger_ = std::make_unique<juce::FileLogger> (logFile, "VoxScript Debug Session");
    
    // Set as current logger
    if (fileLogger_)
    {
        juce::Logger::setCurrentLogger (fileLogger_.get());
        
        // Write session header
        juce::Logger::writeToLog ("================================================================================");
        juce::Logger::writeToLog ("VoxScript Debug Log");
        juce::Logger::writeToLog ("Started: " + juce::Time::getCurrentTime().toString (true, true));
        juce::Logger::writeToLog ("Version: 0.1.0");
        juce::Logger::writeToLog ("ARA Mode: " + juce::String (isBoundToARA() ? "ACTIVE" : "Standalone"));
        juce::Logger::writeToLog ("Log File: " + fileLogger_->getLogFile().getFullPathName());
        juce::Logger::writeToLog ("Log Directory: " + logDir.getFullPathName());
        juce::Logger::writeToLog ("================================================================================");
        juce::Logger::writeToLog ("");
    }
    else
    {
        // Fallback to stderr if logger fails
        std::cerr << "\n[VoxScript] WARNING: Could not create file logger at " << logDir.getFullPathName() << "\n";
    }
    
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
    juce::Logger::writeToLog ("");
    juce::Logger::writeToLog ("================================================================================");
    juce::Logger::writeToLog ("Session ended: " + juce::Time::getCurrentTime().toString (true, true));
    juce::Logger::writeToLog ("================================================================================");
    
    // Clear logger before destroying it
    if (fileLogger_)
    {
        juce::Logger::setCurrentLogger (nullptr);
        fileLogger_.reset();
    }
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

void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, 
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
       juce::ignoreUnused (midiMessages);
       
       // In non-ARA mode, audio passes through unchanged
       if (!isBoundToARA())
       {
           // Audio buffer already contains input - just pass it through
           return;
       }
       
       // In ARA mode, audio processing happens in VoxScriptPlaybackRenderer
}

void VoxScriptAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    if (!isBoundToARA())
    {
        return;
    }
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
    // Log to stderr for early crash debugging
    std::cerr << "[VoxScript] createARAFactory() called" << std::endl;
    
    try
    {
        auto* factory = juce::ARADocumentControllerSpecialisation::createARAFactory<VoxScript::VoxScriptDocumentController>();
        std::cerr << "[VoxScript] ARA Factory created successfully: " << (void*)factory << std::endl;
        return factory;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[VoxScript] EXCEPTION in createARAFactory: " << e.what() << std::endl;
        return nullptr;
    }
    catch (...)
    {
        std::cerr << "[VoxScript] UNKNOWN EXCEPTION in createARAFactory" << std::endl;
        return nullptr;
    }
}

//==============================================================================
// Plugin Entry Point (required by JUCE)

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    // Log to stderr for early crash debugging
    std::cerr << "[VoxScript] createPluginFilter() called" << std::endl;
    
    try
    {
        auto* processor = new VoxScript::VoxScriptAudioProcessor();
        std::cerr << "[VoxScript] Audio Processor created successfully: " << (void*)processor << std::endl;
        return processor;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[VoxScript] EXCEPTION in createPluginFilter: " << e.what() << std::endl;
        return nullptr;
    }
    catch (...)
    {
    std::cerr << "[VoxScript] UNKNOWN EXCEPTION in createPluginFilter" << std::endl;
        return nullptr;
    }
}
