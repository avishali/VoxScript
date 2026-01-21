/*
  ==============================================================================
    ScriptView.cpp
    
    Implementation of Script View
    Phase II: Updated to display VoxSequence transcription data
    Phase III: Added timer-based status polling
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
    displayText = "";
    
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
    g.drawText (statusText, 10, 10, 300, 20, juce::Justification::left);
    
    // Draw transcription text
    if (displayText.isNotEmpty())
    {
        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (14.0f));
        
        auto textArea = getLocalBounds().reduced (20).withTrimmedTop (40);
        
        // Simple multi-line text drawing
        g.drawMultiLineText (displayText, 
                            textArea.getX(), 
                            textArea.getY() + 20,
                            textArea.getWidth());
    }
    else
    {
        // Show placeholder text when no transcription
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Ready - Awaiting audio source", 
                    getLocalBounds(), 
                    juce::Justification::centred);
    }
}

void ScriptView::resized()
{
    // No child components to layout in Phase II
    // Phase III will add TextEditor component
}

//==============================================================================
// Phase II: Transcription Display API

void ScriptView::setTranscription (const VoxSequence& sequence)
{
    displayText = sequence.getFullText();
    repaint();
}

void ScriptView::setStatus (const juce::String& status)
{
    statusText = status;
    repaint();
}

void ScriptView::clear()
{
    displayText = "";
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
    if (!transcription.getSegments().isEmpty() && displayText.isEmpty())
    {
        setTranscription(transcription);
    }
}

} // namespace VoxScript
