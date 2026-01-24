/*
  ==============================================================================
    TranscriptionJobQueue.cpp
    
    Part of VoxScript Mission 3: Isolate Whisper
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "TranscriptionJobQueue.h"

namespace VoxScript
{

TranscriptionJobQueue::TranscriptionJobQueue()
    : juce::Thread("TranscriptionWorker")
{
}

TranscriptionJobQueue::~TranscriptionJobQueue()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopRequested = true;
    }
    
    cancelAll();
    queueCV.notify_all();
    stopThread(4000);
}

void TranscriptionJobQueue::initialise(VoxScriptDocumentStore* store, WhisperEngine* engine)
{
    documentStore = store;
    whisperEngine = engine;
    startThread();
}

void TranscriptionJobQueue::setCompletionCallback(std::function<void(AudioSourceID)> callback)
{
    completionCallback = callback;
}

void TranscriptionJobQueue::enqueueTranscription(const TranscriptionJob& job)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
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
    
    if (whisperEngine)
        whisperEngine->cancelTranscription();
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
    while (!threadShouldExit())
    {
        TranscriptionJob currentJob;
        bool hasJob = false;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return stopRequested || !jobQueue.empty(); });
            
            if (stopRequested && jobQueue.empty())
                break;
                
            if (!jobQueue.empty())
            {
                currentJob = jobQueue.front();
                jobQueue.pop_front();
                hasJob = true;
            }
        }
        
        if (hasJob && !threadShouldExit() && whisperEngine != nullptr)
        {
            // Execute synchronous transcription
            VoxSequence result;
            
            if (currentJob.sourcePtr != nullptr)
            {
                 result = whisperEngine->processSync(currentJob.sourcePtr);
            }
            else
            {
                 result = whisperEngine->processSync(currentJob.audioFile);
            }
            
            // Post result if valid and not cancelled (empty result usually means failed/cancelled)
            if (result.getSegmentCount() > 0 && !threadShouldExit())
            {
                juce::MessageManager::callAsync([this, id = currentJob.sourceID, res = result]()
                {
                    if (documentStore)
                        documentStore->updateTranscription(id, res);
                        
                    if (completionCallback)
                        completionCallback(id);
                });
            }
        }
    }
}

} // namespace VoxScript
