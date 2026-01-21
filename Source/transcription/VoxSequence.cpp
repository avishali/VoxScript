/*
  ==============================================================================
    VoxSequence.cpp
    
    Implementation of VoxSequence transcription data structure
    
    Part of VoxScript Phase II: Transcription Engine
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "VoxSequence.h"

namespace VoxScript
{

void VoxSequence::addSegment (const VoxSegment& segment)
{
    segments.add (segment);
}

void VoxSequence::clear()
{
    segments.clear();
}

const juce::Array<VoxSegment>& VoxSequence::getSegments() const
{
    return segments;
}

juce::String VoxSequence::getFullText() const
{
    juce::String result;
    
    for (int i = 0; i < segments.size(); ++i)
    {
        result += segments[i].text;
        
        // Add space between segments (but not after last segment)
        if (i < segments.size() - 1)
            result += " ";
    }
    
    return result;
}

int VoxSequence::getWordCount() const
{
    int count = 0;
    
    for (const auto& segment : segments)
        count += segment.words.size();
    
    return count;
}

double VoxSequence::getTotalDuration() const
{
    if (segments.isEmpty())
        return 0.0;
    
    // Duration from start of first segment to end of last segment
    double start = segments.getFirst().startTime;
    double end = segments.getLast().endTime;
    
    return end - start;
}

} // namespace VoxScript
