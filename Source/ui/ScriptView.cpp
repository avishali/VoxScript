/*
  ==============================================================================
    ScriptView.cpp
    
    Implementation of Script View
    Phase II: Updated to display VoxSequence transcription data
  ==============================================================================
*/

#include "ScriptView.h"

namespace VoxScript
{

ScriptView::ScriptView()
{
    // Initialize with placeholder text
    statusText = "Ready - Awaiting audio source";
    displayText = "";
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
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("No transcription available", 
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

} // namespace VoxScript
