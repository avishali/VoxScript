# install_JUCE_ARA.cmake
# Helper module for JUCE ARA plugin installation

# This file is currently empty but included by CMakeLists.txt
# Future use: Custom ARA-specific installation rules

# For now, rely on JUCE's built-in COPY_PLUGIN_AFTER_BUILD which handles:
# - VST3 -> ~/Library/Audio/Plug-Ins/VST3/
# - AU -> ~/Library/Audio/Plug-Ins/Components/
# - AAX -> /Library/Application Support/Avid/Audio/Plug-Ins/
# - Standalone -> ~/Applications/

message(STATUS "ARA installation: Using JUCE built-in COPY_PLUGIN_AFTER_BUILD")
