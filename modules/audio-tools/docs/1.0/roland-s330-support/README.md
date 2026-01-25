# Roland S-330 Support

**Status:** In Progress
**Branch:** `feature/roland-s330-support`

## Overview

Web-based editor for the Roland S-330 12-bit sampler, providing modern patch and tone editing capabilities via Web MIDI.

## Tracking Links

**GitHub Project:** [S330 Editor](https://github.com/users/oletizi/projects/1)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- Parent: [#45 - [audio-tools] Roland S-330 Support](https://github.com/oletizi/ol_dsp/issues/45)
- [#46 - Complete S-330 parameter coverage audit](https://github.com/oletizi/ol_dsp/issues/46)
- [#47 - Improve S-330 editor error handling](https://github.com/oletizi/ol_dsp/issues/47)
- [#48 - Write S-330 editor user documentation](https://github.com/oletizi/ol_dsp/issues/48)

## Affected Modules

- `@oletizi/s330-editor` - Web application (new module)
- `@oletizi/sampler-devices` - S-330 device client additions

## Documentation

- [PRD](./prd.md) - Product Requirements Document
- [Workplan](./implementation/workplan.md) - Implementation plan and task breakdown

## Quick Links

- **Live Demo:** https://s330-editor.netlify.app (when deployed)
- **S-330 MIDI Setup:** See instructions in the editor's home page

## Module Changes

### @oletizi/s330-editor (New)

New web application module providing:
- React + Vite + TypeScript stack
- Patch list and editor interfaces
- Tone list and editor interfaces
- Play mode with device monitoring
- Netlify deployment configuration

### @oletizi/sampler-devices

Additions to existing module:
- `src/devices/s330/` - S-330 device client
  - `s330-client.ts` - Main client with caching
  - `s330-addresses.ts` - SysEx address definitions
  - `s330-params.ts` - Parameter definitions
  - `s330-messages.ts` - Message encoding/decoding
  - `s330-types.ts` - TypeScript type definitions
