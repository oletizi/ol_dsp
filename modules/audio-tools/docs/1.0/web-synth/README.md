# Web Synthesizer

**Status:** Planning
**Branch:** `feature/web-synth`

## Overview

Browser-based synthesizer using WebAssembly-compiled DSP algorithms from ol_dsp. Provides an interactive playground for experimenting with oscillators, filters, envelopes, and reverb without requiring any software installation.

## Tracking Links

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

## Affected Modules

- `@oletizi/web-synth` - Web application (new module)
- `modules/synthlib/` - Source DSP algorithms (C++)
- `modules/fxlib/` - Source effects (C++)
- `libs/dattorro-verb/` - Reverb algorithm (C)

## Documentation

- [PRD](./prd.md) - Product Requirements Document
- [Workplan](./implementation/workplan.md) - Implementation plan and task breakdown

## Quick Links

- **Live Demo:** TBD (when deployed)
- **Emscripten Docs:** https://emscripten.org/docs/

## Module Changes

### @oletizi/web-synth (New)

New web application module providing:
- React + Vite + TypeScript stack
- WASM-compiled DSP core
- AudioWorklet for real-time processing
- Virtual keyboard and parameter controls
- Web MIDI API integration
- Netlify deployment configuration

### WASM DSP Module (New)

New C/C++ module compiled to WebAssembly:
- Standalone oscillator (PolyBLEP)
- Standalone SVF filter
- Standalone ADSR envelope
- Dattorro plate reverb
- Saturator effect
- JavaScript bindings via Embind

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    web-synth (Web App)                       │
├─────────────────────────────────────────────────────────────┤
│  React UI            │  Audio Engine   │  MIDI Handler      │
│  - Keyboard          │  - AudioWorklet │  - Web MIDI API    │
│  - Parameter knobs   │  - WASM bridge  │  - Note on/off     │
│  - Oscilloscope      │                 │  - CC mapping      │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    WASM DSP (C → WebAssembly)                │
├─────────────────────────────────────────────────────────────┤
│  synth_process()     │  reverb_process()  │  Parameter setters │
└──────────────────────┴────────────────────┴──────────────────┘
```
