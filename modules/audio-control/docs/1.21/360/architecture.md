# Feature 360 Architecture

**Version:** 1.21
**Status:** In Progress
**Last Updated:** 2025-10-12

## Overview

The 360 feature implements a modular, extensible architecture for deploying MIDI controller configurations to multiple DAWs. The design follows interface-first principles with clear separation of concerns across four main layers:

1. **Controller Abstraction Layer** - Hardware interrogation and configuration extraction
2. **Canonical Conversion Layer** - Format translation to universal MIDI mappings
3. **Parameter Matching Layer** - AI-powered fuzzy matching of control names to plugin parameters
4. **DAW Deployment Layer** - Platform-specific map generation and installation

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
           │ CanonicalMidiMap (without plugin_parameter)
           ↓
┌─────────────────────┐
│ Matching Layer ⭐   │  AI-powered parameter matching
├─────────────────────┤
│ • Parameter Matcher │  Claude Code CLI integration
│ • Descriptor Loader │  Plugin descriptor reader
│ • Confidence Scorer │  Match quality validation
└──────────┬──────────┘
           │ CanonicalMidiMap (with plugin_parameter)
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

// Parameter matching interface (NEW)
interface ParameterMatcherInterface {
  matchParameters(
    controlNames: string[],
    pluginDescriptor: PluginDescriptor
  ): Promise<ParameterMatch[]>;
  getConfidence(match: ParameterMatch): number;
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
- **AI-augmented:** Optional Claude Code CLI integration for parameter matching

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
  date: 2025-10-12
  aiMatchingEnabled: true  # NEW: Indicates fuzzy matching was used

midi_channel: 1  # Global MIDI channel

controls:
  - id: encoder_1
    name: VCF Cutoff
    type: encoder
    cc: 13
    channel: 1
    plugin_parameter: 105      # Added by AI matcher
    match_confidence: 1.0       # NEW: Match quality
    range: [0, 127]
```

**Key Features:**
- **Real parameter indices:** Uses plugin descriptors for accurate mappings
- **Hardware abstraction:** Generic control IDs work across devices
- **Plugin awareness:** Links to plugin descriptors for validation
- **Metadata rich:** Version tracking, tags, creation dates
- **AI-enhanced:** Optional fuzzy matching with confidence scores

See [project ARCHITECTURE.md](../../ARCHITECTURE.md) for component relationships.

### 3.5. Parameter Matching Service ⭐

**Purpose:** AI-powered fuzzy matching of control names to plugin parameters

**Location:** `modules/controller-workflow/src/services/ParameterMatcher.ts`

**Architecture:**

```
┌───────────────────────────────────────────────────────────────────┐
│                   Parameter Matching Service                      │
└───────────────────────────────────────────────────────────────────┘

Input:
┌────────────────────┐         ┌────────────────────┐
│ Control Names      │         │ Plugin Descriptor  │
│ ──────────────────│         │ ──────────────────│
│ • VCF Cutoff       │         │ • [105] VCF Cutoff │
│ • Filter Res       │         │ • [107] VCF Res... │
│ • Env Attack       │         │ • [65] Attack Time │
└────────────────────┘         └────────────────────┘
         │                              │
         └──────────┬───────────────────┘
                    ↓
         ┌─────────────────────┐
         │  Parameter Matcher  │
         │  ─────────────────  │
         │  • Load descriptor  │
         │  • Build AI prompt  │
         │  • Call Claude CLI  │
         │  • Parse response   │
         │  • Validate matches │
         └─────────────────────┘
                    ↓
         ┌─────────────────────┐
         │  Claude Code CLI    │
         │  ─────────────────  │
         │  Model: Claude 3.5  │
         │  Integration: CLI   │
         │  Semantic matching  │
         └─────────────────────┘
                    ↓
Output:
┌────────────────────────────────────────────┐
│ Parameter Matches                          │
│ ──────────────────────────────────────────│
│ • VCF Cutoff → 105 (confidence: 1.0)      │
│ • Filter Res → 107 (confidence: 0.95)     │
│ • Env Attack → 65 (confidence: 0.9)       │
└────────────────────────────────────────────┘
```

**Key Capabilities:**

1. **Semantic Matching**
   - Exact matches: "VCF Cutoff" → "VCF Cutoff"
   - Abbreviations: "Filter Res" → "VCF Resonance"
   - Synonyms: "Env Attack" → "Attack Time"
   - Context-aware: Groups related parameters

2. **Integration Method**
   - **Claude Code CLI:** Exclusively uses Claude Code CLI for AI integration
   - **Authentication:** Leverages Claude Code's built-in authentication
   - **Caching:** Results cached in canonical YAML for performance

3. **Confidence Scoring**
   - 1.0: Exact string match
   - 0.9-0.99: High confidence (clear synonym)
   - 0.7-0.89: Medium confidence (abbreviation/context)
   - < 0.7: Low confidence (ambiguous match)

4. **Error Handling**
   - Graceful degradation if CLI unavailable
   - Retry logic with exponential backoff
   - Cache results for performance
   - Manual override support

**Interface:**

```typescript
interface ParameterMatcherInterface {
  /**
   * Match control names to plugin parameters using AI
   */
  matchParameters(
    controlNames: string[],
    pluginDescriptor: PluginDescriptor,
    options?: MatchOptions
  ): Promise<ParameterMatch[]>;

  /**
   * Get match confidence score (0-1)
   */
  getConfidence(match: ParameterMatch): number;

  /**
   * Check if matcher is available (CLI installed and authenticated)
   */
  isAvailable(): Promise<boolean>;
}

interface ParameterMatch {
  controlName: string;
  controlCC: number;
  parameterIndex: number;
  parameterName: string;
  confidence: number;
  matchType: 'exact' | 'abbreviation' | 'synonym' | 'context';
}

interface MatchOptions {
  confidenceThreshold?: number;  // Default: 0.7
  enableCaching?: boolean;        // Default: true
  model?: string;                 // Default: 'claude-3-5-sonnet-20241022'
}
```

**Example Usage:**

```typescript
import { ParameterMatcher } from '@/services/ParameterMatcher';

const matcher = new ParameterMatcher({
  useCLI: true  // Uses Claude Code CLI
});

// Load plugin descriptor
const descriptor = await loadPluginDescriptor('tal-j-8');

// Extract control names from hardware
const controlNames = ['VCF Cutoff', 'Filter Res', 'Env Attack'];

// Match parameters
const matches = await matcher.matchParameters(controlNames, descriptor);

// Apply matches to canonical map
for (const match of matches) {
  const control = canonicalMap.controls.find(c => c.name === match.controlName);
  if (control && match.confidence >= 0.7) {
    control.plugin_parameter = match.parameterIndex;
    control.match_confidence = match.confidence;
  }
}
```

**Performance:**
- Single CLI call batches all controls
- Results cached in canonical YAML
- Typical matching time: 2-5 seconds
- Subsequent deployments: instant (uses cache)

**Dependencies:**
- Claude Code CLI (required)
- Plugin descriptor files (from Phase 1)

### 4. Ardour Deployer

**Purpose:** Generate Ardour MIDI map XML files

**Location:** `modules/controller-workflow/src/adapters/daws/ArdourDeployer.ts`

**Process:**
```
CanonicalMidiMap → ArdourMidiMap → XML → Install
                   (MidiMapBuilder)   (Serializer)
```

**Output Format:** Ardour XML (with AI-matched parameters)
```xml
<ArdourMIDIBindings version="1.0.0" name="LCXL3 - TAL-J-8">
  <DeviceInfo bank-size="8"/>
  <!-- AI-matched parameters -->
  <Binding channel="1" ctl="13" function="plugin-parameter"
           uri="TAL-J-8/param/105"/>  <!-- VCF Cutoff (confidence: 1.0) -->
  <Binding channel="1" ctl="53" function="plugin-parameter"
           uri="TAL-J-8/param/65"/>   <!-- Attack (confidence: 0.9) -->
</ArdourMIDIBindings>
```

**Installation:**
- **macOS:** `~/Library/Preferences/Ardour8/midi_maps/`
- **Linux:** `~/.config/ardour8/midi_maps/`
- **Windows:** `%APPDATA%\Ardour8\midi_maps\`

### 5. Live Deployer (Dual-Pipeline Architecture)

**Purpose:** Deploy to Ableton Live via Max for Live

**Location:** `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts`

**Architecture:** Two-tier mapping system with AI matching

```
┌────────────────────────────────────────────────────────────┐
│                    Dual-Pipeline System                    │
└────────────────────────────────────────────────────────────┘

Tier 1: Canonical (Build-time)                 Tier 2: Runtime (Device-extracted + AI)
────────────────────────────                    ──────────────────────────────────
Source: YAML files                              Source: Hardware custom modes
        (version-controlled)                             (extracted via LiveDeployer)

Build:  convert-canonical-maps.cjs              Extract: controller-deploy deploy
        ↓                                       AI Match: ParameterMatcher (CLI)
Output: canonical-plugin-maps.ts                        ↓
                                                Output:  plugin-mappings.json
                                                         (with AI-matched params)

Load:   Import at compile-time                  Load:    Runtime JSON parsing
        (build artifact)                                 (runtime-loader.ts)

Updates: Via PR/commit workflow                 Updates: One-click from hardware

                              Runtime Merge
                              ─────────────
                    { ...CANONICAL_PLUGIN_MAPS, ...runtimeMaps }
                              (runtime wins, includes AI matches)
```

**Benefits:**
- **Tier 1** provides curated, version-controlled defaults
- **Tier 2** enables one-click deployment with AI parameter matching via Claude Code CLI
- **No conflicts** with build pipeline (separate data files)
- **Runtime override** allows user customization
- **AI-enhanced** parameter bindings for better plugin control

See [live-deployer/architecture.md](./live-deployer/architecture.md) for details.

## Data Flow

### Complete 360 Pipeline (with AI Matching)

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
              │ CanonicalMidiMap (no plugin_parameter)
              ↓

2.5. AI Parameter Matching ⭐
   ┌──────────────────────┐
   │ ParameterMatcher     │
   │ .matchParameters()   │
   │ ├─ Load descriptor   │
   │ ├─ Call Claude CLI   │
   │ └─ Validate matches  │
   └──────────┬───────────┘
              │ CanonicalMidiMap (with plugin_parameter)
              ↓

3. DAW Deployment
   ┌──────────────────────┐         ┌──────────────────────┐
   │ ArdourDeployer       │         │ LiveDeployer         │
   │ .deploy()            │         │ .deploy()            │
   └──────────┬───────────┘         └──────────┬───────────┘
              │ XML                             │ JSON
              │ (plugin-specific URIs)          │ (with match confidence)
              ↓                                 ↓
   ┌──────────────────────┐         ┌──────────────────────┐
   │ Ardour MIDI Maps     │         │ Max for Live         │
   │ (installed)          │         │ (runtime-loader)     │
   └──────────────────────┘         └──────────────────────┘
```

### Workflow Orchestration (with AI Matching)

```typescript
// Automated workflow via DeploymentWorkflow
const workflow = await DeploymentWorkflow.create({
  targets: ['ardour', 'live'],
  enableAIMatching: true  // NEW: Enable fuzzy parameter matching
});

const result = await workflow.execute({
  configSlot: 0,                    // Hardware slot
  targets: ['ardour', 'live'],      // Multiple DAWs
  pluginInfo: { name: 'TAL-J-8' },  // Triggers AI matching
  midiChannel: 1,
  preserveLabels: true,
  autoInstall: true
});

// Result contains:
// - controllerConfig: ControllerConfiguration
// - canonicalMap: CanonicalMidiMap (with AI-matched parameters)
// - parameterMatches: ParameterMatch[] (NEW: Match details)
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

**No changes required:** Orchestrator, CLI, DAW deployers, and AI matcher all work generically.

### Adding a New DAW

1. **Implement DAWDeployer:**
   ```typescript
   export class NewDAWDeployer implements DAWDeployerInterface {
     async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
       // Generate DAW-specific format
       // AI-matched plugin_parameter fields available in canonicalMap
     }
   }
   ```

2. **Register in Workflow:**
   - Update `DeploymentWorkflow.create()` to include new deployer

**No changes required:** Controller adapters, converters, orchestrator, and AI matcher all work generically.

## Technology Stack

| Layer | Technologies |
|-------|--------------|
| **Core** | TypeScript 5.3+, ESM modules, Strict mode |
| **MIDI** | WebMIDI API (browsers), node-midi (Node.js), SysEx parsing |
| **AI Integration** | Claude Code CLI |
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
- AI matcher is optional dependency

### Dependency Injection

```typescript
export class DeploymentWorkflow {
  constructor(
    private controllerAdapter: ControllerAdapterInterface,
    private converter: CanonicalConverterInterface,
    private parameterMatcher: ParameterMatcherInterface,  // NEW
    private deployers: Map<string, DAWDeployerInterface>
  ) {}
}
```

Benefits:
- Testable (inject mocks)
- Flexible (swap implementations)
- Follows project guidelines
- AI matcher can be replaced or disabled

### Factory Pattern

```typescript
static async create(options: CreateOptions): Promise<DeploymentWorkflow> {
  const adapter = options.controllerAdapter || await this.detectController();
  const converter = this.getConverterFor(adapter);
  const matcher = options.enableAIMatching
    ? new ParameterMatcher()
    : new NoOpMatcher();  // Graceful degradation
  const deployers = new Map();
  // ...
  return new DeploymentWorkflow(adapter, converter, matcher, deployers);
}
```

Benefits:
- Auto-detection of hardware
- Simplified instantiation
- Encapsulates complexity
- Optional AI matching

### Event-Driven Progress

```typescript
workflow.on('progress', ({ step, message }) => {
  console.log(`[${step}/4] ${message}`);  // Updated for AI matching step
});

workflow.on('ai-matching', ({ controlCount, duration }) => {
  console.log(`AI matched ${controlCount} parameters in ${duration}ms`);
});

workflow.on('canonical-saved', ({ path }) => {
  console.log(`Saved: ${path}`);
});
```

Benefits:
- Real-time feedback
- Decoupled UI/logging
- Extensible event system
- AI matching progress visibility

## Performance Considerations

### Target Metrics

| Operation | Target | Current Status |
|-----------|--------|----------------|
| Controller read | <2s | ✅ Sub-second |
| Canonical conversion | <100ms | ⏳ Not measured |
| AI parameter matching | <5s | ⏳ In progress |
| Ardour XML generation | <50ms | ⏳ Not measured |
| Live JSON write | <20ms | ✅ Instant |
| End-to-end workflow | <10s | ⏳ Not measured (~6s with AI) |

### Optimization Strategies

- **Caching:** Controller configurations cached between deployments
- **AI result caching:** Matched parameters saved in canonical YAML
- **Batch matching:** All controls matched in single CLI call
- **Lazy loading:** DAW deployers loaded on-demand
- **Async operations:** Non-blocking I/O for file operations
- **Streaming:** Large maps processed in chunks
- **Graceful degradation:** Falls back to generic mappings if AI unavailable

## Security Considerations

- **File system access:** Validates output paths before writing
- **MIDI input:** Validates SysEx messages before parsing
- **Authentication:** Leverages Claude Code's secure authentication
- **Command execution:** Only trusted Claude Code CLI commands
- **Path traversal:** Sanitizes file paths in output directory options
- **AI prompts:** Input validation to prevent prompt injection
- **Rate limiting:** Respects Claude Code CLI rate limits

## Cross-References

- **[Main Workplan](./implementation/workplan.md)** - Detailed implementation phases
- **[Workflow Guide](./workflow.md)** - Phase 2.5 AI matching workflow
- **[LiveDeployer Architecture](./live-deployer/architecture.md)** - Dual-pipeline system
- **[Project Architecture](../../ARCHITECTURE.md)** - Overall component relationships
- **[Module READMEs](../../../modules/)** - Component-specific documentation

---

**Next:** See [workflow.md](./workflow.md) for the complete user workflow including AI parameter matching
