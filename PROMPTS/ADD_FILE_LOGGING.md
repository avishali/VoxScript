# Add File-Based Logging to VoxScript

Since console output isn't visible, let's write to a log file.

---

## Step 1: Add Logging Helper

**Create:** `Source/util/VoxLogger.h`

```cpp
#pragma once
#include <juce_core/juce_core.h>
#include <fstream>
#include <mutex>

namespace VoxScript
{

class VoxLogger
{
public:
    static VoxLogger& getInstance()
    {
        static VoxLogger instance;
        return instance;
    }
    
    void log(const juce::String& message)
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!logFile.is_open())
            return;
            
        auto timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
        logFile << "[" << timestamp.toStdString() << "] " << message.toStdString() << std::endl;
        logFile.flush();
    }
    
private:
    VoxLogger()
    {
        auto logPath = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                           .getChildFile("Desktop/VoxScript_Debug.log");
        
        logFile.open(logPath.getFullPathName().toStdString(), std::ios::app);
        
        if (logFile.is_open())
        {
            log("========================================");
            log("VoxScript Session Started");
            log("========================================");
        }
    }
    
    ~VoxLogger()
    {
        if (logFile.is_open())
        {
            log("VoxScript Session Ended");
            logFile.close();
        }
    }
    
    std::ofstream logFile;
    std::mutex mutex;
};

// Convenience macro
#define VOXLOG(msg) VoxScript::VoxLogger::getInstance().log(msg)

} // namespace VoxScript
```

---

## Step 2: Update VoxScriptAudioSource.cpp

**File:** `Source/ara/VoxScriptAudioSource.cpp`

**Add at top:**
```cpp
#include "VoxScriptAudioSource.h"
#include "VoxScriptDocumentController.h"
#include "../transcription/AudioExtractor.h"
#include "../util/VoxLogger.h"  // ADD THIS
```

**Replace constructor:**
```cpp
VoxScriptAudioSource::VoxScriptAudioSource (juce::ARADocument* document,
                                           ARA::ARAAudioSourceHostRef hostRef)
    : juce::ARAAudioSource (document, hostRef)
{
    VOXLOG("========================================");
    VOXLOG("AUDIO SOURCE CREATED!!!");
    VOXLOG("========================================");
    
    ownerController = dynamic_cast<VoxScriptDocumentController*>(
        getDocumentController()
    );
    
    if (ownerController == nullptr)
    {
        VOXLOG("ERROR: Could not get DocumentController!");
    }
    else
    {
        VOXLOG("DocumentController obtained successfully");
    }
}
```

**Replace notifyPropertiesUpdated():**
```cpp
void VoxScriptAudioSource::notifyPropertiesUpdated() noexcept
{
    VOXLOG("========================================");
    VOXLOG("PROPERTIES UPDATED CALLED!!!");
    VOXLOG("Sample Rate: " + juce::String(getSampleRate()) + " Hz");
    VOXLOG("Channels: " + juce::String(getChannelCount()));
    VOXLOG("Duration: " + juce::String(getDurationInSeconds()) + " seconds");
    VOXLOG("Sample Access Enabled: " + juce::String(isSampleAccessEnabled() ? "YES" : "NO"));
    VOXLOG("========================================");
    
    if (!isSampleAccessEnabled())
    {
        VOXLOG("Sample access not available - skipping transcription");
        return;
    }
    
    VOXLOG("Launching transcription thread...");
    
    juce::Thread::launch([this]() {
        triggerTranscription();
    });
}
```

**Add triggerTranscription():**
```cpp
void VoxScriptAudioSource::triggerTranscription()
{
    VOXLOG("triggerTranscription() called");
    
    if (ownerController == nullptr)
    {
        VOXLOG("ERROR: No DocumentController - cannot transcribe");
        return;
    }
    
    VOXLOG("Calling AudioExtractor::extractToTempWAV()...");
    tempAudioFile = AudioExtractor::extractToTempWAV(this, "voxscript_");
    
    if (!tempAudioFile.existsAsFile())
    {
        VOXLOG("ERROR: Audio extraction failed - no temp file created");
        return;
    }
    
    VOXLOG("Extraction SUCCESS: " + tempAudioFile.getFullPathName());
    VOXLOG("File size: " + juce::String(tempAudioFile.getSize() / 1024) + " KB");
    
    ownerController->getWhisperEngine().addListener(this);
    
    VOXLOG("Starting WhisperEngine transcription...");
    ownerController->getWhisperEngine().transcribeAudioFile(tempAudioFile);
}
```

**Add Listener callbacks:**
```cpp
void VoxScriptAudioSource::transcriptionProgress(float progress)
{
    int percentage = juce::roundToInt(progress * 100.0f);
    VOXLOG("Transcription progress: " + juce::String(percentage) + "%");
}

void VoxScriptAudioSource::transcriptionComplete(VoxSequence sequence)
{
    VOXLOG("========================================");
    VOXLOG("TRANSCRIPTION COMPLETE!!!");
    VOXLOG("Segments: " + juce::String(sequence.segments.size()));
    VOXLOG("========================================");
    
    currentTranscription = sequence;
    transcriptionReady = true;
    
    if (tempAudioFile.existsAsFile())
    {
        VOXLOG("Deleting temp file: " + tempAudioFile.getFullPathName());
        tempAudioFile.deleteFile();
    }
    
    if (ownerController != nullptr)
    {
        ownerController->getWhisperEngine().removeListener(this);
    }
}

void VoxScriptAudioSource::transcriptionFailed(const juce::String& error)
{
    VOXLOG("========================================");
    VOXLOG("TRANSCRIPTION FAILED: " + error);
    VOXLOG("========================================");
    
    if (tempAudioFile.existsAsFile())
    {
        tempAudioFile.deleteFile();
    }
    
    if (ownerController != nullptr)
    {
        ownerController->getWhisperEngine().removeListener(this);
    }
}
```

---

## Step 3: Update CMakeLists.txt

Add VoxLogger to build:

```cmake
target_sources(VoxScript PRIVATE
    # ... existing files ...
    
    # Utilities
    Source/util/VoxLogger.h
)
```

---

## Step 4: Rebuild

```bash
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript

# Create util directory
mkdir -p Source/util

# Copy VoxLogger.h (create it with the code above)

# Rebuild
cd build-Debug
ninja
```

---

## Step 5: Test

1. Open Reaper
2. Drag audio file to track
3. Right-click → Extensions → VoxScript
4. **Check Desktop for `VoxScript_Debug.log`**

```bash
# Watch log file in real-time
tail -f ~/Desktop/VoxScript_Debug.log
```

---

## Expected Log Output

```
[2026-01-22 17:15:30] ========================================
[2026-01-22 17:15:30] VoxScript Session Started
[2026-01-22 17:15:30] ========================================
[2026-01-22 17:15:35] ========================================
[2026-01-22 17:15:35] AUDIO SOURCE CREATED!!!
[2026-01-22 17:15:35] ========================================
[2026-01-22 17:15:35] DocumentController obtained successfully
[2026-01-22 17:15:36] ========================================
[2026-01-22 17:15:36] PROPERTIES UPDATED CALLED!!!
[2026-01-22 17:15:36] Sample Rate: 44100 Hz
[2026-01-22 17:15:36] Channels: 2
[2026-01-22 17:15:36] Duration: 180 seconds
[2026-01-22 17:15:36] Sample Access Enabled: YES
[2026-01-22 17:15:36] ========================================
[2026-01-22 17:15:36] Launching transcription thread...
[2026-01-22 17:15:36] triggerTranscription() called
[2026-01-22 17:15:36] Calling AudioExtractor::extractToTempWAV()...
[2026-01-22 17:15:42] Extraction SUCCESS: /tmp/voxscript_abc123.wav
[2026-01-22 17:15:42] File size: 5760 KB
[2026-01-22 17:15:42] Starting WhisperEngine transcription...
[2026-01-22 17:15:45] Transcription progress: 25%
[2026-01-22 17:15:48] Transcription progress: 50%
[2026-01-22 17:15:51] Transcription progress: 75%
[2026-01-22 17:15:54] ========================================
[2026-01-22 17:15:54] TRANSCRIPTION COMPLETE!!!
[2026-01-22 17:15:54] Segments: 8
[2026-01-22 17:15:54] ========================================
```

This will tell us **exactly** what's happening (or not happening)!
