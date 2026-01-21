/*
  ==============================================================================
    PluginProcessor.h
    
    Main Audio Processor for VoxScript with ARA2 support
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ara/VoxScriptDocumentController.h"
#include "ara/VoxScriptPlaybackRenderer.h"

namespace VoxScript
{

/**
 * @brief Main Audio Processor for VoxScript
 * 
 * This is the entry point for the plugin. It integrates with JUCE's ARA framework
 * and creates our custom Document Controller and Playback Renderer.
 * 
 * The processor can operate in two modes:
 * 1. Non-ARA mode: Traditional plugin (VST3/AU without ARA host support)
 * 2. ARA mode: Full text-editing capabilities when hosted in ARA-compatible DAW
 */
class VoxScriptAudioProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorARAExtension
{
public:
    //==========================================================================
    VoxScriptAudioProcessor();
    ~VoxScriptAudioProcessor() override;

    //==========================================================================
    // Standard AudioProcessor methods
    
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    
    //==========================================================================
    // Editor
    
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    
    //==========================================================================
    // Plugin Properties
    
    const juce::String getName() const override { return JucePlugin_Name; }
    
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    
    double getTailLengthSeconds() const override { return 0.0; }
    
    //==========================================================================
    // Programs (Presets)
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    const juce::String getProgramName (int index) override 
    { 
        juce::ignoreUnused (index);
        return "Default"; 
    }
    void changeProgramName (int index, const juce::String& newName) override
    {
        juce::ignoreUnused (index, newName);
    }
    
    //==========================================================================
    // State Persistence
    
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==========================================================================
    // Helper methods
    
    /**
     * Get the document controller (ARA mode only)
     */
    VoxScriptDocumentController* getVoxScriptDocumentController() const;

private:
    //==========================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptAudioProcessor)
};

} // namespace VoxScript
