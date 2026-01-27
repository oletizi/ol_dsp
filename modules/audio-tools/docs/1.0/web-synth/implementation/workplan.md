# Web Synthesizer - Workplan

**GitHub Project:** [Web Synth](https://github.com/users/oletizi/projects/5)
**GitHub Milestone:** TBD
**GitHub Issues:**

- [#69 - [audio-tools] Web Synthesizer](https://github.com/oletizi/ol_dsp/issues/69) (Parent)
- [#70 - Create WASM DSP core module](https://github.com/oletizi/ol_dsp/issues/70)
- [#71 - Implement AudioWorklet integration](https://github.com/oletizi/ol_dsp/issues/71)
- [#72 - Create web-synth application scaffold](https://github.com/oletizi/ol_dsp/issues/72)
- [#73 - Create synthesizer user interface](https://github.com/oletizi/ol_dsp/issues/73)
- [#74 - Add Web MIDI support](https://github.com/oletizi/ol_dsp/issues/74)
- [#75 - Deploy web-synth to Netlify](https://github.com/oletizi/ol_dsp/issues/75)

## Technical Approach

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         Browser                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              React Application (Main Thread)         │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │   │
│  │  │Keyboard │ │ Knobs   │ │Oscillo- │ │ MIDI      │  │   │
│  │  │Component│ │Component│ │scope    │ │ Handler   │  │   │
│  │  └────┬────┘ └────┬────┘ └────┬────┘ └─────┬─────┘  │   │
│  │       │           │           │             │        │   │
│  │       └───────────┴───────────┴─────────────┘        │   │
│  │                          │                           │   │
│  │                   Zustand Store                      │   │
│  │                    (Synth State)                     │   │
│  └──────────────────────────┬──────────────────────────┘   │
│                             │                               │
│                    MessagePort                              │
│                             │                               │
│  ┌──────────────────────────┴──────────────────────────┐   │
│  │           AudioWorklet (Audio Thread)                │   │
│  │  ┌──────────────────────────────────────────────┐   │   │
│  │  │              WASM DSP Module                  │   │   │
│  │  │  ┌─────────┐ ┌────────┐ ┌────────┐ ┌──────┐ │   │   │
│  │  │  │Oscillator│ │ Filter │ │Envelope│ │Reverb│ │   │   │
│  │  │  └─────────┘ └────────┘ └────────┘ └──────┘ │   │   │
│  │  └──────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│                             │                               │
│                      AudioContext                           │
│                             │                               │
│                         Speakers                            │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Standalone C DSP**: Port algorithms to dependency-free C for simplest WASM compilation
2. **AudioWorklet**: Use AudioWorklet for real-time audio processing (not ScriptProcessorNode)
3. **MessagePort Communication**: Parameters sent via MessagePort to audio thread
4. **Monophonic v1**: Single voice to simplify initial implementation
5. **Existing Stack**: Use React + Vite + TypeScript like other audio-tools web apps

### DSP Signal Flow

```
Note On → Oscillator → Filter → Amp Envelope → Reverb → Output
              │           │
              │      Filter Env
              │           │
          Portamento  Cutoff Mod
```

## Implementation Phases

### Phase 1: WASM DSP Core

**Goal:** Create standalone C DSP algorithms and compile to WebAssembly

**Tasks:**
- [ ] Set up Emscripten build configuration (CMakeLists.txt)
- [ ] Port PolyBLEP oscillator to standalone C
- [ ] Port SVF filter to standalone C
- [ ] Port ADSR envelope to standalone C
- [ ] Integrate existing Dattorro reverb (already pure C)
- [ ] Create synth voice combining oscillator, filter, envelopes
- [ ] Add Embind JavaScript bindings
- [ ] Write unit tests for DSP algorithms
- [ ] Verify WASM module loads and processes audio

**Success Criteria:**
- WASM module compiles without errors
- All DSP algorithms pass unit tests
- Can call DSP functions from JavaScript
- Audio output is correct (verified by listening)

### Phase 2: AudioWorklet Integration

**Goal:** Run WASM DSP in AudioWorklet for real-time performance

**Tasks:**
- [ ] Create AudioWorkletProcessor that loads WASM
- [ ] Implement parameter communication via MessagePort
- [ ] Handle note on/off messages
- [ ] Implement parameter smoothing to prevent clicks
- [ ] Measure and optimize latency
- [ ] Handle AudioContext suspension/resume

**Success Criteria:**
- Audio plays without glitches
- Parameter changes are smooth (no clicks)
- Latency is acceptable (<20ms)
- Works across Chrome, Firefox, Safari

### Phase 3: Web Application Scaffold

**Goal:** Create web-synth module with React/Vite setup

**Tasks:**
- [ ] Initialize Vite + React + TypeScript project
- [ ] Configure Tailwind CSS and Radix UI components
- [ ] Create Zustand store for synth state
- [ ] Implement audio engine initialization
- [ ] Add basic parameter controls (frequency, filter cutoff)
- [ ] Test audio output

**Success Criteria:**
- Application builds and runs locally
- Can trigger notes programmatically
- Parameter controls affect sound
- No console errors

### Phase 4: User Interface

**Goal:** Create interactive synthesizer controls

**Tasks:**
- [ ] Create virtual keyboard component (2 octaves)
- [ ] Add oscillator section (waveform select, tuning)
- [ ] Add filter section (cutoff, resonance, envelope amount)
- [ ] Add amp envelope section (ADSR sliders)
- [ ] Add filter envelope section (ADSR sliders)
- [ ] Add reverb section (decay, mix)
- [ ] Add master volume control
- [ ] Create simple oscilloscope visualization

**Success Criteria:**
- All synth parameters controllable via UI
- Visual feedback for active notes
- Oscilloscope shows waveform
- Responsive layout (works on various screen sizes)

### Phase 5: MIDI Integration

**Goal:** Support external MIDI controllers

**Tasks:**
- [ ] Implement Web MIDI API device detection
- [ ] Handle MIDI note on/off messages
- [ ] Map MIDI CC to synth parameters
- [ ] Add MIDI learn functionality (optional)
- [ ] Show MIDI device selector in UI

**Success Criteria:**
- External MIDI keyboard triggers notes
- MIDI CC controls filter cutoff (CC74)
- Works with common USB MIDI controllers

### Phase 6: Deployment

**Goal:** Deploy to Netlify for public access

**Tasks:**
- [ ] Configure Netlify deployment (netlify.toml)
- [ ] Set up proper CORS headers for WASM
- [ ] Configure SharedArrayBuffer headers (if needed)
- [ ] Add loading indicator for WASM initialization
- [ ] Test on multiple browsers
- [ ] Add browser compatibility message

**Success Criteria:**
- Application accessible via Netlify URL
- WASM loads correctly in production
- Works on Chrome, Firefox, Safari, Edge
- Graceful fallback for unsupported browsers

### Phase 7: Documentation and Polish

**Goal:** Prepare for release

**Tasks:**
- [ ] Write user guide (how to use the synth)
- [ ] Document keyboard shortcuts
- [ ] Add "About" modal with credits
- [ ] Performance optimization pass
- [ ] Accessibility improvements (keyboard navigation)

**Success Criteria:**
- User can understand how to use the synth without instruction
- No performance issues on typical hardware
- Keyboard-navigable for accessibility

## Dependencies

| Dependency | Type | Status |
|------------|------|--------|
| Emscripten | Toolchain | Required |
| Web Audio API | Browser | Required (all modern browsers) |
| AudioWorklet | Browser | Required (all modern browsers) |
| Web MIDI API | Browser | Optional (Chrome, Edge, Opera) |
| Dattorro Reverb | Internal | Complete (`libs/dattorro-verb/`) |

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Browser AudioWorklet support | High | Test on all browsers, provide fallback message |
| WASM loading performance | Medium | Add loading indicator, lazy load |
| Audio latency | Medium | Optimize buffer sizes, use SharedArrayBuffer if needed |
| DaisySP WASM complexity | Medium | Start with standalone C, avoid DaisySP in v1 |
| Cross-origin isolation (SharedArrayBuffer) | Medium | Configure proper headers, test without if possible |

## File Structure

```
modules/audio-tools/
├── web-synth/                    # New web application
│   ├── src/
│   │   ├── components/           # React components
│   │   │   ├── Keyboard.tsx
│   │   │   ├── Knob.tsx
│   │   │   ├── OscillatorSection.tsx
│   │   │   ├── FilterSection.tsx
│   │   │   ├── EnvelopeSection.tsx
│   │   │   ├── ReverbSection.tsx
│   │   │   └── Oscilloscope.tsx
│   │   ├── audio/                # Audio engine
│   │   │   ├── worklet-processor.ts
│   │   │   ├── audio-engine.ts
│   │   │   └── midi-handler.ts
│   │   ├── store/                # Zustand store
│   │   │   └── synth-store.ts
│   │   ├── wasm/                 # WASM loading
│   │   │   └── dsp-loader.ts
│   │   ├── App.tsx
│   │   └── main.tsx
│   ├── public/
│   │   └── dsp.wasm              # Compiled WASM module
│   ├── package.json
│   ├── vite.config.ts
│   ├── tailwind.config.js
│   └── netlify.toml
│
└── wasm-dsp/                     # WASM DSP source (C)
    ├── src/
    │   ├── oscillator.c
    │   ├── oscillator.h
    │   ├── filter.c
    │   ├── filter.h
    │   ├── envelope.c
    │   ├── envelope.h
    │   ├── synth.c               # Main synth voice
    │   ├── synth.h
    │   └── bindings.cpp          # Embind JavaScript bindings
    ├── CMakeLists.txt
    └── README.md
```

## Timeline

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 1: WASM DSP Core | Pending | Core algorithms |
| Phase 2: AudioWorklet | Pending | Real-time integration |
| Phase 3: App Scaffold | Pending | React setup |
| Phase 4: User Interface | Pending | Interactive controls |
| Phase 5: MIDI Integration | Pending | External controllers |
| Phase 6: Deployment | Pending | Netlify |
| Phase 7: Documentation | Pending | User guide |
