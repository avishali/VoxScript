/*
  ==============================================================================
    DetailView.h
    
    The Detail View - Signal-level precision layer
    
    From PDF Section 2.1.2: "Located below the Script View, the Detail View 
    offers a microscopic look at the audio corresponding to the current text selection."
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace VoxScript
{

/**
 * @brief The Detail View - Signal-level editing layer
 * 
 * Shows waveform + spectrogram with phoneme alignment overlay.
 * Allows micro-editing of cut boundaries.
 * 
 * Phase I: Simple waveform display placeholder
 * Phase II: Basic waveform visualization
 * Phase III: Phoneme overlay + boundary dragging
 * 
 * Features (from PDF):
 * - Waveform & Spectrogram hybrid view
 * - Phoneme alignment visualization (e.g., /h/ /e/ /l/ /o/ for "Hello")
 * - Draggable edit boundaries for precision
 * - Real-time crossfade preview
 */
class DetailView : public juce::Component
{
public:
    //==========================================================================
    DetailView();
    ~DetailView() override;
    
    //==========================================================================
    // Component
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // Phase II: Audio Display API
    
    /**
     * Set the audio buffer to display
     */
    void setAudioData (const juce::AudioBuffer<float>& buffer);
    
    /**
     * Set the time range to display (in seconds)
     */
    void setTimeRange (double startTime, double endTime);
    
    /**
     * Clear the display
     */
    void clear();

private:
    //==========================================================================
    // Phase I: Placeholder visualization
    juce::Label infoLabel;
    
    // Phase II: Will add:
    // - Waveform rendering from audio buffer
    // - Zoom and pan controls
    
    // Phase III: Will add:
    // - Spectrogram overlay
    // - Phoneme boundary markers
    // - Draggable edit points
    
    double displayStartTime { 0.0 };
    double displayEndTime { 1.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetailView)
};

} // namespace VoxScript
