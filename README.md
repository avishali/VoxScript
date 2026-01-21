# VoxScript

**Semantic Audio Integration for Professional DAWs**

VoxScript is an ARA2 plugin that brings text-based audio editing into professional Digital Audio Workstations. Edit vocals by editing transcribed text.

## 🎯 Product Vision

**"The Blue Ocean between Melodyne and Descript"**

VoxScript bridges the semantic gap in audio editing:
- **Signal Level** (Melodyne): Edit waveforms and pitch
- **⭐ VoxScript**: Edit audio by editing text
- **Standalone** (Descript): Text editing, but no mixing environment

## 📋 Development Phases

### ✅ Phase I: ARA Skeleton (Current)
- [x] CMake build system with ARA2 support
- [x] VoxScriptDocumentController (ARA session manager)
- [x] VoxScriptAudioSource (audio file wrapper)
- [x] VoxScriptPlaybackRenderer (audio output)
- [x] Dual-view UI (Script View + Detail View)
- [ ] Verify builds in target DAWs

### 🔄 Phase II: Transcription Engine (Months 3-5)
- [ ] Integrate whisper.cpp for speech-to-text
- [ ] Implement background analysis thread
- [ ] Display transcription in Script View
- [ ] Add basic word-level timing

### 🚀 Phase III: Alignment and Editing (Months 6-9)
- [ ] SOFA integration for singing alignment
- [ ] VoxEditList implementation
- [ ] Phase-coherent crossfading DSP
- [ ] Interactive text -> audio edits

### 🎨 Phase IV: UI/UX and Optimization (Months 9-12)
- [ ] Advanced Script View features
- [ ] Phoneme visualization in Detail View
- [ ] Room tone synthesis
- [ ] Performance optimization

## 🏗️ Building VoxScript

### Prerequisites
- CMake 3.22+
- JUCE 8 (in SDK folder)
- C++17 compiler
- ARA2-compatible DAW (Logic Pro, Studio One, Reaper, Pro Tools)

### Initial Setup

1. **Set JUCE path** (if not in default location):
```bash
export JUCE_PATH=/path/to/SDK/JUCE
```

2. **Add MelechDSP shared modules** (optional, for UI components):
```bash
git submodule add ../melechdsp-hq third_party/melechdsp-hq
git submodule update --init --recursive
```

3. **Configure and build**:
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Development Build (Fast Iteration)
```bash
cmake .. -DPLUGIN_DEV_MODE=ON  # VST3 + Standalone only, no LTO
```

### Release Build
```bash
cmake .. -DPLUGIN_DEV_MODE=OFF  # All formats, with LTO
```

## 🎛️ Target DAW Support

| DAW | ARA2 Support | Integration Method | Status |
|-----|--------------|-------------------|--------|
| **Studio One** | ✅ Full | Integrated Editor Pane | Phase I |
| **Logic Pro** | ⚠️ Limited | Floating Window | Phase I |
| **Reaper** | ✅ Full | Docker/Floating | Phase I |
| **Pro Tools** | ✅ Full | Tabbed Editor (2024.06+) | Phase I |

## 📐 Architecture

```
VoxScript/
├── CMakeLists.txt              # Build configuration with ARA2
├── Source/
│   ├── PluginProcessor.h/cpp   # Main processor with ARA extension
│   ├── PluginEditor.h/cpp      # Dual-view UI
│   ├── ara/                    # ARA2 integration
│   │   ├── VoxScriptDocumentController
│   │   ├── VoxScriptAudioSource
│   │   └── VoxScriptPlaybackRenderer
│   └── ui/                     # User interface
│       ├── ScriptView          # Text editing layer
│       └── DetailView          # Waveform/phoneme layer
└── third_party/
    └── melechdsp-hq/          # Shared MelechDSP modules (optional)
```

## 🔑 Key Technical Decisions

From the Product Definition Document:

1. **ARA2 Integration**: Random access to audio, non-destructive editing
2. **On-Device ML**: Privacy-first, zero-latency (whisper.cpp, ONNX Runtime)
3. **Hybrid Transcription**: Whisper for dialogue + SOFA for singing
4. **Phase-Coherent DSP**: Zero-crossing search, cross-correlation, equal-power crossfades
5. **Room Tone Synthesis**: Spectral inpainting to fill edit gaps

## 📝 Plugin Configuration

- **Name**: VoxScript
- **Version**: 0.1.0 (Phase I)
- **Code**: VxSc
- **Manufacturer**: MelechDSP (Melc)
- **Formats**: VST3, AU, AAX, Standalone
- **ARA**: Enabled with TempoAdjustment, Notes content types

## 🎓 Development Notes

### Phase I Focus
Current phase focuses on **proving the ARA2 integration**:
- Plugin loads in target DAWs
- Document controller receives audio sources
- UI displays in different DAW environments
- State persists across project save/load

### No ML Yet
Phase I deliberately excludes machine learning:
- No whisper.cpp integration (Phase II)
- No ONNX Runtime (Phase III)
- Focus on ARA architecture

### Testing in DAWs
To test Phase I:
1. Load plugin in ARA-compatible DAW
2. Drag audio clip onto timeline
3. Verify plugin opens and shows "ARA2 Active"
4. Check console logs for ARA callbacks

## 📚 References

- [Product Definition Document](../Audio_Plugin_Development_Requirements.pdf)
- [ARA SDK Documentation](https://github.com/Celemony/ARA_SDK)
- [JUCE ARA Classes](https://docs.juce.com/master/group__juce__ara.html)

## 👥 Team

**MelechDSP** - Professional audio plugin development

---

**Phase I Milestone**: Verify ARA2 integration in all target DAWs ✓
