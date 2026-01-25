# S5000/S6000 Editor

**Status:** Planning
**Branch:** `feature/s5000-editor`

## Overview

Web-based editor for the Akai S5000/S6000 professional samplers, providing comprehensive program and keygroup editing capabilities via file import/export. Built following the same architecture as the S-330, JV-1080, and S3000XL editors.

## Tracking Links

**GitHub Project:** [S5000/S6000 Editor](https://github.com/users/oletizi/projects/4)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#60 - [audio-tools] S5000/S6000 Editor](https://github.com/oletizi/ol_dsp/issues/60) (Parent)
- [#61 - Create s5000-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/61)
- [#62 - Implement S5000/S6000 program file import and parsing](https://github.com/oletizi/ol_dsp/issues/62)
- [#63 - Implement S5000/S6000 program visualization and keygroup editing](https://github.com/oletizi/ol_dsp/issues/63)
- [#64 - Implement S5000/S6000 filter, envelope, and LFO editors](https://github.com/oletizi/ol_dsp/issues/64)
- [#65 - Implement S5000/S6000 program export and deployment](https://github.com/oletizi/ol_dsp/issues/65)

## Affected Modules

- `@oletizi/s5000-editor` - Web application (new module)
- `@oletizi/sampler-devices` - S5000/S6000 program parser (existing)

## Documentation

- [PRD](./prd.md) - Product Requirements Document
- [Workplan](./implementation/workplan.md) - Implementation plan and task breakdown

## Quick Links

- **Live Demo:** TBD (Netlify deployment)
- **Existing Code:** `modules/audio-tools/modules/sampler-devices/src/devices/s56k*.ts` (1,812 lines)

## Module Changes

### @oletizi/s5000-editor (New)

New web application module providing:
- React + Vite + TypeScript stack (same as other editors)
- Program file import (.akp)
- Program structure visualization
- Keygroup list and editing (up to 128 keygroups)
- Sample zone assignment interface (4 zones per keygroup)
- Filter and envelope editors
- LFO and modulation parameter editing
- Program file export

### @oletizi/sampler-devices

Potential additions to existing module:
- Web-compatible program parsing (browser File API)
- Program writing/export functionality
- Enhanced validation for editor use
