# S3000XL Editor

**Status:** Planning
**Branch:** `feature/s3000xl-editor`

## Overview

Web-based editor for the Akai S3000XL professional sampler, providing comprehensive program and keygroup editing capabilities via Web MIDI. Built following the same architecture as the S-330 and JV-1080 editors.

## Tracking Links

**GitHub Project:** [S3000XL Editor](https://github.com/users/oletizi/projects/3)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#54 - [audio-tools] S3000XL Editor](https://github.com/oletizi/ol_dsp/issues/54) (Parent)
- [#55 - Create Web MIDI adapter for s3000xl.ts](https://github.com/oletizi/ol_dsp/issues/55)
- [#56 - Create s3000xl-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/56)
- [#57 - Implement S3000XL program list and header editing](https://github.com/oletizi/ol_dsp/issues/57)
- [#58 - Implement S3000XL keygroup editing interface](https://github.com/oletizi/ol_dsp/issues/58)
- [#59 - Implement S3000XL sample zone and modulation editors](https://github.com/oletizi/ol_dsp/issues/59)

## Affected Modules

- `@oletizi/s3000xl-editor` - Web application (new module)
- `@oletizi/sampler-devices` - S3000XL device client (existing, may need web adaptation)

## Documentation

- [PRD](./prd.md) - Product Requirements Document
- [Workplan](./implementation/workplan.md) - Implementation plan and task breakdown

## Quick Links

- **Live Demo:** TBD (Netlify deployment)
- **Existing Code:** `modules/audio-tools/modules/sampler-devices/src/devices/s3000xl.ts` (4,868 lines)

## Module Changes

### @oletizi/s3000xl-editor (New)

New web application module providing:
- React + Vite + TypeScript stack (same as other editors)
- Program list and selection
- Program header editing
- Keygroup list and editing
- Sample zone assignment interface
- Filter and envelope editors

### @oletizi/sampler-devices

Potential additions to existing module:
- Web MIDI adapter for s3000xl.ts
- Caching layer for performance
- Async/await API wrappers
