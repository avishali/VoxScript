/*
  ==============================================================================
    VoxSequence.h
    
    Data structure for transcription results from Whisper
    Stores segments, words, and timing information
    
    Part of VoxScript Phase II: Transcription Engine
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace VoxScript
{

/**
 * @brief Represents a single word with timing and confidence
 */
struct VoxWord
{
    juce::String text;
    double startTime;   // seconds
    double endTime;     // seconds
    float confidence;   // 0.0 to 1.0
};

/**
 * @brief Represents a segment of transcribed text (usually a phrase or sentence)
 * 
 * Whisper outputs text in segments, each containing multiple words.
 * Segments are naturally delimited by pauses or speaker changes.
 */
struct VoxSegment
{
    juce::String text;
    double startTime;
    double endTime;
    juce::Array<VoxWord> words;
};

/**
 * @brief Complete transcription sequence for an audio source
 * 
 * This is the primary data structure for storing transcription results.
 * It's designed to be:
 * - Value-type (copyable, can be passed by value)
 * - Thread-safe (immutable once created)
 * - Serializable (Phase III: will add XML/binary serialization)
 * 
 * Phase II: Basic storage and retrieval
 * Phase III: Add edit tracking, undo/redo, serialization
 */
class VoxSequence
{
public:
    VoxSequence() = default;
    
    /**
     * Add a segment to the sequence
     * Segments should be added in chronological order
     */
    void addSegment (const VoxSegment& segment);
    
    /**
     * Clear all segments
     */
    void clear();
    
    /**
     * Get all segments
     */
    const juce::Array<VoxSegment>& getSegments() const;
    
    /**
     * Get the full text of the transcription
     * Concatenates all segment text with spaces
     */
    juce::String getFullText() const;
    
    /**
     * Get total word count across all segments
     */
    int getWordCount() const;
    
    /**
     * Get total duration of the transcription in seconds
     */
    double getTotalDuration() const;

private:
    juce::Array<VoxSegment> segments;
    
    JUCE_LEAK_DETECTOR (VoxSequence)
};

} // namespace VoxScript
