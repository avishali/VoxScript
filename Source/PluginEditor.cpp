/*
  ==============================================================================
    PluginEditor.cpp
    
    Implementation of main Plugin Editor
  ==============================================================================
*/

#include "PluginEditor.h"

namespace VoxScript
{

VoxScriptAudioProcessorEditor::VoxScriptAudioProcessorEditor (VoxScriptAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , processorRef (p)
{
    // Register as listener for transcription updates
    if (auto* controller = processorRef.getVoxScriptDocumentController())
    {
        controller->addListener (this);
    }
    
    // Setup header
    headerLabel.setText ("VoxScript - Text-Based Audio Editor", juce::dontSendNotification);
    headerLabel.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
    headerLabel.setJustificationType (juce::Justification::centred);
    headerLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (headerLabel);
    
    // Setup ARA status
    araStatusLabel.setFont (juce::FontOptions (12.0f));
    araStatusLabel.setJustificationType (juce::Justification::centredRight);
    araStatusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (araStatusLabel);
    updateARAStatus();
    
    // Add views
    addAndMakeVisible (scriptView);
    addAndMakeVisible (detailView);
    
    // Set initial size
    // From PDF: Different hosts have different presentation requirements
    setSize (800, 600);
    
    // Make resizable
    #ifdef PLUGIN_EDITOR_RESIZABLE
    setResizable (true, true);
    setResizeLimits (600, 400, 2000, 1200);
    #endif
}

VoxScriptAudioProcessorEditor::~VoxScriptAudioProcessorEditor()
{
    // Unregister listener
    if (auto* controller = processorRef.getVoxScriptDocumentController())
    {
        controller->removeListener (this);
    }
}

//==============================================================================
// Component

void VoxScriptAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff2a2a2a));
    
    // Header area
    auto headerBounds = getLocalBounds().removeFromTop (50);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (headerBounds);
    
    // Splitter line between views
    auto bounds = getLocalBounds().withTrimmedTop (50);
    auto splitY = static_cast<int> (bounds.getHeight() * splitRatio);
    
    g.setColour (juce::Colour (0xff4a4a4a));
    g.drawLine (0, 50 + splitY, getWidth(), 50 + splitY, 2.0f);
}

void VoxScriptAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Header area
    auto headerBounds = bounds.removeFromTop (50);
    headerLabel.setBounds (headerBounds.reduced (10));
    
    // ARA status in top-right of header
    auto statusBounds = headerBounds.removeFromRight (200).reduced (10, 15);
    araStatusLabel.setBounds (statusBounds);
    
    // Split remaining space between script and detail views
    auto splitY = static_cast<int> (bounds.getHeight() * splitRatio);
    
    auto scriptBounds = bounds.removeFromTop (splitY);
    scriptView.setBounds (scriptBounds);
    
    // Remaining space for detail view
    detailView.setBounds (bounds);
}

//==============================================================================
// DocumentController::Listener

void VoxScriptAudioProcessorEditor::transcriptionUpdated (juce::ARAAudioSource* source)
{
    // Phase II: Update UI with new transcription
    // For now, just log
    DBG ("Editor: Transcription updated for source");
    juce::ignoreUnused (source);
    
    // Update the script view with transcription
    // if (auto* voxSource = dynamic_cast<VoxScriptAudioSource*> (source))
    // {
    //     scriptView.setTranscription (voxSource->getTranscription());
    // }
}

//==============================================================================
// Helper Methods

void VoxScriptAudioProcessorEditor::updateARAStatus()
{
    if (processorRef.isBoundToARA())
    {
        araStatusLabel.setText ("ARA2 Active", juce::dontSendNotification);
        araStatusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else
    {
        araStatusLabel.setText ("ARA2 Not Active", juce::dontSendNotification);
        araStatusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    }
}

} // namespace VoxScript
