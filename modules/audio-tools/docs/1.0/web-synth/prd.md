# Web Synthesizer - Product Requirements Document

**Created:** 2026-01-26
**Status:** Draft
**Owner:** Orion Letizi

## Problem Statement

The ol_dsp project contains a substantial collection of portable C/C++ DSP algorithms (oscillators, filters, envelopes, reverb, effects) designed for embedded systems. These algorithms are currently only usable on native platforms (Arduino, Daisy, desktop). Musicians and developers who want to experiment with these DSP algorithms without hardware setup, or who want to build browser-based audio applications, have no way to access this code.

A web-based synthesizer would:

- Make DSP algorithms accessible to anyone with a modern browser
- Provide an interactive playground for experimenting with synthesis parameters
- Demonstrate the capabilities of the ol_dsp DSP library
- Enable browser-based audio applications using battle-tested DSP code
- Serve as a reference implementation for WebAssembly audio integration

## User Stories

- As a musician, I want to play a synthesizer in my browser so that I can experiment with sounds without installing software
- As a developer, I want to use ol_dsp's DSP algorithms in my web application so that I don't have to reimplement standard synthesis components
- As a student, I want to see how oscillators, filters, and envelopes work together so that I can learn about audio synthesis
- As a hardware developer, I want to prototype synth patches in the browser before deploying to embedded hardware

## Success Criteria

- [ ] C/C++ DSP code compiles to WebAssembly via Emscripten
- [ ] Audio processing runs in AudioWorklet for real-time performance
- [ ] Synthesizer responds to MIDI input (Web MIDI API)
- [ ] Synthesizer responds to on-screen keyboard/controls
- [ ] Latency is acceptable for real-time performance (<20ms)
- [ ] All core synthesis components work: oscillator, filter, envelope, reverb
- [ ] Application deployed and accessible on Netlify

## Scope

### In Scope

- **WASM Compilation**: Emscripten build configuration for DSP code
- **Core DSP Modules**:
  - Oscillators (PolyBLEP sawtooth, sine, square, triangle)
  - Filters (SVF low-pass, high-pass, band-pass)
  - Envelopes (ADSR)
  - Portamento/glide
  - Dattorro plate reverb
  - Saturator/distortion
- **Web Application**: React + Vite + TypeScript (matches existing audio-tools stack)
- **Audio Integration**: AudioWorklet for real-time processing
- **MIDI Support**: Web MIDI API for external controller input
- **On-screen Controls**: Virtual keyboard and parameter knobs
- **Deployment**: Netlify configuration

### Out of Scope

- Polyphony (initial version is monophonic)
- Preset management/save/load
- Sequencer or arpeggiator
- Effects beyond reverb and saturation (delay, chorus, etc.)
- Mobile-optimized interface (desktop-first)
- JUCE or plugin framework integration
- DaisySP-dependent code (pure C/C++ only in v1)

## Dependencies

- **WebAssembly**: Browser support for WASM (all modern browsers)
- **AudioWorklet**: Browser support (Chrome, Firefox, Safari, Edge)
- **Web MIDI API**: Browser support (Chrome, Edge, Opera; Firefox requires flag)
- **Emscripten**: WASM compiler toolchain
- **Existing DSP Code**:
  - `libs/dattorro-verb/` - Pure C reverb (no dependencies)
  - `modules/synthlib/` - C++ synthesis components
  - `modules/fxlib/` - C++ effects

## Technical Notes

### Portable DSP Code Available

| Component | Location | Language | Dependencies |
|-----------|----------|----------|--------------|
| Oscillators | `modules/synthlib/OscillatorSoundSource.h` | C++ | DaisySP |
| SVF Filter | `modules/synthlib/Filter.h` | C++ | DaisySP |
| ADSR Envelope | `modules/synthlib/Adsr.h` | C++ | DaisySP |
| Portamento | `modules/synthlib/Portamento.h` | C++ | None (standalone) |
| Dattorro Reverb | `libs/dattorro-verb/` | C | None (standalone) |
| Saturator | `modules/fxlib/Fx.h` | C++ | None |
| Synth Voice | `modules/synthlib/SynthVoice.h` | C++ | Above components |

### Recommended Approach

**Phase 1: Standalone C Code**
Start with pure C code that has no dependencies:
- Dattorro reverb
- Custom oscillator (port PolyBLEP to C)
- Custom filter (port SVF to C)
- Custom ADSR (port envelope to C)

**Phase 2: DaisySP Integration (Optional)**
If needed, compile DaisySP to WASM and use existing wrappers.

### Existing WASM Infrastructure

`libs/miniaudio/miniaudio.h` already contains Emscripten support:
```c
#include <emscripten/emscripten.h>
#include <emscripten/webaudio.h>

// AudioWorklet integration
emscripten_create_wasm_audio_worklet_node()

// Memory management exports
EMSCRIPTEN_KEEPALIVE void ma_malloc_emscripten(size_t sz)
EMSCRIPTEN_KEEPALIVE void ma_free_emscripten(void* p)
```

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    web-synth (Web App)                       │
├─────────────────────────────────────────────────────────────┤
│  React Components    │  Zustand Store  │  AudioWorklet       │
│  - Keyboard          │  - MIDI state   │  - WASM DSP         │
│  - Knobs/Sliders     │  - Synth params │  - Audio processing │
│  - Oscilloscope      │  - UI state     │                     │
└──────────────────────┴─────────────────┴─────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    WASM DSP Module                           │
├─────────────────────────────────────────────────────────────┤
│  Oscillator          │  Filter         │  Reverb            │
│  - setFrequency()    │  - setCutoff()  │  - setDecay()      │
│  - setWaveform()     │  - setResonance()│  - setMix()       │
│  - process()         │  - process()    │  - process()       │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Web Audio API (Browser)                   │
└─────────────────────────────────────────────────────────────┘
```

## Open Questions

- [ ] Should we compile DaisySP to WASM or port algorithms to standalone C?
- [ ] What's the optimal AudioWorklet buffer size for latency vs performance?
- [ ] Should we use SharedArrayBuffer for parameter communication?
- [ ] What sample rate should we target (44.1kHz vs 48kHz)?

## References

- [Web Audio API Specification](https://webaudio.github.io/web-audio-api/)
- [AudioWorklet Design Pattern](https://developer.chrome.com/blog/audio-worklet-design-pattern/)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [Dattorro Reverb Paper](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf)
- Existing ol_dsp DSP code in `modules/synthlib/`, `modules/fxlib/`, `libs/dattorro-verb/`
