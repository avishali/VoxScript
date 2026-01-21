/*
  ==============================================================================
    ScriptView.h
    
    The Script View - Semantic text editing layer
    
    From PDF Section 2.1.1: "The primary workspace functions as a rich-text 
    editor. The transcribed audio flows continuously, wrapping at the window edge."
    
    Phase II: Updated to display VoxSequence transcription data
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../transcription/VoxSequence.h"

namespace VoxScript
{

// Forward declaration
class VoxScriptDocumentController;

/**
 * @brief The Script View - Semantic editing layer
 * 
 * This is the main text editor interface where users see and edit transcribed audio.
 * 
 * Phase I: Simple text display with placeholder
 * Phase II: Display Whisper transcription
 * Phase III: Interactive editing with text selection -> audio edits
 * 
 * Features (from PDF):
 * - Rich text editor with word wrapping
 * - Bi-directional selection with DAW timeline
 * - Text editing operations (delete, cut, copy, paste)
 * - Non-speech tokens (<breath>, <silence>)
 * - Confidence underlining for low-confidence words
 */
class ScriptView : public juce::Component,
                   private juce::Timer
{
public:
    //==========================================================================
    ScriptView();
    ~ScriptView() override;
    
    //==========================================================================
    // Component
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // Phase II: Transcription Display API
    
    /**
     * Set the transcription data to display
     */
    void setTranscription (const VoxSequence& sequence);
    
    /**
     * Set status message (e.g., "Transcribing...", "Ready")
     */
    void setStatus (const juce::String& status);
    
    /**
     * Clear all text
     */
    void clear();
    
    /**
     * Set the document controller for status polling (Phase III)
     */
    void setDocumentController(VoxScriptDocumentController* controller);

private:
    //==========================================================================
    // Timer callback for status polling (Phase III temporary solution)
    void timerCallback() override;
    //==========================================================================
    // Phase II: Display members
    juce::String displayText;
    juce::String statusText;
    
    // Phase III: Document controller for status polling
    VoxScriptDocumentController* documentController = nullptr;
    
    // Phase II: Will add:
    // - TextEditor for interactive editing
    // - Token visualization (breaths, silences)
    // - Confidence underlining
    
    // Phase III: Will add:
    // - Selection synchronization with timeline
    // - Context menu for corrections
    // - Undo/redo stack
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptView)
};

} // namespace VoxScript
