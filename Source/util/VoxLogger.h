/*
  ==============================================================================
    VoxLogger.h
    
    Simple file-based logger for debugging VoxScript
    Writes to ~/Desktop/VoxScript_Debug.log
  ==============================================================================
*/

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
            log("========================================");
            log("VoxScript Session Ended");
            log("========================================");
            logFile.close();
        }
    }
    
    std::ofstream logFile;
    std::mutex mutex;
};

// Convenience macro
#define VOXLOG(msg) VoxScript::VoxLogger::getInstance().log(msg)

} // namespace VoxScript
