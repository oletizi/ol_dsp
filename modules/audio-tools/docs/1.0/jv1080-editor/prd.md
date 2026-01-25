# JV-1080 Editor - Product Requirements Document

**Created:** 2026-01-25
**Status:** Draft
**Owner:** Orion Letizi

## Problem Statement

The Roland JV-1080 is a legendary 1994 synthesizer module used extensively in professional music production. While the hardware remains popular, editing patches requires either the limited front-panel interface or discontinued legacy software. A modern, web-based editor would provide:

- Intuitive visual interface for patch and effects editing
- Real-time bidirectional communication with the hardware via Web MIDI
- Cross-platform accessibility (any device with a browser and MIDI interface)
- No software installation required

Existing code for JV-1080 SysEx communication exists in `sampler-attic/src/midi/roland-jv-1080.ts` and needs to be extracted, refactored, and built into a proper web application following the s330-editor pattern.

## User Stories

- As a JV-1080 owner, I want to edit my patches from a web browser so that I don't need legacy software or limited hardware controls
- As a sound designer, I want to see all patch parameters at once so that I can quickly understand and modify my sounds
- As a musician, I want to control effects parameters in real-time for live performance tweaking
- As a studio engineer, I want to organize and manage my JV-1080 patches efficiently

## Success Criteria

- [ ] Web MIDI connection to Roland JV-1080 established
- [ ] Bidirectional SysEx communication working (read/write parameters)
- [ ] System parameter controls (panel mode, FX switches, clock)
- [ ] Patch selection and navigation
- [ ] Effects type selection and parameter editing
- [ ] Patch name editing
- [ ] Real-time parameter feedback from hardware
- [ ] Netlify deployment for public access

## Scope

### In Scope

- Web-based editor application (React + Vite + TypeScript)
- JV-1080 device client library (extract from sampler-attic, enhance)
- SysEx message encoding/decoding for JV-1080 protocol
- System parameter editing (panel mode, FX switches, clock source)
- Patch selection (user/PCM groups, patch numbers)
- Effects editing (40 FX types with parameters)
- Patch name editing
- Netlify deployment configuration

### Out of Scope (v1.0)

- Performance mode editing (multi-timbral setups)
- Waveform/sample editing (hardware limitation)
- Expansion board management (SR-JV80 series)
- Bulk patch dump/restore
- Mobile-optimized interface (desktop-first)

## Dependencies

- Web MIDI API (browser support required)
- MIDI interface hardware (USB-MIDI adapter)
- Roland JV-1080 hardware with MIDI connections
- Existing code in `sampler-attic/src/midi/roland-jv-1080.ts`

## Technical Notes

### JV-1080 MIDI Implementation

The JV-1080 uses Roland's standard SysEx format:
- Manufacturer ID: 0x41 (Roland)
- Model ID: 0x6A (JV-1080)
- Device ID: 0x10 (default, configurable 0x10-0x1F)
- Commands: RQ1 (0x11) for requests, DT1 (0x12) for data

### Key Address Ranges (from existing code)

- System parameters: Base [0, 0, 0, 0]
  - Panel mode, performance number, patch group/number
  - EFX/Chorus/Reverb switches, clock source
- Temp Patch: Base [3, 0, 0, 0]
  - Patch name (12 chars)
  - FX type and parameters

### Existing Code Assets

The `roland-jv-1080.ts` file (332 lines) contains:
- SysEx message generation with checksum
- System parameter controls (panel mode, patch selection)
- FX type constants (40 effect types)
- Event subscription system for parameter changes
- Basic Jv1080 class with MIDI integration

## Open Questions

- [ ] Should we support multiple JV-1080 units (different device IDs)?
- [ ] Priority for Performance mode editing in future versions?
- [ ] Should this share a GitHub Project with S-330 or have its own?

## Appendix

### JV-1080 Effects Types (40 total)

From existing code:
1. STEREO-EQ, OVERDRIVE, DISTORTION, PHASER, SPECTRUM
2. ENHANCER, AUTO-WAH, ROTARY, COMPRESSOR, LIMITER
3. HEXA-CHORUS, TREMOLO-CHORUS, SPACE-D, STEREO-CHORUS
4. STEREO-FLANGER, STEP-FLANGER, STEREO-DELAY, MODULATION-DELAY
5. TRIPLE-TAP-DELAY, QUADRUPLE-TAP-DELAY, TIME-CONTROL-DELAY
6. VOICE-PITCH-SHIFTER, FBK-PITCH-SHIFTER, REVERB, GATE-REVERB
7. Combination effects (OVERDRIVE→CHORUS, DISTORTION→DELAY, etc.)

### References

- Roland JV-1080 Owner's Manual
- Roland JV-1080 MIDI Implementation Chart
- Existing code: `modules/audio-tools/modules/sampler-attic/src/midi/roland-jv-1080.ts`
