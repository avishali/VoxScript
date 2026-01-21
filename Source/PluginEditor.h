/*
  ==============================================================================
    PluginEditor.h
    
    Main Plugin Editor - Dual-view interface combining Script View and Detail View
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/ScriptView.h"
#include "ui/DetailView.h"

namespace VoxScript
{

/**
 * @brief Main Plugin Editor for VoxScript
 * 
 * Implements the dual-view paradigm from PDF Section 2.1:
 * - Top: Script View (semantic text editing)
 * - Bottom: Detail View (signal-level precision)
 * 
 * Handles different DAW presentation modes:
 * - Studio One: Integrated editor pane
 * - Logic Pro: Floating window
 * - Reaper: Docker/floating
 * - Pro Tools: Tabbed editor
 */
class VoxScriptAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public VoxScriptDocumentController::Listener
{
public:
    //==========================================================================
    explicit VoxScriptAudioProcessorEditor (VoxScriptAudioProcessor& processor);
    ~VoxScriptAudioProcessorEditor() override;

    //==========================================================================
    // Component
    
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //==========================================================================
    // DocumentController::Listener
    
    /**
     * Called when transcription is updated
     */
    void transcriptionUpdated (juce::ARAAudioSource* source) override;

private:
    //==========================================================================
    VoxScriptAudioProcessor& processorRef;
    
    // UI Components
    ScriptView scriptView;
    DetailView detailView;
    
    juce::Label headerLabel;
    juce::Label araStatusLabel;
    
    // Splitter between script and detail views
    float splitRatio { 0.65f };  // 65% for script view, 35% for detail view
    
    // Phase I helper
    void updateARAStatus();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxScriptAudioProcessorEditor)
};

} // namespace VoxScript
