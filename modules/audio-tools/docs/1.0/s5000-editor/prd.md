# S5000/S6000 Editor - Product Requirements Document

**Created:** 2026-01-25
**Status:** Draft
**Owner:** Orion Letizi

## Problem Statement

The Akai S5000 and S6000 are professional samplers from the late 1990s that remain popular for their powerful synthesis engine and extensive modulation capabilities. While the hardware front panel provides comprehensive editing, managing program files (.akp) requires proprietary software (MESA) or command-line tools. A modern, web-based editor would provide:

- Visual interface for program and keygroup editing
- Real-time preview of program structure and mappings
- Cross-platform accessibility (any device with a browser)
- Import/export of .akp program files
- No software installation required

Substantial existing code for S5000/S6000 program parsing exists in `sampler-devices/src/devices/s56k*.ts` (1,812 lines of TypeScript with full chunk and program support).

## User Stories

- As an S5000/S6000 owner, I want to edit my programs from a web browser so that I don't need legacy software
- As a sound designer, I want to see all keygroup parameters visually so that I can create complex multi-sample instruments
- As a producer, I want to manage sample assignments and key mappings efficiently
- As a studio engineer, I want to convert and organize my S5000/S6000 programs for modern workflows

## Success Criteria

- [ ] Program file (.akp) import working
- [ ] Program structure visualization (keygroups, zones)
- [ ] Program header editing (name, polyphony, etc.)
- [ ] Keygroup list and navigation
- [ ] Keygroup parameter editing (key range, velocity, filters, envelopes)
- [ ] Sample zone assignment interface
- [ ] LFO and modulation parameter editing
- [ ] Filter and envelope visualization
- [ ] Program export (.akp format)
- [ ] Netlify deployment for public access

## Scope

### In Scope

- Web-based editor application (React + Vite + TypeScript)
- S5000/S6000 program file parsing (already exists in sampler-devices)
- Program header editing
- Keygroup editing (key ranges, velocity layers, filter, envelopes)
- Sample zone assignment display
- LFO and modulation routing
- Aux envelope editing
- Program file export
- Netlify deployment configuration

### Out of Scope (v1.0)

- Real-time MIDI SysEx communication (S5000/S6000 MIDI is limited)
- Sample waveform editing
- Disk image management
- Multi-program batch editing
- Mobile-optimized interface (desktop-first)
- Direct hardware connection (file-based workflow)

## Dependencies

- Existing code in `sampler-devices/src/devices/s56k*.ts` (1,812 lines)
- File API for browser-based file import/export
- Existing converters in `sampler-export/src/lib/converters/s5k-to-*.ts`

## Technical Notes

### S5000/S6000 Program File Format

The .akp format uses a chunk-based structure:
- Header chunk with program metadata
- Program chunk with global settings
- Keygroup chunks with per-keygroup parameters
- Zone chunks for sample assignment
- Filter, envelope, LFO, and modulation chunks

### Key Data Structures (from existing code)

**AkaiS56kProgram** - Complete program container:
- HeaderChunk (file metadata)
- ProgramChunk (global parameters)
- KeygroupChunk[] (up to 128 keygroups)
- ZoneChunk[] (4 zones per keygroup)
- FilterChunk, AmpEnvelopeChunk, FilterEnvelopeChunk
- Lfo1Chunk, Lfo2Chunk, ModsChunk
- AuxEnvelopeChunk, OutputChunk, TuneChunk, KlocChunk

**Keygroup Structure:**
- Key range (low/high)
- Velocity switching
- 4 sample zones per keygroup
- Independent filter per keygroup
- Amplitude and filter envelopes
- Two LFOs with modulation routing

### Existing Code Assets

The `s56k*.ts` files (1,812 lines total) contain:
- Complete TypeScript interfaces for all chunks (`s56k-types.ts` - 682 lines)
- Chunk parsing and factory functions (`s56k-chunks.ts` - 406 lines)
- Binary parsing logic (`s56k-parser.ts` - 335 lines)
- Program API (`s56k-program.ts` - 158 lines)
- Utility functions (`s56k-utils.ts` - 145 lines)

## Open Questions

- [ ] Should we support S5000 vs S6000 specific features?
- [ ] Sample preview via audio file association?
- [ ] Drag-and-drop sample assignment from file system?

## Appendix

### S5000/S6000 Key Specifications

- 64-voice polyphony (S6000: 128 voices optional)
- 256MB RAM maximum
- Up to 128 keygroups per program
- 4 sample zones per keygroup
- Comprehensive filter types (LP, HP, BP, BR, combinations)
- Dual LFOs with extensive modulation
- Aux envelope for additional modulation

### References

- Akai S5000/S6000 Operator's Manual
- Existing code: `modules/audio-tools/modules/sampler-devices/src/devices/s56k*.ts`
- Converters: `modules/audio-tools/modules/sampler-export/src/lib/converters/s5k-to-*.ts`
