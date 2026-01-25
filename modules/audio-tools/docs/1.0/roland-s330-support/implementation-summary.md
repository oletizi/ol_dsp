# Roland S-330 Support - Implementation Summary

**Status:** In Progress
**Last Updated:** 2026-01-25

## Overview

This document summarizes the implementation of Roland S-330 support, including a web-based editor application and device client library.

## Completed Work

### S-330 Device Client (sampler-devices module)

Added comprehensive S-330 support to the sampler-devices module:

- **s330-client.ts**: Main client class with:
  - Web MIDI connection management
  - Centralized data caching (patches, tones)
  - Batch read/write operations
  - Parameter-level access methods

- **s330-addresses.ts**: SysEx address calculations:
  - System parameter addresses
  - Patch parameter addresses (64 patches, 8 banks × 8 slots)
  - Tone parameter addresses (128 tones, 16 banks × 8 slots)
  - Correct base address handling (tone base: 0x00 0x03 xx xx)

- **s330-messages.ts**: SysEx message handling:
  - Roland SysEx encoding/decoding
  - Checksum calculation
  - Request/data message construction

- **s330-types.ts**: TypeScript interfaces for all S-330 data structures

- **s330-params.ts**: Parameter definitions and metadata

### S-330 Editor Web Application (new module)

Created `@oletizi/s330-editor` web application:

- **Technology Stack:**
  - React 18 with TypeScript
  - Vite for build tooling
  - Tailwind CSS for styling
  - Radix UI for accessible components
  - Zustand for state management
  - React Query for data fetching

- **Pages:**
  - Home: MIDI device selection and setup instructions
  - Patches: Bank/slot navigation, patch list, patch editor
  - Tones: Bank/slot navigation, tone list, tone editor
  - Play: Device monitoring and status display

- **Key Components:**
  - PatchList / PatchEditor
  - ToneList / ToneEditor
  - ToneZoneEditor (patch → tone mappings)
  - MIDI device selector

### Deployment

- Netlify configuration (netlify.toml)
- Subpath routing support
- Google Analytics integration
- MIDI setup instructions with screenshot

## Technical Decisions

### Why Web MIDI?

- Cross-platform without native installers
- Works in modern browsers (Chrome, Edge, Opera)
- Real-time bidirectional communication
- No dependencies on legacy software

### Why Centralized Caching?

- Minimizes hardware polling
- Faster UI responsiveness
- Single source of truth for device state
- Enables batch operations

### Address Calculation Approach

The S-330 uses a 4-byte address scheme. Key insight: tone parameters start at base address 0x00 0x03 xx xx (not 0x00 0x01 as initially assumed). This was discovered through SysEx analysis and testing.

## Known Limitations

1. **Browser Support**: Web MIDI only works in Chromium-based browsers
2. **Parameter Coverage**: Not all S-330 parameters are exposed yet
3. **Error Recovery**: MIDI disconnection handling could be more robust
4. **Sample Editing**: Waveform data not editable (requires disk access)

## Metrics

- **Commits:** 13 on feature branch
- **Files Changed:** 28
- **Lines Added:** ~659
- **Lines Removed:** ~754 (net cleanup)

## Next Steps

1. Complete parameter coverage audit
2. Improve error handling and user feedback
3. Write user documentation
4. Create release version

## Lessons Learned

1. **SysEx debugging is challenging**: Roland's documentation has gaps; testing with real hardware essential
2. **Address encoding matters**: Off-by-one errors in address calculation cause silent failures
3. **Web MIDI works well**: Browser-based MIDI is surprisingly capable for this use case
