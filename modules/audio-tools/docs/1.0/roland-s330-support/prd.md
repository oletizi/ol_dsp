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
- As a user with a video capture setup, I want a virtual front panel next to the video feed so I can control the S-330's native interface remotely via mouse clicks

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
- [ ] Virtual Front Panel with remote control capabilities

## Scope

### In Scope

- Web-based editor application (React + Vite + TypeScript)
- S-330 device client library in sampler-devices module
- SysEx message encoding/decoding for S-330 protocol
- Patch and tone editing interfaces
- Play mode for triggering and monitoring
- Virtual Front Panel for remote control via SysEx button messages
- Video capture display for viewing S-330's physical screen
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

For detailed S-330 MIDI protocol documentation including SysEx message formats, address maps, and parameter definitions, see the authoritative reference:

**[S-330 SysEx Documentation](../s330_sysex.md)**

### Front Panel Remote Control

The S-330 accepts DT1 SysEx messages to address `00 04 00 00` that simulate physical button presses. This undocumented feature (discovered January 2026) enables full remote control of the S-330's native UI:

**[S-330 Front Panel SysEx Documentation](../s330_front_panel_sysex.md)**

Supported controls:
- Navigation: Up, Down, Left, Right arrows
- Value: Inc (increment), Dec (decrement)
- Function: MODE, MENU, SUB MENU, COM, Execute

## Open Questions

- [ ] Complete list of all editable parameters
- [ ] Optimal polling interval for device state refresh

## References

- [S-330 SysEx Documentation](../s330_sysex.md) - Protocol and address map reference
- Roland S-330 Owner's Manual
- Web MIDI API specification: https://webaudio.github.io/web-midi-api/
