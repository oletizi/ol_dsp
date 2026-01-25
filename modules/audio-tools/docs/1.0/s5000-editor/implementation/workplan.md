# S5000/S6000 Editor - Workplan

**GitHub Project:** [S5000/S6000 Editor](https://github.com/users/oletizi/projects/4)
**GitHub Milestone:** [Week of Jan 26-30](https://github.com/oletizi/ol_dsp/milestone/1)
**GitHub Issues:**

- [#60 - [audio-tools] S5000/S6000 Editor](https://github.com/oletizi/ol_dsp/issues/60) (Parent)
- [#61 - Create s5000-editor web application scaffold](https://github.com/oletizi/ol_dsp/issues/61)
- [#62 - Implement S5000/S6000 program file import and parsing](https://github.com/oletizi/ol_dsp/issues/62)
- [#63 - Implement S5000/S6000 program visualization and keygroup editing](https://github.com/oletizi/ol_dsp/issues/63)
- [#64 - Implement S5000/S6000 filter, envelope, and LFO editors](https://github.com/oletizi/ol_dsp/issues/64)
- [#65 - Implement S5000/S6000 program export and deployment](https://github.com/oletizi/ol_dsp/issues/65)

## Technical Approach

### Architecture

Following the same pattern as other editors, but with file-based workflow:

```
┌─────────────────────────────────────────────────────────────┐
│                   s5000-editor (Web App)                     │
├─────────────────────────────────────────────────────────────┤
│  React Components    │  Zustand Store  │  File Handling     │
│  - ProgramView       │  - Program state│  - Import .akp     │
│  - KeygroupList      │  - Edit state   │  - Export .akp     │
│  - KeygroupEditor    │  - UI state     │  - Drag & drop     │
│  - ZoneEditor        │                 │                    │
│  - FilterEditor      │                 │                    │
│  - EnvelopeEditor    │                 │                    │
│  - LfoEditor         │                 │                    │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              sampler-devices/s56k (Library)                  │
├─────────────────────────────────────────────────────────────┤
│  Program Parser      │  Chunk Handlers │  Types             │
│  - newProgramFromBuf │  - HeaderChunk  │  - AkaiS56kProgram │
│  - writeProgram      │  - KeygroupChunk│  - Keygroup        │
│  - validateProgram   │  - ZoneChunk    │  - Zone            │
│                      │  - FilterChunk  │  - Filter          │
│                      │  - LfoChunk     │  - Envelope        │
└──────────────────────┴─────────────────┴────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Browser File API                          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Local .akp Files (Import/Export)                │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **File-based workflow**: Unlike MIDI editors, S5000/S6000 editing is file-based
2. **Reuse existing s56k code**: 1,812 lines of program parsing already exists
3. **Browser File API**: Use drag-and-drop and file picker for import/export
4. **Same UI patterns**: Follow s330/jv1080/s3000xl editor component structure

## Implementation Phases

### Phase 1: Web Application Scaffold

**Goal:** Create s5000-editor module with basic React/Vite setup

**Tasks:**
- [ ] Copy and adapt existing editor scaffold
- [ ] Configure package.json and dependencies
- [ ] Set up Zustand store for program state
- [ ] Implement file import UI (drag-and-drop, file picker)
- [ ] Create basic routing structure

**Success Criteria:**
- Application builds and runs locally
- Can import .akp file via drag-and-drop
- Basic navigation works

### Phase 2: Program File Import

**Goal:** Parse and display S5000/S6000 program files

**Tasks:**
- [ ] Integrate s56k parser for browser environment
- [ ] Handle binary file reading via File API
- [ ] Parse program structure into Zustand store
- [ ] Display program metadata (name, keygroup count)
- [ ] Add error handling for invalid files

**Success Criteria:**
- Can import valid .akp files
- Program structure loaded into state
- Clear error messages for invalid files

### Phase 3: Program Structure Visualization

**Goal:** Display program structure with keygroup overview

**Tasks:**
- [ ] Create ProgramView component
- [ ] Display program header information
- [ ] Create KeygroupList component with key range visualization
- [ ] Show keygroup count and navigation
- [ ] Visual keyboard representation of mappings

**Success Criteria:**
- Program structure clearly visible
- Keygroups displayed with key ranges
- Can select individual keygroups

### Phase 4: Keygroup Editing

**Goal:** Comprehensive keygroup parameter editing

**Tasks:**
- [ ] Create KeygroupEditor component
- [ ] Implement key range controls (low/high key)
- [ ] Add velocity switching parameters
- [ ] Create ZoneEditor for 4 sample zones
- [ ] Display zone sample assignments
- [ ] Add zone tuning and level controls

**Success Criteria:**
- All keygroup parameters editable
- Zone assignments visible
- Changes reflected in state

### Phase 5: Filter and Envelope Editors

**Goal:** Visual filter and envelope editing

**Tasks:**
- [ ] Create FilterEditor component
- [ ] Implement filter type selector
- [ ] Add cutoff and resonance controls
- [ ] Create AmpEnvelopeEditor (ADSR)
- [ ] Create FilterEnvelopeEditor
- [ ] Add envelope visualization graphs

**Success Criteria:**
- Filter parameters fully editable
- Envelope visualization accurate
- Real-time visual feedback

### Phase 6: LFO and Modulation Editors

**Goal:** LFO and modulation routing controls

**Tasks:**
- [ ] Create Lfo1Editor component
- [ ] Create Lfo2Editor component
- [ ] Implement LFO waveform selection
- [ ] Add rate, depth, delay controls
- [ ] Create ModsEditor for modulation routing
- [ ] Create AuxEnvelopeEditor

**Success Criteria:**
- Both LFOs fully editable
- Modulation routing visible
- Aux envelope editable

### Phase 7: Program Export

**Goal:** Export edited programs back to .akp format

**Tasks:**
- [ ] Implement program serialization
- [ ] Create binary writer for chunks
- [ ] Add export button and file download
- [ ] Validate exported files
- [ ] Test round-trip (import → edit → export → import)

**Success Criteria:**
- Can export valid .akp files
- Exported files load in sampler
- No data loss in round-trip

### Phase 8: Deployment and Polish

**Goal:** Deploy and prepare for release

**Tasks:**
- [ ] Configure Netlify deployment
- [ ] Add loading states and progress indicators
- [ ] Improve error handling and messages
- [ ] Write user documentation
- [ ] Browser compatibility testing
- [ ] Add sample program files for testing

**Success Criteria:**
- Deployed and accessible on Netlify
- Graceful error handling
- Documentation complete

## Dependencies

| Dependency | Type | Status |
|------------|------|--------|
| sampler-devices/s56k | Internal | Exists (1,812 lines) |
| Browser File API | Browser | Standard, well-supported |
| Netlify account | Infrastructure | Available |

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Browser binary handling | Medium | Use ArrayBuffer and DataView APIs |
| Complex chunk structure | Medium | Existing parser handles complexity |
| Export validation | Medium | Test with real sampler hardware |
| Large program files | Low | Optimize parsing, show progress |

## Timeline

| Phase | Target | Notes |
|-------|--------|-------|
| Phase 1: App Scaffold | Week 1 | Leverage existing editors |
| Phase 2: File Import | Week 1 | Core functionality |
| Phase 3: Visualization | Week 1-2 | Program overview |
| Phase 4: Keygroup Editing | Week 2 | Main editing capability |
| Phase 5: Filter/Envelope | Week 2-3 | Visual editors |
| Phase 6: LFO/Modulation | Week 3 | Complete parameter coverage |
| Phase 7: Export | Week 3 | Full workflow |
| Phase 8: Deployment | Week 3-4 | Release |

## Existing Code Assets

### From sampler-devices/src/devices/s56k*.ts (1,812 lines)

**s56k-types.ts (682 lines):**
- AkaiS56kProgram - main program interface
- Keygroup, Zone, Filter interfaces
- AmpEnvelope, FilterEnvelope, AuxEnvelope
- Lfo1, Lfo2, Mods interfaces
- All chunk type definitions

**s56k-chunks.ts (406 lines):**
- Chunk factory functions
- Chunk header parsing
- Binary to chunk conversion

**s56k-parser.ts (335 lines):**
- Binary file parsing
- Chunk iteration and extraction
- Program assembly from chunks

**s56k-program.ts (158 lines):**
- newProgramFromBuffer - main parsing entry point
- newProgramFromJson - JSON construction
- Program validation

**s56k-utils.ts (145 lines):**
- bytes2Number, bytes2String utilities
- Binary conversion helpers

**May Need:**
- Browser-compatible binary handling
- Program writer/serializer for export
- Enhanced validation for editing
