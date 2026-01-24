#pragma once
#include <JuceHeader.h>

namespace VoxScript
{

// Restored VoxWord for compatibility with WhisperEngine
struct VoxWord {
    double startTime = 0.0;
    double endTime = 0.0;
    juce::String text;
    float confidence = 0.0f;
};

struct VoxSegment
{
    double startTime = 0.0;
    double endTime = 0.0;
    juce::String text;
    juce::Array<VoxWord> words;
};

class VoxSequence
{
public:
    VoxSequence() = default;
    
    // Existing API (implemented in VoxSequence.cpp)
    const juce::Array<VoxSegment>& getSegments() const;
    void addSegment(const VoxSegment& segment);
    void clear();
    
    // Restored methods (implemented in VoxSequence.cpp)
    juce::String getFullText() const;
    int getWordCount() const;
    double getTotalDuration() const;

    // User's requested helper (adapted to use correct fields)
    void addSegment(double start, double end, const juce::String& text)
    {
        VoxSegment s;
        s.startTime = start;
        s.endTime = end;
        s.text = text;
        addSegment(s);
    }
    
    // Serialization
    juce::ValueTree toValueTree() const;
    bool fromValueTree(const juce::ValueTree& vt);

private:
    juce::Array<VoxSegment> segments;
};

} // namespace VoxScript
