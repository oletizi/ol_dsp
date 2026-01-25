# S3000XL Editor - Workplan

**GitHub Project:** [S3000XL Editor](https://github.com/users/oletizi/projects/3)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#54 - [audio-tools] S3000XL Editor](https://github.com/oletizi/ol_dsp/issues/54) (Parent)
- [#55 - Create Web MIDI adapter for s3000xl.ts](https://github.com/oletizi/ol_dsp/issues/55)
- [#56 - Create s3000xl-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/56)
- [#57 - Implement S3000XL program list and header editing](https://github.com/oletizi/ol_dsp/issues/57)
- [#58 - Implement S3000XL keygroup editing interface](https://github.com/oletizi/ol_dsp/issues/58)
- [#59 - Implement S3000XL sample zone and modulation editors](https://github.com/oletizi/ol_dsp/issues/59)

## Technical Approach

### Architecture

Following the same pattern as s330-editor and jv1080-editor:

```
┌─────────────────────────────────────────────────────────────┐
│                  s3000xl-editor (Web App)                    │
├─────────────────────────────────────────────────────────────┤
│  React Components    │  Zustand Store  │  React Query       │
│  - ProgramList       │  - MIDI state   │  - Data fetching   │
│  - ProgramHeader     │  - Device state │  - Caching         │
│  - KeygroupEditor    │  - UI state     │                    │
│  - SampleZoneView    │                 │                    │
│  - FilterEditor      │                 │                    │
│  - EnvelopeEditor    │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│            sampler-devices/s3000xl (Library)                 │
├─────────────────────────────────────────────────────────────┤
│  S3000XLClient       │  Messages       │  Parameters        │
│  - connect()         │  - encode()     │  - ProgramHeader   │
│  - getProgram()      │  - decode()     │  - Keygroup        │
│  - setParameter()    │  - checksum()   │  - SampleZone      │
│  - getKeygroup()     │                 │  - Filter/Envelope │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Web MIDI API (Browser)                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Akai S3000XL Hardware                      │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Reuse s330/jv1080-editor architecture**: Same stack, similar component structure
2. **Leverage existing s3000xl.ts**: 4,868 lines of generated code with full parameter support
3. **Web MIDI adapter**: Adapt existing Node.js code for browser environment
4. **Caching layer**: Client maintains state to minimize SysEx polling

## Implementation Phases

### Phase 1: Web MIDI Adapter for S3000XL

**Goal:** Adapt existing s3000xl.ts code for Web MIDI

**Tasks:**
- [ ] Create `src/devices/s3000xl/web-adapter.ts` for browser compatibility
- [ ] Implement Web MIDI input/output handling
- [ ] Create async/await wrappers for SysEx operations
- [ ] Add connection management and error handling
- [ ] Write integration tests with MIDI mocking

**Success Criteria:**
- Can connect to S3000XL via Web MIDI
- SysEx messages sent and received correctly
- Connection state properly managed

### Phase 2: Web Application Scaffold

**Goal:** Create s3000xl-editor module with basic React/Vite setup

**Tasks:**
- [ ] Copy and adapt s330-editor/jv1080-editor scaffold
- [ ] Configure package.json and dependencies
- [ ] Set up Zustand store for S3000XL state
- [ ] Implement MIDI device selection UI
- [ ] Create basic routing structure

**Success Criteria:**
- Application builds and runs locally
- Can select MIDI input/output devices
- Basic navigation works

### Phase 3: Program List and Selection

**Goal:** Display and navigate programs on the S3000XL

**Tasks:**
- [ ] Implement program list fetching via SysEx
- [ ] Create ProgramList component with scrolling
- [ ] Add program selection with visual feedback
- [ ] Show program metadata (name, keygroup count)

**Success Criteria:**
- All programs visible in list
- Selection triggers program load
- Current program highlighted

### Phase 4: Program Header Editing

**Goal:** Edit top-level program parameters

**Tasks:**
- [ ] Create ProgramHeader component
- [ ] Implement program name editor (12 characters)
- [ ] Add MIDI channel selector
- [ ] Implement polyphony and priority controls
- [ ] Add LFO rate/depth/delay controls
- [ ] Create output routing controls

**Success Criteria:**
- All program header parameters editable
- Changes sync to hardware immediately
- Visual feedback for current values

### Phase 5: Keygroup List and Navigation

**Goal:** Display and navigate keygroups within a program

**Tasks:**
- [ ] Implement keygroup list fetching
- [ ] Create KeygroupList component
- [ ] Show key range visualization per keygroup
- [ ] Add keygroup selection
- [ ] Display keygroup count and current selection

**Success Criteria:**
- All keygroups visible with key ranges
- Can navigate between keygroups
- Current keygroup highlighted

### Phase 6: Keygroup Parameter Editing

**Goal:** Comprehensive keygroup parameter editing

**Tasks:**
- [ ] Create KeygroupEditor component
- [ ] Implement key range controls (low/high key)
- [ ] Add velocity switching parameters
- [ ] Create filter section (cutoff, resonance, envelope)
- [ ] Implement amplitude envelope (ADSR)
- [ ] Add filter envelope controls

**Success Criteria:**
- All keygroup parameters editable
- Filter and envelope visualizations
- Real-time hardware sync

### Phase 7: Sample Zone Assignment

**Goal:** Interface for assigning samples to zones

**Tasks:**
- [ ] Create SampleZoneView component
- [ ] Display sample assignment per zone (1-4)
- [ ] Implement sample selection from available samples
- [ ] Add zone tuning parameters
- [ ] Create zone level/pan controls

**Success Criteria:**
- Sample zones visible and editable
- Can assign samples to zones
- Zone parameters adjustable

### Phase 8: LFO and Modulation

**Goal:** LFO and modulation routing controls

**Tasks:**
- [ ] Create LFO editor component
- [ ] Implement LFO waveform selection
- [ ] Add modulation depth controls
- [ ] Create modulation routing display

**Success Criteria:**
- LFO parameters fully editable
- Modulation routing clear
- Real-time sync with hardware

### Phase 9: Deployment and Polish

**Goal:** Deploy and prepare for release

**Tasks:**
- [ ] Configure Netlify deployment
- [ ] Add error handling for MIDI disconnections
- [ ] Write user documentation
- [ ] Add loading states and feedback
- [ ] Browser compatibility testing

**Success Criteria:**
- Deployed and accessible on Netlify
- Graceful error handling
- Documentation complete

## Dependencies

| Dependency | Type | Status |
|------------|------|--------|
| Web MIDI API | Browser | Required (Chrome, Edge, Opera) |
| sampler-devices module | Internal | Exists with s3000xl.ts (4,868 lines) |
| Netlify account | Infrastructure | Available |

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Large generated code file (4,868 lines) | Medium | May need modularization for web |
| Complex keygroup/zone structure | Medium | Build incrementally, test each layer |
| SysEx timing sensitivity | Low | Add retry logic and timeouts |
| Browser MIDI support | Medium | Document supported browsers |

## Timeline

| Phase | Target | Notes |
|-------|--------|-------|
| Phase 1: Web MIDI Adapter | Week 1 | Foundation work |
| Phase 2: App Scaffold | Week 1 | Leverage existing editors |
| Phase 3: Program List | Week 1-2 | Core navigation |
| Phase 4: Program Header | Week 2 | First editing capability |
| Phase 5: Keygroup List | Week 2 | Navigation layer |
| Phase 6: Keygroup Editing | Week 2-3 | Most complex part |
| Phase 7: Sample Zones | Week 3 | Sample assignment |
| Phase 8: LFO/Modulation | Week 3 | Polish |
| Phase 9: Deployment | Week 3-4 | Release |

## Existing Code Assets

### From sampler-devices/src/devices/s3000xl.ts (4,868 lines)

**Contains:**
- Complete TypeScript interfaces for all parameters
- Generated from Akai S3000XL specification
- Parameter labels, ranges, and byte mappings
- SysEx encoding/decoding utilities

**Key Interfaces:**
- `ProgramHeader` - 80+ program-level parameters
- `Keygroup` - Per-keygroup parameters
- `SampleZone` - Sample assignment and tuning
- `Filter` - Filter cutoff, resonance, envelope
- `Envelope` - ADSR for amplitude and filter

**May Need:**
- Web MIDI compatibility layer
- Async/await wrappers for browser
- Modularization if bundle size is concern
