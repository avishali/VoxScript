# ==============================================================================
# FetchWhisper.cmake
#
# Fetch and configure whisper.cpp library for VoxScript Phase II
#
# Copyright (c) 2025 MelechDSP - All rights reserved.
# ==============================================================================

include(FetchContent)

message(STATUS "Configuring whisper.cpp...")

# Declare whisper.cpp dependency
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.5.4  # Use specific stable version
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp
)

# Configure whisper.cpp build options
# Disable everything we don't need for faster builds
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Build whisper tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Build whisper examples" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "Build whisper server" FORCE)
# Android examples are in 'examples/whisper.android' and are excluded
# because WHISPER_BUILD_EXAMPLES is OFF.
# Java bindings are not built by default in the main CMakeLists.txt.

# Enable Metal acceleration on macOS (optional, but recommended)
if(APPLE)
    set(WHISPER_METAL ON CACHE BOOL "Enable Metal acceleration" FORCE)
    message(STATUS "whisper.cpp: Metal acceleration enabled")
endif()

# Fetch the content (download if not already present)
FetchContent_MakeAvailable(whisper)

# Create alias target for easier linking
if(TARGET whisper)
    add_library(whisper::whisper ALIAS whisper)
    message(STATUS "whisper.cpp: Successfully configured")
else()
    message(FATAL_ERROR "whisper.cpp: Failed to create target")
endif()

# Log configuration
message(STATUS "whisper.cpp source: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp")
