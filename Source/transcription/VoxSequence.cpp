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

juce::ValueTree VoxSequence::toValueTree() const
{
    juce::ValueTree vt("SEQUENCE");
    vt.setProperty("duration", getTotalDuration(), nullptr);
    
    for (const auto& seg : segments)
    {
        juce::ValueTree segNode("SEGMENT");
        segNode.setProperty("start", seg.startTime, nullptr);
        segNode.setProperty("end", seg.endTime, nullptr);
        segNode.setProperty("text", seg.text, nullptr);
        
        // Serialize words
        for (const auto& w : seg.words)
        {
             juce::ValueTree wordNode("WORD");
             wordNode.setProperty("s", w.startTime, nullptr);
             wordNode.setProperty("e", w.endTime, nullptr);
             wordNode.setProperty("t", w.text, nullptr);
             wordNode.setProperty("c", w.confidence, nullptr);
             segNode.addChild(wordNode, -1, nullptr);
        }
        
        vt.addChild(segNode, -1, nullptr);
    }
    return vt;
}

bool VoxSequence::fromValueTree(const juce::ValueTree& vt)
{
    if (!vt.isValid() || vt.getType().toString() != "SEQUENCE")
        return false;
        
    clear();
    
    // Iterate over SEGMENT children
    for (const auto& segNode : vt)
    {
        if (segNode.getType().toString() == "SEGMENT")
        {
            VoxSegment s;
            s.startTime = segNode.getProperty("start");
            s.endTime = segNode.getProperty("end");
            s.text = segNode.getProperty("text");
            
            // Deserialize words
            for (const auto& wordNode : segNode)
            {
                if (wordNode.getType().toString() == "WORD")
                {
                    VoxWord w;
                    w.startTime = wordNode.getProperty("s");
                    w.endTime = wordNode.getProperty("e");
                    w.text = wordNode.getProperty("t");
                    w.confidence = wordNode.getProperty("c");
                    s.words.add(w);
                }
            }
            
            addSegment(s);
        }
    }
    
    return true;
}

} // namespace VoxScript
