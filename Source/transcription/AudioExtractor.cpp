/*
  ==============================================================================
    AudioExtractor.cpp
    
    Implementation of audio extraction from ARA to WAV
    Fixed for JUCE 8.0+ ARA API using ARA SDK directly where needed
    
    Part of VoxScript Phase III: Automatic Transcription
    
    Copyright (c) 2025 MelechDSP - All rights reserved.
  ==============================================================================
*/

#include "AudioExtractor.h"
#include <ARA_Library/PlugIn/ARAPlug.h>

namespace VoxScript
{

AudioExtractor::AudioExtractor()
{
}

AudioExtractor::~AudioExtractor()
{
}

juce::File AudioExtractor::getTempDirectory()
{
    return juce::File::getSpecialLocation(juce::File::tempDirectory);
}

juce::File AudioExtractor::extractToTempWAV(juce::ARAAudioSource* araAudioSource, 
                                            const juce::String& prefix)
{
    if (araAudioSource == nullptr)
    {
        DBG("AudioExtractor: NULL audio source provided");
        return juce::File();
    }
    
    // Step 1: Check if sample access is available
    // NOTE: Sample access must be enabled on the main thread via DocumentController::enableAudioSourceSamplesAccess
    // before calling this method from a background thread.
    
    // Step 2: Get audio properties using ARA SDK API (inherited by juce::ARAAudioSource)
    auto sourceSampleRate = araAudioSource->getSampleRate();
    auto sourceChannels = (int)araAudioSource->getChannelCount();
    auto totalSamples = (juce::int64)araAudioSource->getSampleCount();
    
    DBG("AudioExtractor: Source format: " + 
        juce::String(sourceSampleRate) + " Hz, " +
        juce::String(sourceChannels) + " channels, " +
        juce::String(totalSamples) + " samples");
    
    if (totalSamples == 0 || juce::approximatelyEqual(sourceSampleRate, 0.0))
    {
        DBG("AudioExtractor: Invalid audio properties");
        return juce::File();
    }
    
    // Step 3: Create temp WAV file with unique name
    auto tempDir = getTempDirectory();
    auto tempFile = tempDir.getChildFile(prefix + juce::Uuid().toString() + ".wav");
    
    DBG("AudioExtractor: Creating temp file: " + tempFile.getFullPathName());
    
    // Step 4: Create WavAudioFormat writer (16kHz mono, 16-bit PCM)
    const double targetSampleRate = 16000.0;
    const int targetChannels = 1; // mono
    const int bitsPerSample = 16;
    
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(tempFile.createOutputStream());
    
    if (!fileStream)
    {
        DBG("AudioExtractor: Failed to create output stream");
        return juce::File();
    }
    
    juce::StringPairArray metadata;
    
    // Use the non-deprecated version of createWriterFor if possible
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(fileStream.get(),
                                  targetSampleRate,
                                  (unsigned int)targetChannels,
                                  bitsPerSample,
                                  metadata,
                                  0));
    
    if (!writer)
    {
        DBG("AudioExtractor: Failed to create WAV writer");
        return juce::File();
    }
    
    fileStream.release(); // Writer now owns the stream
    
    // Step 5-7: Downmix to mono, resample to 16kHz, and write in chunks
    const int chunkSize = 8192;
    juce::AudioBuffer<float> sourceBuffer(sourceChannels, chunkSize);
    juce::AudioBuffer<float> monoBuffer(1, chunkSize);
    
    // speedRatio is inputRate / outputRate (e.g. 48000 / 16000 = 3.0)
    const double speedRatio = sourceSampleRate / targetSampleRate;
    const double resampleRatio = 1.0 / speedRatio;
    
    // Use Lagrange interpolator for resampling (good speed/quality balance)
    juce::LagrangeInterpolator interpolator;
    
    // Create ARA Host Audio Reader
    ARA::PlugIn::HostAudioReader reader(araAudioSource);
    
    juce::int64 sourcePosition = 0;
    juce::int64 samplesWritten = 0;
    
    while (sourcePosition < totalSamples)
    {
        // Read chunk from source
        auto samplesToRead = (int)juce::jmin((juce::int64)chunkSize, totalSamples - sourcePosition);
        
        // Use readAudioSamples from HostAudioReader
        if (!reader.readAudioSamples(sourcePosition, samplesToRead, (void* const*)sourceBuffer.getArrayOfWritePointers()))
        {
            DBG("AudioExtractor: Failed to read samples at position " + juce::String(sourcePosition));
            tempFile.deleteFile();
            return juce::File();
        }
        
        // Step 5: Downmix to mono BEFORE resampling (50% CPU savings)
        monoBuffer.clear();
        for (int ch = 0; ch < sourceChannels; ++ch)
        {
            monoBuffer.addFrom(0, 0, sourceBuffer, ch, 0, samplesToRead, 1.0f / (float)sourceChannels);
        }
        
        // Step 6: Resample to 16kHz using Lagrange interpolation
        // Calculate exactly how many output samples to produce for this input chunk
        const int numOutputSamples = static_cast<int>(static_cast<double>(samplesToRead) * resampleRatio);
        juce::AudioBuffer<float> resampledBuffer(1, numOutputSamples + 10); // buffer with a bit of padding
        resampledBuffer.clear();
        
        interpolator.process(
            speedRatio,
            monoBuffer.getReadPointer(0),
            resampledBuffer.getWritePointer(0),
            numOutputSamples);
        
        // Step 7: Write resampled samples to WAV file
        if (numOutputSamples > 0)
        {
            if (!writer->writeFromAudioSampleBuffer(resampledBuffer, 0, numOutputSamples))
            {
                DBG("AudioExtractor: Failed to write samples");
                tempFile.deleteFile();
                return juce::File();
            }
            samplesWritten += numOutputSamples;
        }
        
        sourcePosition += samplesToRead;
    }
    
    // Step 8: Close writer and return file
    writer.reset();
    
    DBG("AudioExtractor: Successfully wrote " + juce::String(samplesWritten) + 
        " samples to " + tempFile.getFullPathName());
    
    juce::ignoreUnused(samplesWritten);
    
    return tempFile;
}

} // namespace VoxScript
