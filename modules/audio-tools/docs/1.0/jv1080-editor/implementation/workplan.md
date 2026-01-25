# JV-1080 Editor - Workplan

**GitHub Project:** [JV-1080 Editor](https://github.com/users/oletizi/projects/2)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#49 - [audio-tools] JV-1080 Editor](https://github.com/oletizi/ol_dsp/issues/49) (Parent)
- [#50 - Extract JV-1080 client from sampler-attic](https://github.com/oletizi/ol_dsp/issues/50)
- [#51 - Create jv1080-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/51)
- [#52 - Implement JV-1080 system parameter controls](https://github.com/oletizi/ol_dsp/issues/52)
- [#53 - Implement JV-1080 effects editor](https://github.com/oletizi/ol_dsp/issues/53)

## Technical Approach

### Architecture

Following the same pattern as s330-editor:

```
┌─────────────────────────────────────────────────────────────┐
│                   jv1080-editor (Web App)                    │
├─────────────────────────────────────────────────────────────┤
│  React Components    │  Zustand Store  │  React Query       │
│  - SystemControls    │  - MIDI state   │  - Data fetching   │
│  - PatchSelector     │  - Device state │  - Caching         │
│  - EffectsEditor     │  - UI state     │                    │
│  - PatchNameEditor   │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│             sampler-devices/jv1080 (Library)                 │
├─────────────────────────────────────────────────────────────┤
│  JV1080Client        │  Messages       │  Addresses         │
│  - connect()         │  - encode()     │  - SYSTEM_BASE     │
│  - getSystemParams() │  - decode()     │  - TEMP_PATCH      │
│  - setFxType()       │  - checksum()   │  - FX_TYPES        │
│  - setPatchName()    │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Web MIDI API (Browser)                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Roland JV-1080 Hardware                    │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Reuse s330-editor architecture**: Same stack, similar component structure
2. **Extract from sampler-attic**: Refactor existing code into proper module structure
3. **Event-based updates**: Subscribe to parameter changes from hardware
4. **Centralized caching**: Client maintains state to minimize polling

## Implementation Phases

### Phase 1: Code Extraction and Client Setup

**Goal:** Extract JV-1080 code from sampler-attic into sampler-devices

**Tasks:**
- [ ] Create `src/devices/jv1080/` directory structure
- [ ] Extract and refactor `roland-jv-1080.ts` code
- [ ] Create proper TypeScript interfaces (`jv1080-types.ts`)
- [ ] Separate address constants (`jv1080-addresses.ts`)
- [ ] Implement message encoding (`jv1080-messages.ts`)
- [ ] Add caching layer to client (`jv1080-client.ts`)
- [ ] Write unit tests

**Success Criteria:**
- Clean module structure matching s330 pattern
- All existing functionality preserved
- TypeScript strict mode compliance
- Unit tests passing

### Phase 2: Web Application Scaffold

**Goal:** Create jv1080-editor module with basic React/Vite setup

**Tasks:**
- [ ] Copy and adapt s330-editor scaffold
- [ ] Configure package.json and dependencies
- [ ] Set up Zustand store for JV-1080 state
- [ ] Implement MIDI device selection UI
- [ ] Create basic routing structure

**Success Criteria:**
- Application builds and runs locally
- Can select MIDI input/output devices
- Basic navigation works

### Phase 3: System Parameter Controls

**Goal:** Implement system-level parameter editing

**Tasks:**
- [ ] Panel mode selector (Performance/Patch/GM)
- [ ] Performance number control
- [ ] Patch group selector (User/PCM)
- [ ] Patch number control
- [ ] FX/Chorus/Reverb switches
- [ ] Clock source selector (Internal/MIDI)

**Success Criteria:**
- All system parameters editable
- Changes sync to hardware immediately
- Visual feedback for current state

### Phase 4: Effects Editor

**Goal:** Implement comprehensive effects editing

**Tasks:**
- [ ] FX type selector (40 types with grouping)
- [ ] Dynamic parameter controls based on FX type
- [ ] Parameter value feedback from hardware
- [ ] Visual effect chain representation

**Success Criteria:**
- All 40 FX types selectable
- Parameters display correctly per FX type
- Real-time hardware sync

### Phase 5: Patch Management

**Goal:** Implement patch selection and naming

**Tasks:**
- [ ] Patch name editor (12 characters)
- [ ] Patch browser/selector UI
- [ ] Patch remain toggle
- [ ] Performance number quick select

**Success Criteria:**
- Can edit patch names
- Can navigate patches easily
- Changes persist to hardware

### Phase 6: Deployment and Polish

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
| sampler-devices module | Internal | Exists, needs JV-1080 additions |
| sampler-attic code | Internal | Exists, needs extraction |
| Netlify account | Infrastructure | Available |

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Incomplete SysEx documentation | Medium | Test with real hardware, reverse engineer |
| Different JV-1080 firmware versions | Low | Test with available unit, document differences |
| FX parameter variations by type | Medium | Build parameter maps per FX type |

## Timeline

| Phase | Target | Notes |
|-------|--------|-------|
| Phase 1: Code Extraction | Week 1 | Foundation work |
| Phase 2: App Scaffold | Week 1 | Leverage s330 work |
| Phase 3: System Controls | Week 2 | Core functionality |
| Phase 4: Effects Editor | Week 2-3 | Most complex part |
| Phase 5: Patch Management | Week 3 | Polish |
| Phase 6: Deployment | Week 3 | Release |

## Code Migration Notes

### From sampler-attic/src/midi/roland-jv-1080.ts

**Keep:**
- SysEx constants (ROLAND_MANUFACTURER_ID, JV_1080_MODEL_ID, etc.)
- Address definitions (BASE_SYSTEM, OFFSET_*, BASE_TEMP_PATCH)
- FX_TYPES array
- Checksum calculation
- Event subscription pattern

**Refactor:**
- Split monolithic file into proper module structure
- Add TypeScript interfaces for all data types
- Implement caching layer (like S330Client)
- Improve error handling
- Add JSDoc documentation

**Remove:**
- Console.log debugging statements
- Any test/experiment code
