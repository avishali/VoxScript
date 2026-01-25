/*
  ==============================================================================
    AudioExtractor.cpp
    Created: 21 Jan 2026
    Author: VoxScript Team (with Gemini 2.0 Flash research)
    
    Implementation Notes:
    - Uses juce::ARAAudioSourceReader as the single source of truth
    - Downmixes to mono FIRST, then resamples to 16kHz (CPU optimization)
    - Uses Lagrange interpolation for speed/quality balance on speech
    - Thread-safe: Creates reader in local scope for Steinberg hosts
  ==============================================================================
*/

#include "AudioExtractor.h"

namespace VoxScript
{

//==============================================================================
// Public API

juce::File AudioExtractor::extractToTempWAV (juce::ARAAudioSource* araSource, 
                                             const juce::String& tempFilePrefix)
{
    // 1. Validate Input
    //    Ensure sample access is enabled before attempting to read
    if (araSource == nullptr || !araSource->isSampleAccessEnabled())
    {
        DBG ("AudioExtractor: Source is null or sample access not enabled");
        return juce::File();
    }

    // 2. Create Reader (THREAD SAFETY CRITICAL)
    //    Instantiate reader in LOCAL SCOPE to satisfy Steinberg/Nuendo 
    //    threading requirements. Passing reader across threads causes glitches.
    auto reader = std::make_unique<juce::ARAAudioSourceReader> (araSource);

    if (reader == nullptr || reader->lengthInSamples == 0)
    {
        DBG ("AudioExtractor: Failed to create reader or source is empty");
        return juce::File();
    }

    const double sourceRate = reader->sampleRate;
    const int numSourceChannels = static_cast<int> (reader->numChannels);
    const int64 totalSourceSamples = reader->lengthInSamples;

    DBG ("AudioExtractor: Starting extraction");
    DBG ("  Source: " + juce::String (sourceRate) + " Hz, " + 
         juce::String (numSourceChannels) + " ch, " + 
         juce::String (totalSourceSamples) + " samples");
    DBG ("  Target: 16000 Hz, 1 ch, 16-bit PCM");

    // 3. Prepare Temp File
    juce::File tempFile = getUniqueTempFile (tempFilePrefix);
    
    // Create file output stream
    std::unique_ptr<juce::FileOutputStream> fileStream (tempFile.createOutputStream());
    if (fileStream == nullptr || !fileStream->openedOk())
    {
        DBG ("AudioExtractor: Failed to open temp file: " + tempFile.getFullPathName());
        return juce::File();
    }

    // 4. Configure WAV Writer
    //    Output: 16kHz, Mono, 16-bit PCM (Whisper requirement)
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (
            fileStream.get(), 
            TARGET_SAMPLE_RATE,
            TARGET_CHANNELS,
            TARGET_BIT_DEPTH,
            {},    // Metadata
            0      // Quality hint
        )
    );

    if (writer == nullptr)
    {
        DBG ("AudioExtractor: Failed to create WAV writer");
        return juce::File();
    }

    // Transfer stream ownership to writer
    fileStream.release();

    // 5. Setup Processing Buffers
    //    Calculate resampling parameters
    const double resampleRatio = TARGET_SAMPLE_RATE / sourceRate;
    const int destBlockSize = static_cast<int> (CHUNK_SIZE * resampleRatio) + 32; // +32 safety margin

    // Buffer A: Raw multi-channel data from host (source rate)
    juce::AudioBuffer<float> sourceBuffer (numSourceChannels, CHUNK_SIZE);
    
    // Buffer B: Downmixed mono (source rate)
    //    OPTIMIZATION: Downmix BEFORE resampling saves ~50% CPU
    juce::AudioBuffer<float> monoSourceBuffer (1, CHUNK_SIZE);

    // Buffer C: Resampled mono (16kHz)
    juce::AudioBuffer<float> resampledBuffer (1, destBlockSize);

    // Resampler: Lagrange is ideal for speech (balance of speed/quality)
    juce::LagrangeInterpolator resampler;

    // 6. Processing Loop
    int64 samplesRead = 0;
    bool aborted = false;

    while (samplesRead < totalSourceSamples && !aborted)
    {
        // A. Validate Source (detect live edits)
        //    If user modifies audio in DAW during extraction, abort safely
        if (!reader->isValid())
        {
            DBG ("AudioExtractor: Source invalidated during read (user edit). Aborting.");
            aborted = true;
            break;
        }

        // B. Calculate chunk size for this iteration
        const int numToRead = static_cast<int> (
            juce::jmin (static_cast<int64> (CHUNK_SIZE), totalSourceSamples - samplesRead)
        );

        // C. Read from ARA host
        bool readSuccess = reader->read (
            &sourceBuffer,
            0,                              // Destination start offset
            numToRead,                      // Number of samples to read
            samplesRead,                    // Source start position
            true,                           // Read left channel
            numSourceChannels > 1           // Read right channel (if exists)
        );

        if (!readSuccess)
        {
            DBG ("AudioExtractor: Read operation failed at sample " + juce::String (samplesRead));
            aborted = true;
            break;
        }

        // D. Downmix to Mono
        //    Formula: Mono = (L + R) / 2
        monoSourceBuffer.clear();
        monoSourceBuffer.copyFrom (0, 0, sourceBuffer.getReadPointer (0), numToRead);

        if (numSourceChannels > 1)
        {
            // Add right channel and average
            monoSourceBuffer.addFrom (0, 0, sourceBuffer.getReadPointer (1), numToRead);
            monoSourceBuffer.applyGain (0, 0, numToRead, 0.5f);
        }

        // E. Resample (Source Rate → 16kHz)
        int numOutputSamples = resampler.process (
            resampleRatio,
            monoSourceBuffer.getReadPointer (0),
            resampledBuffer.getWritePointer (0),
            numToRead
        );

        // F. Write to Disk
        if (numOutputSamples > 0)
        {
            writer->writeFromAudioSampleBuffer (resampledBuffer, 0, numOutputSamples);
        }

        samplesRead += numToRead;
    }

    // 7. Finalize
    writer.reset();

    // Check if extraction completed successfully
    if (aborted || samplesRead < totalSourceSamples)
    {
        DBG ("AudioExtractor: Extraction incomplete. Cleaning up temp file.");
        tempFile.deleteFile();
        return juce::File();
    }

    DBG ("AudioExtractor: Extraction complete - " + tempFile.getFileName());
    DBG ("  Output size: " + juce::String (tempFile.getSize() / 1024) + " KB");
    
    return tempFile;
}

bool AudioExtractor::isSampleAccessAvailable (juce::ARAAudioSource* araSource)
{
    if (araSource == nullptr)
        return false;
    
    return araSource->isSampleAccessEnabled();
}

int64 AudioExtractor::getExpectedOutputSize (juce::ARAAudioSource* araSource)
{
    if (araSource == nullptr || !araSource->isSampleAccessEnabled())
        return 0;
    
    // Create temporary reader to get source properties
    auto reader = std::make_unique<juce::ARAAudioSourceReader> (araSource);
    
    if (reader == nullptr || reader->lengthInSamples == 0)
        return 0;
    
    // Calculate output size at 16kHz
    const double sourceRate = reader->sampleRate;
    const int64 sourceSamples = reader->lengthInSamples;
    const double resampleRatio = TARGET_SAMPLE_RATE / sourceRate;
    
    return static_cast<int64> (sourceSamples * resampleRatio);
}

//==============================================================================
// Private Implementation

juce::File AudioExtractor::getUniqueTempFile (const juce::String& prefix)
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
    auto uniqueName = prefix + juce::Uuid().toString() + ".wav";
    return tempDir.getChildFile (uniqueName);
}

} // namespace VoxScript
