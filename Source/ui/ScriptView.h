/*
  ==============================================================================
    ScriptView.h
    
    The Script View - Semantic text editing layer
    
    From PDF Section 2.1.1: "The primary workspace functions as a rich-text 
    editor. The transcribed audio flows continuously, wrapping at the window edge."
    
    Phase II: Updated to display VoxSequence transcription data with timestamps
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
 * Phase II: Display Whisper transcription with timestamps
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
    juce::String statusText;
    VoxSequence currentSequence;
    
    // Phase III: Document controller for status polling
    VoxScriptDocumentController* documentController = nullptr;
    
    // Scrollable viewport for transcription
    juce::Viewport viewport;
    
    // Component to hold the transcription text with timestamps
    class TranscriptionDisplay : public juce::Component
    {
    public:
        void setSequence(const VoxSequence& seq)
        {
            sequence = seq;
            updateSize();
            repaint();
        }
        
        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::white);
            
            if (sequence.getSegments().isEmpty())
                return;
            
            int yPos = 10;
            g.setFont(juce::FontOptions(13.0f));
            
            for (const auto& segment : sequence.getSegments())
            {
                // Format timestamp [MM:SS.mmm]
                int minutes = (int)(segment.startTime / 60.0);
                double seconds = segment.startTime - (minutes * 60.0);
                juce::String timestamp = juce::String::formatted("[%02d:%06.3f] ", minutes, seconds);
                
                // Draw timestamp in grey
                g.setColour(juce::Colours::grey);
                g.drawText(timestamp, 10, yPos, 120, 20, juce::Justification::left);
                
                // Draw text in black
                g.setColour(juce::Colours::black);
                auto textBounds = juce::Rectangle<int>(135, yPos, getWidth() - 145, 1000);
                
                // Calculate how many lines this text needs
                juce::AttributedString attrStr;
                attrStr.append(segment.text, juce::FontOptions(13.0f));
                
                juce::TextLayout layout;
                layout.createLayout(attrStr, (float)(getWidth() - 145));
                
                layout.draw(g, textBounds.toFloat());
                
                int textHeight = (int)layout.getHeight();
                yPos += textHeight + 15; // Add spacing between segments
            }
        }
        
        void updateSize()
        {
            // Calculate required height for all segments
            int totalHeight = 10;
            
            for (const auto& segment : sequence.getSegments())
            {
                // Estimate lines needed (rough calculation)
                int charsPerLine = (getWidth() - 145) / 7; // Approximate
                if (charsPerLine < 20) charsPerLine = 20;
                
                int lines = juce::jmax(1, (int)std::ceil((float)segment.text.length() / (float)charsPerLine));
                totalHeight += lines * 18 + 15; // 18px per line + 15px spacing
            }
            
            setSize(getWidth(), totalHeight);
        }
        
        void resized() override
        {
            updateSize();
        }
        
    private:
        VoxSequence sequence;
    };
    
    TranscriptionDisplay transcriptionDisplay;
    
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
