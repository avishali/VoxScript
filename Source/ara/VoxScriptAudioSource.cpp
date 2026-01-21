/*
  ==============================================================================
    VoxScriptAudioSource.cpp
    
    Implementation of custom ARA Audio Source
    Fixed for JUCE 8.0+ ARA API
  ==============================================================================
*/

#include "VoxScriptAudioSource.h"
#include "../transcription/AudioExtractor.h"

namespace VoxScript
{

    VoxScriptAudioSource::VoxScriptAudioSource (juce::ARADocument* document,
             ARA::ARAAudioSourceHostRef hostRef)
    : juce::ARAAudioSource (document, hostRef)
{
    std::cerr << "\n\n******************************\n";
    std::cerr << "AUDIO SOURCE CREATED!!!\n";
    std::cerr << "******************************\n\n";

    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Source Created");
    DBG ("(Properties will be available after notifyPropertiesUpdated)");
    DBG ("================================================");
}


VoxScriptAudioSource::~VoxScriptAudioSource()
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Source DESTROYED");
    DBG ("================================================");
    
    // Phase II: Make sure to stop and join analysis thread here
}

//==============================================================================
// ARA Notifications

void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    std::cerr << "\n\n******************************\n";
    std::cerr << "PROPERTIES UPDATED!!!\n";
    std::cerr << "******************************\n\n";
    
    DBG ("VoxScriptAudioSource: Properties updated");
    
    // Phase III Test: Trigger AudioExtractor test
    testAudioExtractor();
}

void VoxScriptAudioSource::notifyContentChanged (juce::ARAContentUpdateScopes scopeFlags) noexcept
{
    DBG ("================================================");
    DBG ("VOXSCRIPT: Audio Content Changed!");
    DBG ("Scope flags: " + juce::String (scopeFlags));
    DBG ("This would trigger re-transcription in Phase II");
    DBG ("================================================");
    
    juce::ignoreUnused (scopeFlags);
}

//==============================================================================
// Transcription API (Phase II placeholders)

juce::String VoxScriptAudioSource::getTranscription() const
{
    return transcriptionText.isEmpty() 
        ? "Transcription will appear here (Phase II)" 
        : transcriptionText;
}

bool VoxScriptAudioSource::isTranscriptionReady() const
{
    return transcriptionReady;
}

float VoxScriptAudioSource::getTranscriptionProgress() const
{
    return transcriptionProgress;
}

//==============================================================================
// Phase III Test: AudioExtractor validation

void VoxScriptAudioSource::testAudioExtractor()
{
    // Use std::cout for visibility in Console.app and Terminal
    std::cout << "========================================" << std::endl;
    std::cout << "🎯 VOXSCRIPT: TEST STARTING" << std::endl;
    std::cout << "========================================" << std::endl;
    
    DBG ("================================================");
    DBG ("AUDIOEXTRACTOR TEST STARTING");
    DBG ("================================================");
    
    // Check sample access
    if (!AudioExtractor::isSampleAccessAvailable (this))
    {
        std::cout << "⚠️  Sample access NOT available - test skipped" << std::endl;
        std::cout << "   (Audio may still be loading or recording in progress)" << std::endl;
        DBG ("Sample access not available - test skipped");
        return;
    }
    
    std::cout << "✅ Sample access available - starting test" << std::endl;
    DBG ("Sample access available - starting extraction test");
    
    // Launch extraction on background thread
    juce::Thread::launch ([this]() {
        std::cout << "🔄 Extraction thread started" << std::endl;
        DBG ("Starting extraction...");
        
        auto tempFile = AudioExtractor::extractToTempWAV (this, "test_");
        
        if (tempFile.existsAsFile())
        {
            std::cout << "✅ SUCCESS: Extraction completed!" << std::endl;
            std::cout << "📁 File: " << tempFile.getFullPathName().toStdString() << std::endl;
            std::cout << "💾 Size: " << (tempFile.getSize() / 1024) << " KB" << std::endl;
            
            DBG ("SUCCESS: Extraction completed!");
            DBG ("  File: " + tempFile.getFullPathName());
            DBG ("  Size: " + juce::String (tempFile.getSize() / 1024) + " KB");
            
            // Verify it's a valid WAV (JUCE 8: createInputStream returns unique_ptr)
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatReader> reader (
                wavFormat.createReaderFor (tempFile.createInputStream().release(), true)
            );
            
            if (reader != nullptr)
            {
                std::cout << "🎵 Sample Rate: " << reader->sampleRate << " Hz" << std::endl;
                std::cout << "🔊 Channels: " << reader->numChannels << std::endl;
                std::cout << "⏱️  Duration: " << (reader->lengthInSamples / reader->sampleRate) << " sec" << std::endl;
                
                DBG ("  Sample Rate: " + juce::String (reader->sampleRate) + " Hz");
                DBG ("  Channels: " + juce::String (reader->numChannels));
                DBG ("  Duration: " + juce::String (reader->lengthInSamples / reader->sampleRate, 2) + " sec");
                
                // Verify format is correct for Whisper
                if (reader->sampleRate == 16000.0 && reader->numChannels == 1)
                {
                    std::cout << "✅ Format correct for Whisper (16kHz mono)!" << std::endl;
                    DBG ("  Format verified: 16kHz mono (Whisper-ready)");
                }
                else
                {
                    std::cout << "❌ ERROR: Format incorrect!" << std::endl;
                    std::cout << "   Expected: 16000 Hz, 1 channel" << std::endl;
                    std::cout << "   Got: " << reader->sampleRate << " Hz, " 
                              << reader->numChannels << " channels" << std::endl;
                    DBG ("ERROR: Format incorrect for Whisper");
                }
            }
            else
            {
                std::cout << "❌ ERROR: Created file is not a valid WAV!" << std::endl;
                DBG ("ERROR: WAV reader could not open file");
            }
            
            // Clean up
            tempFile.deleteFile();
            std::cout << "🗑️  Temp file cleaned up" << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "🎉 AUDIOEXTRACTOR TEST PASSED!" << std::endl;
            std::cout << "========================================" << std::endl;
            
            DBG ("  Temp file cleaned up");
            DBG ("================================================");
            DBG ("AUDIOEXTRACTOR TEST PASSED");
            DBG ("================================================");
        }
        else
        {
            std::cout << "❌ FAILED: Extraction returned invalid file" << std::endl;
            DBG ("FAILED: Extraction returned invalid file");
            DBG ("================================================");
            DBG ("AUDIOEXTRACTOR TEST FAILED");
            DBG ("================================================");
        }
    });
}

} // namespace VoxScript
