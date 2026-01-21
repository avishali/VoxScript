/*
  ==============================================================================
    DetailView.cpp
    
    Implementation of Detail View
  ==============================================================================
*/

#include "DetailView.h"

namespace VoxScript
{

DetailView::DetailView()
{
    // Setup info label
    infoLabel.setText ("Detail View (Signal Layer)\n\nWaveform + Spectrogram + Phoneme Overlay",
                      juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    infoLabel.setFont (juce::FontOptions (14.0f));
    infoLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (infoLabel);
}

DetailView::~DetailView()
{
}

//==============================================================================
// Component

void DetailView::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff252525));  // Slightly lighter than Script View
    
    // Border
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawRect (getLocalBounds(), 1);
    
    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
    g.drawText ("Detail View (Signal Layer)", 10, 5, getWidth() - 20, 25,
                juce::Justification::centredLeft);
    
    // Placeholder waveform visualization
    auto bounds = getLocalBounds().reduced (20).withTrimmedTop (30);
    
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (bounds);
    
    // Draw center line
    g.setColour (juce::Colour (0xff3a3a3a));
    auto centerY = bounds.getCentreY();
    g.drawLine (bounds.getX(), centerY, bounds.getRight(), centerY, 1.0f);
    
    // Draw placeholder waveform shape
    g.setColour (juce::Colour (0xff4a9eff).withAlpha (0.5f));
    juce::Path waveformPath;
    bool started = false;
    
    for (int x = bounds.getX(); x < bounds.getRight(); x += 5)
    {
        auto freq = 0.05f;
        auto amplitude = 30.0f * (1.0f - std::abs (static_cast<float> (x - bounds.getCentreX()) / bounds.getWidth()));
        auto y = centerY + amplitude * std::sin (freq * x);
        
        if (!started)
        {
            waveformPath.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            waveformPath.lineTo (x, y);
        }
    }
    
    g.strokePath (waveformPath, juce::PathStrokeType (2.0f));
    
    // Phase marker labels (placeholder)
    g.setColour (juce::Colours::lightgreen);
    g.setFont (juce::FontOptions (12.0f));
    auto labelY = bounds.getBottom() - 20;
    g.drawText ("/h/", bounds.getX() + 20, labelY, 40, 20, juce::Justification::centred);
    g.drawText ("/e/", bounds.getX() + 80, labelY, 40, 20, juce::Justification::centred);
    g.drawText ("/l/", bounds.getX() + 140, labelY, 40, 20, juce::Justification::centred);
    g.drawText ("/o/", bounds.getX() + 200, labelY, 40, 20, juce::Justification::centred);
}

void DetailView::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (30);  // Reserve for title
    
    infoLabel.setBounds (bounds.removeFromBottom (40));
}

//==============================================================================
// Audio Display API

void DetailView::setAudioData (const juce::AudioBuffer<float>& buffer)
{
    // Phase II: Store buffer and render waveform
    juce::ignoreUnused (buffer);
    
    repaint();
}

void DetailView::setTimeRange (double startTime, double endTime)
{
    displayStartTime = startTime;
    displayEndTime = endTime;
    
    repaint();
}

void DetailView::clear()
{
    repaint();
}

} // namespace VoxScript
