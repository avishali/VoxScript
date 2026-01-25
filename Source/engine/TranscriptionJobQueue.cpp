/*
  ==============================================================================
    TranscriptionJobQueue.cpp
    
    Part of VoxScript Mission 3: Isolate Whisper
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "TranscriptionJobQueue.h"
#include "../transcription/WhisperEngine.h"

namespace VoxScript
{

TranscriptionJobQueue::TranscriptionJobQueue()
    : juce::Thread("TranscriptionWorker")
{
    aliveFlag = std::make_shared<std::atomic<bool>>(true);
}

TranscriptionJobQueue::~TranscriptionJobQueue()
{
    if (aliveFlag)
        aliveFlag->store(false);

    // Requirement 1: Shutdown sequence
    signalThreadShouldExit();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopRequested = true;
    }

    cancelAll();
    queueCV.notify_all();
    stopThread(4000);
}

void TranscriptionJobQueue::initialise(VoxScriptDocumentStore* store)
{
    documentStore = store;
    startThread();
}

void TranscriptionJobQueue::setCompletionCallback(std::function<void(AudioSourceID)> callback)
{
    completionCallback = callback;
}

void TranscriptionJobQueue::enqueueTranscription(const TranscriptionJob& job)
{
    // Requirement 3: Check exit flags
    if (threadShouldExit())
        return;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (stopRequested)
            return;

        // Remove existing pending jobs for this source
        for (auto it = jobQueue.begin(); it != jobQueue.end(); )
        {
            if (it->sourceID == job.sourceID)
                it = jobQueue.erase(it);
            else
                ++it;
        }
        
        jobQueue.push_back(job);
    }
    queueCV.notify_one();
}

void TranscriptionJobQueue::cancelAll()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        jobQueue.clear();
    }
    
    // Note: WhisperEngine ownership is now local to the worker thread.
    // We cannot call cancelTranscription() directly.
    // Clearing the queue prevents future jobs. 
    // To stop the current job, we rely on the thread stopping.
}

void TranscriptionJobQueue::cancelForAudioSource(AudioSourceID sourceID)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    
    for (auto it = jobQueue.begin(); it != jobQueue.end(); )
    {
        if (it->sourceID == sourceID)
            it = jobQueue.erase(it);
        else
            ++it;
    }
    
    // Note: We don't cancel valid running jobs for specific ID easily here,
    // but clearing the queue prevents future work.
}

void TranscriptionJobQueue::run()
{
    // Requirement 2: Initialize WhisperEngine once when thread starts
    auto whisper = std::make_unique<WhisperEngine>();

    while (!threadShouldExit())
    {
        TranscriptionJob currentJob;
        bool hasJob = false;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return stopRequested || !jobQueue.empty(); });
            
            // Requirement 2: Check immediately after waking
            if (threadShouldExit() || stopRequested)
                break;
                
            if (!jobQueue.empty())
            {
                currentJob = jobQueue.front();
                jobQueue.pop_front();
                hasJob = true;
            }
        }
        
        // Requirement 2: Check before processing
        if (threadShouldExit() || stopRequested)
            break;
        
        if (hasJob && !threadShouldExit() && whisper != nullptr)
        {
            // Execute synchronous transcription
            VoxSequence result;
            
            // ALWAYS process from file (safety)
            if (currentJob.audioFile.existsAsFile())
            {
                 // Use local whisper instance
                 result = whisper->processSync(currentJob.audioFile);
                 
                 // Cleanup temp file
                 currentJob.audioFile.deleteFile();
            }
            
            // Post result if valid and not cancelled (empty result usually means failed/cancelled)
            if (result.getWordCount() > 0 && !threadShouldExit())
            {
                auto alive = aliveFlag;
                auto* storePtr = documentStore;
                auto cb = completionCallback;
                auto id = currentJob.sourceID;
                auto res = result;

                juce::MessageManager::callAsync([alive, storePtr, cb, id, res]() mutable
                {
                    if (!alive || !alive->load())
                        return;

                    if (storePtr)
                        storePtr->updateTranscription(id, res);
                        
                    if (cb)
                        cb(id);
                });
            }
        }
    }
    // WhisperEngine destroyed automatically as unique_ptr goes out of scope here
}

} // namespace VoxScript
