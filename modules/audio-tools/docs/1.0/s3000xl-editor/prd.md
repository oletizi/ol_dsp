# S3000XL Editor - Product Requirements Document

**Created:** 2026-01-25
**Status:** Draft
**Owner:** Orion Letizi

## Problem Statement

The Akai S3000XL is a professional 16-bit stereo sampler from 1996 that remains popular for its distinctive sound character. While the hardware is still used in studios, editing programs and keygroups requires either the limited front-panel LCD interface or discontinued legacy software (Mesa, Akai S3000 Editor). A modern, web-based editor would provide:

- Comprehensive visual interface for program, keygroup, and sample editing
- Real-time bidirectional communication with the hardware via Web MIDI
- Cross-platform accessibility (any device with a browser and MIDI interface)
- No software installation required

Substantial existing code for S3000XL SysEx communication exists in `sampler-devices/src/devices/s3000xl.ts` (4,868 lines of generated code with full parameter support).

## User Stories

- As an S3000XL owner, I want to edit my programs from a web browser so that I don't need legacy software or the limited LCD interface
- As a sound designer, I want to see all keygroup parameters at once so that I can quickly create and modify multi-sample instruments
- As a producer, I want to manage sample assignments and key mappings visually
- As a studio engineer, I want to organize and back up my S3000XL programs efficiently

## Success Criteria

- [ ] Web MIDI connection to Akai S3000XL established
- [ ] Bidirectional SysEx communication working (read/write parameters)
- [ ] Program list display and selection
- [ ] Program header editing (name, MIDI channel, polyphony, etc.)
- [ ] Keygroup list and navigation
- [ ] Keygroup parameter editing (key range, velocity, filters, envelopes)
- [ ] Sample assignment interface
- [ ] LFO and modulation parameter editing
- [ ] Netlify deployment for public access

## Scope

### In Scope

- Web-based editor application (React + Vite + TypeScript)
- S3000XL device client library (already exists, may need web adaptation)
- SysEx message encoding/decoding for S3000XL protocol
- Program header editing (all parameters from generated interface)
- Keygroup editing (key ranges, velocity layers, filter, envelope)
- Sample zone assignment
- LFO and modulation routing
- Netlify deployment configuration

### Out of Scope (v1.0)

- Sample waveform editing (requires direct disk access)
- Disk management operations (format, backup, restore)
- Multi-program editing (single program at a time)
- Effects section editing (S3000XL has limited effects)
- Mobile-optimized interface (desktop-first)

## Dependencies

- Web MIDI API (browser support required)
- MIDI interface hardware (USB-MIDI adapter)
- Akai S3000XL hardware with MIDI connections
- Existing code in `sampler-devices/src/devices/s3000xl.ts`

## Technical Notes

### S3000XL MIDI Implementation

The S3000XL uses Akai's standard SysEx format:
- Manufacturer ID: 0x47 (Akai)
- Device ID: Configurable
- Exclusive commands for parameter access
- Extensive parameter set (4,868 lines of generated code)

### Key Data Structures (from existing code)

**ProgramHeader** - 80+ parameters including:
- PRNAME (program name)
- PRGNUM (MIDI program number)
- PMCHAN (MIDI channel)
- POLYPH (polyphony depth)
- LFO parameters (rate, depth, delay)
- Output routing and levels

**Keygroup** - Per-keygroup parameters:
- Key range (low/high)
- Velocity switching
- Filter settings (cutoff, resonance, envelope)
- Amplitude envelope (ADSR)
- Sample zone assignments

### Existing Code Assets

The `s3000xl.ts` file (4,868 lines) contains:
- Complete TypeScript interfaces for all parameters
- Generated from Akai specification
- Parameter labels and ranges
- Byte-level encoding/decoding utilities

## Open Questions

- [ ] Should we support multiple S3000XL units (different device IDs)?
- [ ] Sample transfer via MIDI - is this practical for the editor?
- [ ] Which parameters are most commonly edited (prioritize UI)?

## Appendix

### S3000XL Key Specifications

- 16-bit, 44.1kHz stereo sampling
- 32-voice polyphony
- 32MB RAM maximum
- 200 programs, 4000 samples per partition
- Up to 99 keygroups per program
- Comprehensive filter and envelope per keygroup

### References

- Akai S3000XL Operator's Manual
- Akai S3000XL MIDI Implementation
- Existing code: `modules/audio-tools/modules/sampler-devices/src/devices/s3000xl.ts`
- Generator: `modules/audio-tools/gen-s3000xl.ts`
