# JV-1080 Editor

**Status:** Planning
**Branch:** `feature/jv1080-editor`

## Overview

Web-based editor for the Roland JV-1080 synthesizer module, providing modern patch and effects editing capabilities via Web MIDI. Built following the same architecture as the S-330 editor.

## Tracking Links

**GitHub Project:** [S330 Editor](https://github.com/users/oletizi/projects/1)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#49 - [audio-tools] JV-1080 Editor](https://github.com/oletizi/ol_dsp/issues/49) (Parent)
- [#50 - Extract JV-1080 client from sampler-attic](https://github.com/oletizi/ol_dsp/issues/50)
- [#51 - Create jv1080-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/51)
- [#52 - Implement JV-1080 system parameter controls](https://github.com/oletizi/ol_dsp/issues/52)
- [#53 - Implement JV-1080 effects editor](https://github.com/oletizi/ol_dsp/issues/53)

## Affected Modules

- `@oletizi/jv1080-editor` - Web application (new module)
- `@oletizi/sampler-devices` - JV-1080 device client (new, extracted from sampler-attic)

## Documentation

- [PRD](./prd.md) - Product Requirements Document
- [Workplan](./implementation/workplan.md) - Implementation plan and task breakdown

## Quick Links

- **Live Demo:** TBD (Netlify deployment)
- **Existing Code:** `modules/audio-tools/modules/sampler-attic/src/midi/roland-jv-1080.ts`

## Module Changes

### @oletizi/jv1080-editor (New)

New web application module providing:
- React + Vite + TypeScript stack (same as s330-editor)
- System parameter controls
- Patch selection interface
- Effects editor with 40 FX types
- Real-time parameter feedback

### @oletizi/sampler-devices

Additions to existing module:
- `src/devices/jv1080/` - JV-1080 device client
  - Extract and refactor code from sampler-attic
  - Add proper TypeScript interfaces
  - Implement caching pattern (like S-330)
