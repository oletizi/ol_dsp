# Feature 360 Architecture

**Version:** 1.21
**Status:** In Progress
**Last Updated:** 2025-10-11

## Overview

The 360 feature implements a modular, extensible architecture for deploying MIDI controller configurations to multiple DAWs. The design follows interface-first principles with clear separation of concerns across three main layers:

1. **Controller Abstraction Layer** - Hardware interrogation and configuration extraction
2. **Canonical Conversion Layer** - Format translation to universal MIDI mappings
3. **DAW Deployment Layer** - Platform-specific map generation and installation

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          Feature 360 Architecture                       │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐
│ Controller Layer    │  Hardware-specific adapters
├─────────────────────┤
│ • ControllerAdapter │  Interface for device interrogation
│ • Device Manager    │  Connection management
│ • Mode Reader       │  Custom mode extraction
└──────────┬──────────┘
           │ ControllerConfiguration
           ↓
┌─────────────────────┐
│ Conversion Layer    │  Format translation
├─────────────────────┤
│ • CanonicalConverter│  Interface for conversion
│ • Parameter Mapper  │  Control → parameter mapping
│ • Metadata Builder  │  Map metadata creation
└──────────┬──────────┘
           │ CanonicalMidiMap
           ↓
┌─────────────────────┐
│ Deployment Layer    │  DAW-specific generation
├─────────────────────┤
│ • DAWDeployer       │  Interface for deployment
│ • Ardour Deployer   │  XML generation
│ • Live Deployer     │  JSON generation
└──────────┬──────────┘
           │ DeploymentResult
           ↓
┌─────────────────────┐
│ Target DAW          │
└─────────────────────┘
```

## Core Components

### 1. Controller-Workflow Module

**Purpose:** Generic framework for controller → DAW deployment

**Location:** `modules/controller-workflow/`

**Key Abstractions:**

```typescript
// Controller interrogation interface
interface ControllerAdapterInterface {
  connect(): Promise<void>;
  listConfigurations(): Promise<ConfigurationSlot[]>;
  readConfiguration(slot: number): Promise<ControllerConfiguration>;
}

// Canonical conversion interface
interface CanonicalConverterInterface {
  convert(config: ControllerConfiguration, options: ConversionOptions): CanonicalMidiMap;
  canConvert(config: ControllerConfiguration): boolean;
}

// DAW deployment interface
interface DAWDeployerInterface {
  deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult>;
  isInstalled(): Promise<boolean>;
}
```

**Design Principles:**
- **Interface-first:** All adapters implement well-defined interfaces
- **Dependency injection:** Components accept dependencies via constructor
- **Factory pattern:** Auto-detection and instantiation via factories
- **Event-driven:** Progress reporting via EventEmitter

See [implementation/workplan.md](./implementation/workplan.md) for detailed design.

### 2. Launch Control XL3 Adapter

**Purpose:** Reference implementation for Novation Launch Control XL 3

**Location:** `modules/controller-workflow/src/adapters/controllers/LaunchControlXL3Adapter.ts`

**Capabilities:**
- Read all 16 custom mode slots
- Extract control configurations (48 controls: 24 encoders, 8 sliders, 16 buttons)
- Map LCXL3-specific IDs (SEND_A1, FADER1, etc.) to generic control types
- Wrapper around existing launch-control-xl3 library

**Control Mapping:**
```
LCXL3 ID     → Generic ID    → MIDI CC
─────────────────────────────────────────
SEND_A1      → encoder_1     → CC 13
SEND_A2      → encoder_2     → CC 14
...
FADER1       → slider_1      → CC 53
FADER2       → slider_2      → CC 54
...
FOCUS1       → button_1      → CC 41
FOCUS2       → button_2      → CC 42
```

### 3. Canonical MIDI Maps

**Purpose:** Universal MIDI mapping format (DAW-agnostic)

**Location:** `modules/canonical-midi-maps/`

**Format:** YAML

**Structure:**
```yaml
version: 1.0.0

device:
  manufacturer: Novation
  model: Launch Control XL 3

plugin:
  manufacturer: TAL Software
  name: TAL-J-8
  descriptor: plugin-descriptors/tal-togu-audio-line-tal-j-8.json

metadata:
  name: Jupiter-8 Template
  description: Controls for TAL-J-8 synth
  date: 2025-10-11

midi_channel: 1  # Global MIDI channel

controls:
  - id: encoder_1
    name: VCF Cutoff
    type: encoder
    cc: 13
    channel: 1
    plugin_parameter: 105  # Real parameter index from descriptor
    range: [0, 127]
```

**Key Features:**
- **Real parameter indices:** Uses plugin descriptors for accurate mappings
- **Hardware abstraction:** Generic control IDs work across devices
- **Plugin awareness:** Links to plugin descriptors for validation
- **Metadata rich:** Version tracking, tags, creation dates

See [project ARCHITECTURE.md](../../ARCHITECTURE.md) for component relationships.

### 4. Ardour Deployer

**Purpose:** Generate Ardour MIDI map XML files

**Location:** `modules/controller-workflow/src/adapters/daws/ArdourDeployer.ts`

**Process:**
```
CanonicalMidiMap → ArdourMidiMap → XML → Install
                   (MidiMapBuilder)   (Serializer)
```

**Output Format:** Ardour XML
```xml
<ArdourMIDIBindings version="1.0.0" name="LCXL3 - TAL-J-8">
  <DeviceInfo bank-size="8"/>
  <Binding channel="1" ctl="13" function="plugin-parameter" uri="TAL-J-8/param/105"/>
  <Binding channel="1" ctl="53" function="plugin-parameter" uri="TAL-J-8/param/65"/>
</ArdourMIDIBindings>
```

**Installation:**
- **macOS:** `~/Library/Preferences/Ardour8/midi_maps/`
- **Linux:** `~/.config/ardour8/midi_maps/`
- **Windows:** `%APPDATA%\Ardour8\midi_maps\`

### 5. Live Deployer (Dual-Pipeline Architecture)

**Purpose:** Deploy to Ableton Live via Max for Live

**Location:** `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts`

**Architecture:** Two-tier mapping system

```
┌────────────────────────────────────────────────────────────┐
│                    Dual-Pipeline System                    │
└────────────────────────────────────────────────────────────┘

Tier 1: Canonical (Build-time)                 Tier 2: Runtime (Device-extracted)
────────────────────────────                    ──────────────────────────────────
Source: YAML files                              Source: Hardware custom modes
        (version-controlled)                             (extracted via LiveDeployer)

Build:  convert-canonical-maps.cjs              Extract: controller-deploy deploy
        ↓                                                ↓
Output: canonical-plugin-maps.ts                Output:  plugin-mappings.json

Load:   Import at compile-time                  Load:    Runtime JSON parsing
        (build artifact)                                 (runtime-loader.ts)

Updates: Via PR/commit workflow                 Updates: One-click from hardware

                              Runtime Merge
                              ─────────────
                    { ...CANONICAL_PLUGIN_MAPS, ...runtimeMaps }
                              (runtime wins)
```

**Benefits:**
- **Tier 1** provides curated, version-controlled defaults
- **Tier 2** enables one-click deployment from hardware
- **No conflicts** with build pipeline (separate data files)
- **Runtime override** allows user customization

See [live-deployer/architecture.md](./live-deployer/architecture.md) for details.

## Data Flow

### Complete 360 Pipeline

```
1. Hardware Interrogation
   ┌──────────────────────┐
   │ Launch Control XL3   │
   │ Custom Mode (Slot 0) │
   └──────────┬───────────┘
              │ SysEx read
              ↓
   ┌──────────────────────┐
   │ LCXL3Adapter         │
   │ .readConfiguration() │
   └──────────┬───────────┘
              │ ControllerConfiguration
              ↓

2. Canonical Conversion
   ┌──────────────────────┐
   │ LCXL3Converter       │
   │ .convert()           │
   └──────────┬───────────┘
              │ CanonicalMidiMap (YAML/JSON)
              ↓

3. DAW Deployment
   ┌──────────────────────┐         ┌──────────────────────┐
   │ ArdourDeployer       │         │ LiveDeployer         │
   │ .deploy()            │         │ .deploy()            │
   └──────────┬───────────┘         └──────────┬───────────┘
              │ XML                             │ JSON
              ↓                                 ↓
   ┌──────────────────────┐         ┌──────────────────────┐
   │ Ardour MIDI Maps     │         │ Max for Live         │
   │ (installed)          │         │ (runtime-loader)     │
   └──────────────────────┘         └──────────────────────┘
```

### Workflow Orchestration

```typescript
// Automated workflow via DeploymentWorkflow
const workflow = await DeploymentWorkflow.create({
  targets: ['ardour', 'live']
});

const result = await workflow.execute({
  configSlot: 0,                    // Hardware slot
  targets: ['ardour', 'live'],      // Multiple DAWs
  pluginInfo: { name: 'TAL-J-8' },
  midiChannel: 1,
  preserveLabels: true,
  autoInstall: true
});

// Result contains:
// - controllerConfig: ControllerConfiguration
// - canonicalMap: CanonicalMidiMap
// - deployments: DeploymentResult[]
// - errors: string[]
```

## Extensibility

### Adding a New Controller

1. **Implement ControllerAdapter:**
   ```typescript
   export class NewControllerAdapter implements ControllerAdapterInterface {
     // Implement all interface methods
   }
   ```

2. **Implement CanonicalConverter:**
   ```typescript
   export class NewControllerConverter implements CanonicalConverterInterface {
     convert(config: ControllerConfiguration, options: ConversionOptions): CanonicalMidiMap {
       // Device-specific → canonical mapping
     }
   }
   ```

3. **Register in Workflow:**
   - Update `DeploymentWorkflow.detectController()`
   - Update `DeploymentWorkflow.getConverterFor()`

**No changes required:** Orchestrator, CLI, DAW deployers all work generically.

### Adding a New DAW

1. **Implement DAWDeployer:**
   ```typescript
   export class NewDAWDeployer implements DAWDeployerInterface {
     async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
       // Generate DAW-specific format
     }
   }
   ```

2. **Register in Workflow:**
   - Update `DeploymentWorkflow.create()` to include new deployer

**No changes required:** Controller adapters, converters, orchestrator all work generically.

## Technology Stack

| Layer | Technologies |
|-------|--------------|
| **Core** | TypeScript 5.3+, ESM modules, Strict mode |
| **MIDI** | WebMIDI API (browsers), node-midi (Node.js), SysEx parsing |
| **Data Formats** | YAML (canonical), JSON (runtime), XML (Ardour) |
| **Build** | Vite (bundling), TypeScript compiler, pnpm workspaces |
| **Testing** | Vitest, dependency injection for mocking |
| **Documentation** | Markdown, JSDoc, TypeScript types as documentation |

## Design Patterns

### Interface-First Design

All major components define interfaces before implementation:
- Enables testing with mocks
- Clear contracts between layers
- Easier to extend and maintain

### Dependency Injection

```typescript
export class DeploymentWorkflow {
  constructor(
    private controllerAdapter: ControllerAdapterInterface,
    private converter: CanonicalConverterInterface,
    private deployers: Map<string, DAWDeployerInterface>
  ) {}
}
```

Benefits:
- Testable (inject mocks)
- Flexible (swap implementations)
- Follows project guidelines

### Factory Pattern

```typescript
static async create(options: CreateOptions): Promise<DeploymentWorkflow> {
  const adapter = options.controllerAdapter || await this.detectController();
  const converter = this.getConverterFor(adapter);
  const deployers = new Map();
  // ...
  return new DeploymentWorkflow(adapter, converter, deployers);
}
```

Benefits:
- Auto-detection of hardware
- Simplified instantiation
- Encapsulates complexity

### Event-Driven Progress

```typescript
workflow.on('progress', ({ step, message }) => {
  console.log(`[${step}/3] ${message}`);
});

workflow.on('canonical-saved', ({ path }) => {
  console.log(`Saved: ${path}`);
});
```

Benefits:
- Real-time feedback
- Decoupled UI/logging
- Extensible event system

## Performance Considerations

### Target Metrics

| Operation | Target | Current Status |
|-----------|--------|----------------|
| Controller read | <2s | ✅ Sub-second |
| Canonical conversion | <100ms | ⏳ Not measured |
| Ardour XML generation | <50ms | ⏳ Not measured |
| Live JSON write | <20ms | ✅ Instant |
| End-to-end workflow | <10s | ⏳ Not measured |

### Optimization Strategies

- **Caching:** Controller configurations cached between deployments
- **Lazy loading:** DAW deployers loaded on-demand
- **Async operations:** Non-blocking I/O for file operations
- **Streaming:** Large maps processed in chunks

## Security Considerations

- **File system access:** Validates output paths before writing
- **MIDI input:** Validates SysEx messages before parsing
- **Command injection:** No shell command execution (direct API calls only)
- **Path traversal:** Sanitizes file paths in output directory options

## Cross-References

- **[Main Workplan](./implementation/workplan.md)** - Detailed implementation phases
- **[LiveDeployer Architecture](./live-deployer/architecture.md)** - Dual-pipeline system
- **[Project Architecture](../../ARCHITECTURE.md)** - Overall component relationships
- **[Module READMEs](../../../modules/)** - Component-specific documentation

---

**Next:** See [workflow.md](./workflow.md) for the complete user workflow
