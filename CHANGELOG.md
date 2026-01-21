# VoxScript Changelog

All notable changes to VoxScript will be documented in this file.

## [Unreleased]

### Phase I: ARA Skeleton (Current)

#### Added - 2025-01-20
- Initial project structure based on melechdsp-hq template
- CMakeLists.txt with ARA2 support enabled
- Core ARA2 classes:
  - `VoxScriptDocumentController` - Central ARA session manager
  - `VoxScriptAudioSource` - Audio file wrapper with transcription API
  - `VoxScriptPlaybackRenderer` - Audio processing with edit application
- Main plugin classes:
  - `VoxScriptAudioProcessor` - ARA-enabled processor
  - `VoxScriptAudioProcessorEditor` - Dual-view UI container
- UI components:
  - `ScriptView` - Semantic text editing layer (placeholder)
  - `DetailView` - Signal-level waveform view (placeholder)
- Build system:
  - CMake configuration for JUCE 8
  - Dev mode (fast iteration) and release mode
  - macOS build script
- Documentation:
  - Comprehensive README with architecture
  - Phase roadmap aligned with Product Definition Document
  - .gitignore for clean repository

#### Technical Details
- Plugin code: VxSc
- Manufacturer code: Melc (MelechDSP)
- ARA content types: TempoAdjustment, Notes
- ARA analysis types: Lyrics (custom)
- Formats: VST3, AU, AAX, Standalone
- Target DAWs: Logic Pro, Studio One, Reaper, Pro Tools

#### Next Steps
- [ ] Test plugin loads in Logic Pro X
- [ ] Test plugin loads in Studio One
- [ ] Test plugin loads in Reaper
- [ ] Test plugin loads in Pro Tools
- [ ] Verify ARA callbacks are triggered
- [ ] Test state persistence across project save/load

## Phase II: Transcription Engine (Completed - 2025-01-21)

### Added
- **whisper.cpp Integration**: Full integration of whisper.cpp library via CMake FetchContent
  - FetchWhisper.cmake: Automated dependency management
  - Metal acceleration enabled on macOS for improved performance
  - Model path: `~/Library/Application Support/VoxScript/models/ggml-base.en.bin`
  
- **WhisperEngine Class**: Background transcription processing
  - Runs on dedicated background thread (juce::Thread)
  - Non-blocking audio file transcription
  - Progress callbacks with percentage updates
  - Thread-safe listener interface
  - Automatic resampling to 16kHz mono for whisper
  - Graceful error handling with user-friendly messages
  
- **VoxSequence Data Structure**: Transcription result storage
  - Hierarchical structure: Sequence → Segments → Words
  - Word-level timing information (startTime, endTime)
  - Confidence scores (placeholder for Phase II)
  - getFullText() for complete transcription text
  - getWordCount() and getTotalDuration() helpers
  
- **ARA DocumentController Integration**:
  - WhisperEngine instance managed by VoxScriptDocumentController
  - WhisperEngine::Listener implementation for callbacks
  - Transcription status tracking ("Idle", "Transcribing: X%", "Ready", "Failed")
  - VoxSequence storage in document controller
  
- **UI Updates**:
  - ScriptView: Display transcription text from VoxSequence
  - ScriptView: Status indicator for transcription progress
  - Basic multi-line text rendering
  - Placeholder text when no transcription available
  
### Technical Details
- Whisper model: ggml-base.en (English-only, ~140MB)
- Audio processing: Automatic format conversion via JUCE AudioFormatManager
- Resampling: Linear interpolation to 16kHz if needed
- Thread safety: All UI callbacks dispatched via MessageManager
- No allocations on audio thread (RT-safety preserved)

### Known Limitations (Phase II)
- Model must be manually downloaded and placed in correct directory
- No model auto-download (deferred to Phase IV)
- No cancellation UI (architecture supports it, but no UI control)
- Word-level timestamps not fully extracted (using segment timestamps)
- Single file transcription (no queue management yet)

## Phase III: Alignment and Editing (Planned)

### To Be Added
- SOFA forced alignment for singing
- VoxEditList for text-based edits
- Phase-coherent crossfading DSP
- Zero-crossing detection
- Cross-correlation phase alignment

## Phase IV: UI/UX and Optimization (Planned)

### To Be Added
- Interactive text editing
- Phoneme visualization
- Confidence underlining
- Context menu for corrections
- Room tone synthesis
- Performance optimization

---

**Version Format**: Following Semantic Versioning (MAJOR.MINOR.PATCH)
- MAJOR: Breaking changes
- MINOR: New features (backward compatible)
- PATCH: Bug fixes

**Current Version**: 0.1.0-alpha (Phase I)
