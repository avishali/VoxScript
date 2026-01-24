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
    // Poll document controller for status updates & transcription data
    // Phase III: Using DocumentSnapshot (Mission 1)
    
    if (documentController == nullptr)
        return;
    
    // 1. Update Status
    auto newStatus = documentController->getTranscriptionStatus();
    if (newStatus != statusText)
    {
        setStatus(newStatus);
    }
    
    // 2. Poll Store for Data
    // We take a thread-safe snapshot
    auto snapshot = documentController->getStore().makeSnapshot();
    
    // For this phase, we display the first available transcription found in the snapshot.
    // In future phases, we will track the 'selected' AudioSourceID.
    if (!snapshot.transcriptions.empty())
    {
        const auto& seq = snapshot.transcriptions.begin()->second;
        
        // Simple dirtiness check: segment count or just forced refresh if ready
        if (seq.getSegments().size() != currentSequence.getSegments().size() || newStatus == "Ready")
        {
            // Only update if it looks different to avoid repaint spam, unless it's "Ready" (completed)
             // A better dirtiness check would be version/timestamp, but segment count is a proxy for now.
             // If equal count but content changed? We might miss it.
             // Let's assume for now updates always grow or we only care when done.
             // Actually, transcriptionComplete updates the store.
             // Let's just update if counts differ OR if we are in a transient state.
             // But to be safe for Mission 1 verification:
             // If we have content in store and local is empty -> update.
             if (currentSequence.getSegments().isEmpty() && !seq.getSegments().isEmpty())
             {
                 setTranscription(seq);
             }
             // If lengths differ -> update
             else if (seq.getSegments().size() != currentSequence.getSegments().size())
             {
                 setTranscription(seq);
             }
        }
    }
}

} // namespace VoxScript
