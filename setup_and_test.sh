#!/bin/bash
# AudioExtractor Quick Test Setup Script
# Run this on your Mac to integrate and test AudioExtractor

set -e

PROJECT_DIR="/Users/avishaylidani/DEV/GitHubRepo/VoxScript"

echo "==========================================="
echo "🚀 AudioExtractor Quick Test Setup"
echo "==========================================="
echo ""

cd "$PROJECT_DIR"

# Verify files were created
echo "Step 1: Verifying AudioExtractor files..."
if [ ! -f "Source/transcription/AudioExtractor.h" ]; then
    echo "❌ AudioExtractor.h not found!"
    echo "Expected: Source/transcription/AudioExtractor.h"
    exit 1
fi

if [ ! -f "Source/transcription/AudioExtractor.cpp" ]; then
    echo "❌ AudioExtractor.cpp not found!"
    echo "Expected: Source/transcription/AudioExtractor.cpp"
    exit 1
fi

echo "✅ AudioExtractor files found"

# Update CMakeLists.txt
echo ""
echo "Step 2: Updating CMakeLists.txt..."

# Backup
cp CMakeLists.txt CMakeLists.txt.backup

# Add AudioExtractor to build (after VoxSequence.h)
perl -i -pe 's/(Source\/transcription\/VoxSequence\.h)/$1\n        # Phase III: Audio extraction\n        Source\/transcription\/AudioExtractor.cpp\n        Source\/transcription\/AudioExtractor.h/' CMakeLists.txt

echo "✅ CMakeLists.txt updated"

# Update VoxScriptAudioSource.h
echo ""
echo "Step 3: Updating VoxScriptAudioSource.h..."

# Backup
cp Source/ara/VoxScriptAudioSource.h Source/ara/VoxScriptAudioSource.h.backup

# Add test method declaration before private:
perl -i -pe 's/(\s+private:)/    \/\/ Phase III Test\n    void testAudioExtractor();\n\n$1/' Source/ara/VoxScriptAudioSource.h

echo "✅ VoxScriptAudioSource.h updated"

# Update VoxScriptAudioSource.cpp
echo ""
echo "Step 4: Updating VoxScriptAudioSource.cpp..."

# Backup
cp Source/ara/VoxScriptAudioSource.cpp Source/ara/VoxScriptAudioSource.cpp.backup

# Add include after the header include
perl -i -pe 's/(#include "VoxScriptAudioSource.h")/$1\n#include "..\/transcription\/AudioExtractor.h"/' Source/ara/VoxScriptAudioSource.cpp

# Add test method at the end
cat >> Source/ara/VoxScriptAudioSource.cpp << 'TESTCODE'

//==============================================================================
// Phase III Test: AudioExtractor validation

void VoxScriptAudioSource::testAudioExtractor()
{
    DBG ("================================================");
    DBG ("AUDIOEXTRACTOR TEST STARTING");
    DBG ("================================================");
    
    // Check sample access
    if (!AudioExtractor::isSampleAccessAvailable (this))
    {
        DBG ("⚠️ Sample access not available - test skipped");
        DBG ("   (This is normal during recording or before audio is loaded)");
        return;
    }
    
    DBG ("✅ Sample access available - starting extraction test");
    
    // Launch extraction on background thread
    juce::Thread::launch ([this]() {
        DBG ("🔄 Starting extraction...");
        
        auto tempFile = AudioExtractor::extractToTempWAV (this, "test_");
        
        if (tempFile.existsAsFile())
        {
            DBG ("✅ SUCCESS: Extraction completed!");
            DBG ("  📁 File: " + tempFile.getFullPathName());
            DBG ("  💾 Size: " + juce::String (tempFile.getSize() / 1024) + " KB");
            
            // Verify it's a valid WAV
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatReader> reader (
                wavFormat.createReaderFor (tempFile.createInputStream(), true)
            );
            
            if (reader != nullptr)
            {
                DBG ("  🎵 Sample Rate: " + juce::String (reader->sampleRate) + " Hz");
                DBG ("  🔊 Channels: " + juce::String (reader->numChannels));
                DBG ("  ⏱️  Duration: " + juce::String (reader->lengthInSamples / reader->sampleRate, 2) + " sec");
                
                // Verify Whisper requirements
                bool formatOK = true;
                if (reader->sampleRate != 16000.0)
                {
                    DBG ("  ❌ ERROR: Sample rate should be 16000 Hz!");
                    formatOK = false;
                }
                if (reader->numChannels != 1)
                {
                    DBG ("  ❌ ERROR: Should be mono (1 channel)!");
                    formatOK = false;
                }
                
                if (formatOK)
                {
                    DBG ("  ✅ Format correct for Whisper (16kHz mono)");
                }
            }
            else
            {
                DBG ("  ❌ ERROR: Created file is not a valid WAV!");
            }
            
            // Clean up
            tempFile.deleteFile();
            DBG ("  🗑️  Temp file cleaned up");
            DBG ("================================================");
            DBG ("🎉 AUDIOEXTRACTOR TEST PASSED");
            DBG ("================================================");
        }
        else
        {
            DBG ("❌ FAILED: Extraction returned invalid file");
            DBG ("================================================");
            DBG ("💥 AUDIOEXTRACTOR TEST FAILED");
            DBG ("================================================");
        }
    });
}
TESTCODE

# Update notifyPropertiesUpdated to call test
perl -i -pe 'BEGIN{undef $/;} s/void VoxScriptAudioSource::notifyPropertiesUpdated\(\) noexcept\s*\{[^}]*\}/void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept\n{\n    DBG ("VoxScriptAudioSource: Properties updated");\n    \n    \/\/ Phase III Test: Trigger AudioExtractor test\n    testAudioExtractor();\n}/s' Source/ara/VoxScriptAudioSource.cpp

echo "✅ VoxScriptAudioSource.cpp updated"

# Build
echo ""
echo "Step 5: Building VoxScript..."
echo ""

cd build-Debug

# Clean build to ensure all changes are picked up
rm -f CMakeCache.txt
cmake .. -G Ninja

# Build
ninja -v 2>&1 | tee build.log

if [ $? -eq 0 ]; then
    echo ""
    echo "==========================================="
    echo "✅ BUILD SUCCESSFUL!"
    echo "==========================================="
    echo ""
    echo "Plugin ready at:"
    echo "  VST3: build-Debug/VoxScript_artefacts/Debug/VST3/VoxScript.vst3"
    echo ""
    echo "Next steps:"
    echo "  1. Open Reaper"
    echo "  2. Add VoxScript as ARA extension to a track"
    echo "  3. Drag audio file into the track"
    echo "  4. Check Reaper console for test output"
    echo ""
    echo "To view console in Reaper:"
    echo "  View → Monitoring → Show Console"
    echo ""
else
    echo ""
    echo "==========================================="
    echo "❌ BUILD FAILED"
    echo "==========================================="
    echo ""
    echo "Check build.log for errors"
    exit 1
fi
