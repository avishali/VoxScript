/*
  ==============================================================================
    TranscriptionJobQueue.h
    
    Manages a single-threaded queue for background transcription jobs.
    Ensures safe, serialized execution of Whisper inference and 
    reliable result publication to the DocumentStore.
    
    Part of VoxScript Mission 3: Isolate Whisper
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../transcription/WhisperEngine.h"
#include "../ara/VoxScriptDocumentStore.h"
#include <deque>
#include <mutex>
#include <condition_variable>

namespace VoxScript
{

/**
 * @brief Represents a single transcription task
 */
struct TranscriptionJob
{
    AudioSourceID sourceID;
    juce::File audioFile; // Optional: for file-based transcription
    juce::ARAAudioSource* sourcePtr = nullptr; // Optional: process directly from ARA source
    
    // Equality operator for cancellation logic
    bool operator== (const TranscriptionJob& other) const
    {
        return sourceID == other.sourceID;
    }
};

/**
 * @brief Single-threaded job queue for Whisper transcription.
 * 
 * Owned by VoxScriptDocumentController.
 * Consumes jobs from a thread-safe queue and executes them using WhisperEngine.
 * Publishes results to VoxScriptDocumentStore on the Message Thread.
 */
class TranscriptionJobQueue : public juce::Thread
{
public:
    TranscriptionJobQueue();
    ~TranscriptionJobQueue() override;

    void initialise(VoxScriptDocumentStore* store, WhisperEngine* engine);

    /**
     * @brief Set callback to be invoked on the Message Thread when a transcription completes.
     */
    void setCompletionCallback(std::function<void(AudioSourceID)> callback);

    /**
     * @brief Enqueue a transcription job for an audio source.
     * Thread-safe.
     */
    void enqueueTranscription(const TranscriptionJob& job);

    /**
     * @brief Cancel all pending jobs.
     * Also signals the current job to stop if possible.
     */
    void cancelAll();

    /**
     * @brief Cancel pending jobs for a specific audio source.
     * Useful when an audio source is deleted.
     */
    void cancelForAudioSource(AudioSourceID sourceID);

    // juce::Thread override
    void run() override;

private:
    VoxScriptDocumentStore* documentStore = nullptr;
    WhisperEngine* whisperEngine = nullptr;

    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::deque<TranscriptionJob> jobQueue;
    
    std::function<void(AudioSourceID)> completionCallback;
    
    bool stopRequested = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TranscriptionJobQueue)
};

} // namespace VoxScript
