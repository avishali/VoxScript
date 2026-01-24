/*
  ==============================================================================
    ScriptView.cpp
    
    Implementation of Script View
    Phase II: Updated to display VoxSequence transcription data with timestamps
    Phase III: Added timer-based status polling and scrollable viewport
  ==============================================================================
*/

#include "ScriptView.h"
#include "../ara/VoxScriptDocumentController.h"

namespace VoxScript
{

ScriptView::ScriptView()
{
    // Initialize with placeholder text
    statusText = "Ready - Awaiting audio source";
    
    // Set up viewport
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&transcriptionDisplay, false);
    viewport.setScrollBarsShown(true, false); // Vertical scrollbar only
    
    // Start timer for status polling (Phase III temporary solution)
    startTimer(100); // Poll every 100ms
}

ScriptView::~ScriptView()
{
}

//==============================================================================
// Component

void ScriptView::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xffffffff));  // White background
    
    // Border
    g.setColour (juce::Colour (0xffcccccc));
    g.drawRect (getLocalBounds(), 1);
    
    // Draw status in top-left
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (statusText, 10, 10, getWidth() - 20, 20, juce::Justification::left);
    
    // Draw placeholder if no transcription
    if (currentSequence.getSegments().isEmpty())
    {
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Ready - Awaiting audio source", 
                    getLocalBounds().withTrimmedTop(40), 
                    juce::Justification::centred);
    }
}

void ScriptView::resized()
{
    // Position viewport below status bar
    viewport.setBounds(getLocalBounds().withTrimmedTop(40));
    
    // Update transcription display width
    transcriptionDisplay.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), 
                                  transcriptionDisplay.getHeight());
}

//==============================================================================
// Phase II: Transcription Display API

void ScriptView::setTranscription (const VoxSequence& sequence)
{
    currentSequence = sequence;
    transcriptionDisplay.setSequence(sequence);
    
    DBG ("ScriptView: Received transcription with " + juce::String(sequence.getSegments().size()) + " segments");
    
    repaint();
}

void ScriptView::setStatus (const juce::String& status)
{
    statusText = status;
    repaint();
}

void ScriptView::clear()
{
    currentSequence.clear();
    transcriptionDisplay.setSequence(currentSequence);
    statusText = "Ready - Awaiting audio source";
    repaint();
}

void ScriptView::setDocumentController(VoxScriptDocumentController* controller)
{
    documentController = controller;
}

void ScriptView::timerCallback()
{
    // Poll document controller for status updates
    // Note: Proper observer pattern will be implemented in Phase IV
    
    if (documentController == nullptr)
        return;
    
    auto newStatus = documentController->getTranscriptionStatus();
    if (newStatus != statusText)
    {
        setStatus(newStatus);
    }
    
    // Check if transcription updated
    auto& transcription = documentController->getTranscription();
    
    if (newStatus == "Ready" && !transcription.getSegments().isEmpty() 
        && transcription.getSegments().size() != currentSequence.getSegments().size())
    {
        setTranscription(transcription);
    }
}

} // namespace VoxScript
