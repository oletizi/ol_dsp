# Roland S-330 Support - Product Requirements Document

**Created:** 2026-01-25
**Status:** In Progress
**Owner:** Orion Letizi

## Problem Statement

The Roland S-330 is a classic 12-bit sampler from 1987 that lacks modern editing tools. Musicians and vintage gear enthusiasts who own S-330 units must rely on the limited front-panel interface or discontinued legacy software to edit patches, tones, and sample parameters. A modern, web-based editor would provide:

- Intuitive visual interface for patch and tone editing
- Real-time bidirectional communication with the hardware via Web MIDI
- Cross-platform accessibility (any device with a browser and MIDI interface)
- No software installation required

## User Stories

- As an S-330 owner, I want to edit my patches and tones from a web browser so that I don't need legacy software or limited hardware controls
- As a musician, I want to see all patch parameters at once so that I can quickly understand and modify my sounds
- As a live performer, I want a "Play" mode where I can trigger and monitor the sampler's state
- As a vintage gear collector, I want to back up and document my S-330 sounds

## Success Criteria

- [x] Web MIDI connection to Roland S-330 established
- [x] Bidirectional SysEx communication working (read/write parameters)
- [x] Patch list display with bank/slot navigation
- [x] Tone list display with bank/slot navigation
- [x] Patch editor with tone zone configuration
- [x] Tone editor with parameter controls
- [x] Play page with real-time device monitoring
- [x] Netlify deployment for public access
- [ ] Complete parameter coverage for all S-330 SysEx addresses
- [ ] Comprehensive error handling for MIDI disconnections
- [ ] User documentation and setup guide

## Scope

### In Scope

- Web-based editor application (React + Vite + TypeScript)
- S-330 device client library in sampler-devices module
- SysEx message encoding/decoding for S-330 protocol
- Patch and tone editing interfaces
- Play mode for triggering and monitoring
- Netlify deployment configuration
- Basic MIDI setup instructions

### Out of Scope

- Sample waveform editing (requires disk-level access)
- Disk management operations (format, backup)
- Other Roland sampler models (S-550, S-750, etc.)
- Offline/standalone application
- Mobile-optimized interface (desktop-first)

## Dependencies

- Web MIDI API (browser support required)
- MIDI interface hardware (USB-MIDI adapter)
- Roland S-330 hardware with MIDI connections
- sampler-devices module (@oletizi/sampler-devices)

## Technical Notes

### S-330 MIDI Implementation

The S-330 uses Roland's proprietary SysEx format with:
- Device ID: 0x10 (default, configurable 0x10-0x1F)
- Model ID: 0x10 (S-330 identifier)
- Address format: 4 bytes (AA BB CC DD)
- Data format: 7-bit encoded values
- Checksum: Roland checksum algorithm

### Key Address Ranges

- System parameters: 0x00 0x00 xx xx
- Patch parameters: 0x00 0x01 xx xx (64 patches across 8 banks)
- Tone parameters: 0x00 0x03 xx xx (128 tones across 16 banks)

## Open Questions

- [x] Correct base address for tone parameters (resolved: 0x00 0x03 xx xx)
- [ ] Complete list of all editable parameters
- [ ] Optimal polling interval for device state refresh

## Appendix

### References

- Roland S-330 Owner's Manual
- Roland S-330 Service Notes (SysEx documentation)
- Web MIDI API specification: https://webaudio.github.io/web-midi-api/
