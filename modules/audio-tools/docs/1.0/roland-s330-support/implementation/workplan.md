# Roland S-330 Support - Workplan

**GitHub Project:** [S330 Editor](https://github.com/users/oletizi/projects/1)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#45 - [audio-tools] Roland S-330 Support](https://github.com/oletizi/ol_dsp/issues/45) (Parent)
- [#46 - Complete S-330 parameter coverage audit](https://github.com/oletizi/ol_dsp/issues/46)
- [#47 - Improve S-330 editor error handling](https://github.com/oletizi/ol_dsp/issues/47)
- [#48 - Write S-330 editor user documentation](https://github.com/oletizi/ol_dsp/issues/48)

## Technical Approach

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    s330-editor (Web App)                     │
├─────────────────────────────────────────────────────────────┤
│  React Components    │  Zustand Store  │  React Query       │
│  - PatchList         │  - MIDI state   │  - Data fetching   │
│  - PatchEditor       │  - Device state │  - Caching         │
│  - ToneList          │  - UI state     │                    │
│  - ToneEditor        │                 │                    │
│  - PlayPage          │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              sampler-devices/s330 (Library)                  │
├─────────────────────────────────────────────────────────────┤
│  S330Client          │  Messages       │  Addresses         │
│  - connect()         │  - encode()     │  - PATCH_BASE      │
│  - getPatches()      │  - decode()     │  - TONE_BASE       │
│  - getTones()        │  - checksum()   │  - SYSTEM_BASE     │
│  - writeParam()      │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Web MIDI API (Browser)                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Roland S-330 Hardware                     │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Web-first approach**: Use Web MIDI API for cross-platform compatibility without native installers
2. **Centralized caching**: S330Client maintains data cache to minimize hardware polling
3. **Zustand for state**: Lightweight state management for MIDI connection and device state
4. **Bank/slot navigation**: UI organized around S-330's bank/slot memory structure

## Implementation Phases

### Phase 1: Device Client Library (Complete)

**Goal:** Implement S-330 SysEx communication in sampler-devices module

**Tasks:**
- [x] Create s330-types.ts with TypeScript interfaces
- [x] Implement s330-addresses.ts with address calculations
- [x] Create s330-messages.ts for SysEx encoding/decoding
- [x] Build s330-client.ts with caching and MIDI operations
- [x] Add s330-params.ts for parameter definitions

**Success Criteria:**
- Can connect to S-330 via Web MIDI
- Can read/write individual parameters
- Correct checksum calculation
- Proper address encoding for patches and tones

### Phase 2: Web Application Scaffold (Complete)

**Goal:** Create s330-editor module with basic React/Vite setup

**Tasks:**
- [x] Initialize Vite + React + TypeScript project
- [x] Configure Tailwind CSS and Radix UI components
- [x] Set up React Router for page navigation
- [x] Create Zustand store for application state
- [x] Implement MIDI device selection UI

**Success Criteria:**
- Application builds and runs locally
- Can select MIDI input/output devices
- Basic navigation between pages works

### Phase 3: Patch and Tone Editing (Complete)

**Goal:** Implement core editing functionality

**Tasks:**
- [x] Create PatchList component with bank/slot navigation
- [x] Implement PatchEditor with tone zone configuration
- [x] Create ToneList component with bank/slot navigation
- [x] Implement ToneEditor with parameter controls
- [x] Add refresh functionality to reload from hardware

**Success Criteria:**
- Can view all 64 patches organized by bank
- Can edit patch parameters and tone zones
- Can view all 128 tones organized by bank
- Can edit tone parameters
- Changes sync bidirectionally with hardware

### Phase 4: Play Mode and Deployment (Complete)

**Goal:** Add performance features and deploy

**Tasks:**
- [x] Create Play page with device status monitoring
- [x] Add Google Analytics integration
- [x] Configure Netlify deployment (netlify.toml)
- [x] Handle subpath deployment routing
- [x] Add MIDI setup instructions with screenshot

**Success Criteria:**
- Play page shows current device state
- Application deployed and accessible on Netlify
- Correct routing for direct URL access

### Phase 5: Bug Fixes and Polish (In Progress)

**Goal:** Fix discovered issues and improve reliability

**Tasks:**
- [x] Fix tone parameter base address (0x00 0x03 xx xx)
- [x] Correct bank+slot numbering for patches and tones
- [x] Remove unused UI elements (MODE/MENU buttons)
- [x] Centralize data caching in S330Client
- [ ] Complete parameter coverage audit
- [ ] Improve error handling for disconnections
- [ ] Add loading states and error feedback

**Success Criteria:**
- All parameter reads/writes use correct addresses
- UI accurately reflects hardware state
- Graceful handling of MIDI errors

### Phase 6: Documentation and Release (Pending)

**Goal:** Prepare for wider release

**Tasks:**
- [ ] Write comprehensive user guide
- [ ] Document S-330 MIDI requirements
- [ ] Create troubleshooting guide
- [ ] Add contribution guidelines
- [ ] Tag release version

**Success Criteria:**
- Users can set up and use editor without support
- Known limitations documented
- Release artifacts created

## Dependencies

| Dependency | Type | Status |
|------------|------|--------|
| Web MIDI API | Browser | Required (Chrome, Edge, Opera) |
| sampler-devices module | Internal | Complete |
| Netlify account | Infrastructure | Configured |

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Web MIDI browser support | High | Document supported browsers, provide clear error |
| S-330 SysEx undocumented areas | Medium | Reverse engineer as needed, document findings |
| MIDI timing issues | Medium | Implement retry logic, debounce writes |

## Timeline

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 1: Device Client | Complete | 13 commits merged |
| Phase 2: App Scaffold | Complete | Basic app working |
| Phase 3: Editing | Complete | Patches/tones editable |
| Phase 4: Deployment | Complete | Live on Netlify |
| Phase 5: Bug Fixes | In Progress | Address fixes done |
| Phase 6: Documentation | Pending | Target: next milestone |
