# Controller Workflow Module - Generalized One-Click Deployment

**Issue:** #360
**Version:** 1.21
**Status:** Planning → Implementation
**Created:** 2025-10-05

## Related Documentation

- **[360 Overview](../README.md)** - Feature 360 master navigation
- **[Architecture](../architecture.md)** - Overall 360 architecture
- **[Workflow Guide](../workflow.md)** - User workflows and CLI usage
- **[Implementation Status](../status.md)** - Current progress tracking
- **[LiveDeployer Architecture](../live-deployer/architecture.md)** - Dual-pipeline mapping system

## Overview

Create a new **`controller-workflow`** module that provides a generalized framework for:
1. **Interrogating** any MIDI controller for configuration data
2. **Converting** controller-specific formats → canonical MIDI mappings
3. **Deploying** to multiple DAWs (Ardour, Ableton Live, etc.)

**First Implementation:** Launch Control XL3
**Future Support:** Any MIDI controller with programmable modes

---

## Module Structure

```
modules/audio-control/
└── modules/
    ├── controller-workflow/              # NEW - Generalized framework
    │   ├── src/
    │   │   ├── core/
    │   │   │   ├── ControllerAdapter.ts        # Abstract controller interface
    │   │   │   ├── CanonicalConverter.ts       # Abstract converter interface
    │   │   │   └── DAWDeployer.ts              # Abstract DAW interface
    │   │   │
    │   │   ├── adapters/
    │   │   │   ├── controllers/
    │   │   │   │   └── LaunchControlXL3Adapter.ts  # LCXL3 implementation
    │   │   │   └── daws/
    │   │   │       ├── ArdourDeployer.ts
    │   │   │       └── LiveDeployer.ts
    │   │   │
    │   │   ├── converters/
    │   │   │   └── LaunchControlXL3Converter.ts    # LCXL3-specific converter
    │   │   │
    │   │   ├── services/
    │   │   │   └── ParameterMatcher.ts             # AI-powered parameter matching
    │   │   │
    │   │   ├── orchestrator/
    │   │   │   └── DeploymentWorkflow.ts           # Generic orchestrator
    │   │   │
    │   │   ├── cli/
    │   │   │   └── deploy.ts                       # CLI with controller detection
    │   │   │
    │   │   └── index.ts
    │   │
    │   ├── package.json
    │   ├── tsconfig.json
    │   └── README.md
    │
    ├── launch-control-xl3/          # EXISTING - LCXL3 device library
    ├── canonical-midi-maps/         # EXISTING - Canonical format
    ├── ardour-midi-maps/            # EXISTING - Ardour deployment
    └── live-max-cc-router/          # EXISTING - Live deployment
```

---

## Phase 1: Core Abstraction Layer

### 1.1 Controller Adapter Interface

**Location:** `modules/controller-workflow/src/core/ControllerAdapter.ts`

```typescript
/**
 * Abstract interface for controller interrogation
 * Each controller type implements this interface
 */
export interface ControllerAdapterInterface {
  /** Controller metadata */
  readonly manufacturer: string;
  readonly model: string;
  readonly capabilities: ControllerCapabilities;

  /** Connection management */
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  isConnected(): boolean;

  /** Configuration retrieval */
  listConfigurations(): Promise<ConfigurationSlot[]>;
  readConfiguration(slot: number): Promise<ControllerConfiguration>;
  writeConfiguration?(slot: number, config: ControllerConfiguration): Promise<void>;

  /** Device information */
  getDeviceInfo(): Promise<DeviceInfo>;
}

export interface ControllerCapabilities {
  supportsCustomModes: boolean;
  maxConfigSlots: number;
  supportsRead: boolean;
  supportsWrite: boolean;
  supportedControlTypes: ControlType[];
}

export interface ControllerConfiguration {
  name: string;
  controls: ControlMapping[];
  metadata?: Record<string, any>;
}

export interface ConfigurationSlot {
  index: number;
  name?: string;
  isEmpty: boolean;
}

export interface ControlMapping {
  id: string;
  name?: string;
  type: ControlType;
  cc?: number;
  channel?: number;
  range?: [number, number];
}

export type ControlType = 'encoder' | 'slider' | 'button' | 'button_group';

export interface DeviceInfo {
  manufacturer: string;
  model: string;
  firmwareVersion?: string;
}
```

### 1.2 Canonical Converter Interface

**Location:** `modules/controller-workflow/src/core/CanonicalConverter.ts`

```typescript
/**
 * Abstract interface for converting controller configs → canonical format
 * Each controller type implements its own converter
 */
export interface CanonicalConverterInterface {
  /** Convert controller-specific config to canonical map */
  convert(
    config: ControllerConfiguration,
    options: ConversionOptions
  ): CanonicalMidiMap;

  /** Validate conversion is possible */
  canConvert(config: ControllerConfiguration): boolean;

  /** Get converter metadata */
  getConverterInfo(): ConverterInfo;
}

export interface ConversionOptions {
  pluginInfo?: PluginDefinition;
  midiChannel?: number;
  preserveLabels?: boolean;
  deviceOverrides?: Partial<DeviceDefinition>;
}

export interface ConverterInfo {
  supportedController: string;
  version: string;
  features: string[];
}
```

### 1.3 DAW Deployer Interface

**Location:** `modules/controller-workflow/src/core/DAWDeployer.ts`

```typescript
/**
 * Abstract interface for deploying to DAWs
 * Each DAW implements this interface
 */
export interface DAWDeployerInterface {
  readonly dawName: string;
  readonly version: string;

  /** Deploy canonical map to DAW */
  deploy(
    canonicalMap: CanonicalMidiMap,
    options: DeploymentOptions
  ): Promise<DeploymentResult>;

  /** Check if DAW is installed */
  isInstalled(): Promise<boolean>;

  /** Get DAW configuration directory */
  getConfigDirectory(): Promise<string>;
}

export interface DeploymentOptions {
  autoInstall?: boolean;
  outputPath?: string;
  dryRun?: boolean;
}

export interface DeploymentResult {
  success: boolean;
  dawName?: string;
  outputPath?: string;
  installed?: boolean;
  errors?: string[];
}
```

**Acceptance Criteria - Phase 1:**
- ✅ All interfaces defined with TypeScript strict mode
- ✅ Clear separation of concerns (controller, conversion, deployment)
- ✅ Factory methods specified where appropriate
- ✅ Interface-first design pattern enforced

---

## Phase 2: Launch Control XL3 Adapter (Reference Implementation)

**Location:** `modules/controller-workflow/src/adapters/controllers/LaunchControlXL3Adapter.ts`

```typescript
import { LaunchControlXL3 } from '@oletizi/launch-control-xl3';
import type { ControllerAdapterInterface } from '../../core/ControllerAdapter.js';

export class LaunchControlXL3Adapter implements ControllerAdapterInterface {
  readonly manufacturer = 'Novation';
  readonly model = 'Launch Control XL 3';
  readonly capabilities = {
    supportsCustomModes: true,
    maxConfigSlots: 16,
    supportsRead: true,
    supportsWrite: true,
    supportedControlTypes: ['encoder', 'slider', 'button'] as ControlType[]
  };

  constructor(private device: LaunchControlXL3) {}

  static async create(): Promise<LaunchControlXL3Adapter> {
    const device = new LaunchControlXL3({ enableCustomModes: true });
    await device.connect();
    return new LaunchControlXL3Adapter(device);
  }

  async connect(): Promise<void> {
    await this.device.connect();
  }

  async disconnect(): Promise<void> {
    await this.device.disconnect();
  }

  isConnected(): boolean {
    return this.device.isConnected();
  }

  async listConfigurations(): Promise<ConfigurationSlot[]> {
    const slots: ConfigurationSlot[] = [];
    for (let i = 0; i < 16; i++) {
      const mode = await this.device.readCustomMode(i);
      slots.push({
        index: i,
        name: mode?.name,
        isEmpty: mode === null
      });
    }
    return slots;
  }

  async readConfiguration(slot: number): Promise<ControllerConfiguration> {
    const customMode = await this.device.readCustomMode(slot);
    if (!customMode) {
      throw new Error(`No configuration in slot ${slot}`);
    }

    // Convert LCXL3 CustomMode → generic ControllerConfiguration
    return {
      name: customMode.name,
      controls: this.convertControls(customMode.controls),
      metadata: customMode.metadata
    };
  }

  async writeConfiguration(slot: number, config: ControllerConfiguration): Promise<void> {
    // Convert generic ControllerConfiguration → LCXL3 CustomMode
    const customMode = this.convertToCustomMode(config);
    await this.device.writeCustomMode(slot, customMode);
  }

  async getDeviceInfo(): Promise<DeviceInfo> {
    const info = await this.device.verifyDevice();
    return {
      manufacturer: 'Novation',
      model: 'Launch Control XL 3',
      firmwareVersion: info.firmwareVersion
    };
  }

  private convertControls(controls: any): ControlMapping[] {
    const mappings: ControlMapping[] = [];
    for (const [key, control] of Object.entries(controls)) {
      mappings.push({
        id: key,
        name: (control as any).name,
        type: (control as any).type,
        cc: (control as any).ccNumber,
        channel: (control as any).midiChannel,
        range: [(control as any).minValue, (control as any).maxValue]
      });
    }
    return mappings;
  }

  private convertToCustomMode(config: ControllerConfiguration): any {
    // Implementation details
    throw new Error('Not yet implemented');
  }
}
```

**Acceptance Criteria - Phase 2:**
- ✅ Implements ControllerAdapterInterface
- ✅ Wraps existing LaunchControlXL3 library
- ✅ Provides generic ControllerConfiguration format
- ✅ Supports all 16 custom mode slots
- ✅ Factory method for instantiation

---

## Phase 3: Launch Control XL3 Converter

**Location:** `modules/controller-workflow/src/converters/LaunchControlXL3Converter.ts`

```typescript
export class LaunchControlXL3Converter implements CanonicalConverterInterface {

  getConverterInfo(): ConverterInfo {
    return {
      supportedController: 'Novation Launch Control XL 3',
      version: '1.0.0',
      features: ['custom-modes', 'label-preservation', 'all-control-types']
    };
  }

  canConvert(config: ControllerConfiguration): boolean {
    // Validate config has required LCXL3 structure
    return config.controls.length > 0 &&
           config.controls.every(c => c.cc !== undefined);
  }

  convert(
    config: ControllerConfiguration,
    options: ConversionOptions
  ): CanonicalMidiMap {
    return {
      version: '1.0.0',
      device: {
        manufacturer: 'Novation',
        model: 'Launch Control XL 3',
        ...(options.deviceOverrides || {})
      },
      metadata: {
        name: config.name,
        description: `Converted from LCXL3 custom mode: ${config.name}`,
        date: new Date().toISOString().split('T')[0],
        tags: ['launch-control-xl3', 'auto-generated']
      },
      plugin: options.pluginInfo,
      midi_channel: options.midiChannel,
      controls: this.mapControls(config.controls, options)
    };
  }

  private mapControls(controls: ControlMapping[], options: ConversionOptions): ControlDefinition[] {
    return controls.map(control => ({
      id: this.mapControlId(control.id),        // SEND_A1 → encoder_1
      name: options.preserveLabels ? control.name : this.getDefaultName(control.id),
      type: this.mapControlType(control.type),
      cc: control.cc!,
      channel: control.channel ?? options.midiChannel ?? 0,
      range: control.range || [0, 127],
      description: `Mapped from ${control.id}`
    }));
  }

  private mapControlId(lcxl3Id: string): string {
    // LCXL3-specific mapping: SEND_A1 → encoder_1, FADER1 → slider_1, etc.
    const mapping: Record<string, string> = {
      'SEND_A1': 'encoder_1', 'SEND_A2': 'encoder_2', 'SEND_A3': 'encoder_3',
      'SEND_A4': 'encoder_4', 'SEND_A5': 'encoder_5', 'SEND_A6': 'encoder_6',
      'SEND_A7': 'encoder_7', 'SEND_A8': 'encoder_8',
      'SEND_B1': 'encoder_9', 'SEND_B2': 'encoder_10', 'SEND_B3': 'encoder_11',
      'SEND_B4': 'encoder_12', 'SEND_B5': 'encoder_13', 'SEND_B6': 'encoder_14',
      'SEND_B7': 'encoder_15', 'SEND_B8': 'encoder_16',
      'PAN1': 'encoder_17', 'PAN2': 'encoder_18', 'PAN3': 'encoder_19',
      'PAN4': 'encoder_20', 'PAN5': 'encoder_21', 'PAN6': 'encoder_22',
      'PAN7': 'encoder_23', 'PAN8': 'encoder_24',
      'FADER1': 'slider_1', 'FADER2': 'slider_2', 'FADER3': 'slider_3',
      'FADER4': 'slider_4', 'FADER5': 'slider_5', 'FADER6': 'slider_6',
      'FADER7': 'slider_7', 'FADER8': 'slider_8',
      'FOCUS1': 'button_1', 'FOCUS2': 'button_2', 'FOCUS3': 'button_3',
      'FOCUS4': 'button_4', 'FOCUS5': 'button_5', 'FOCUS6': 'button_6',
      'FOCUS7': 'button_7', 'FOCUS8': 'button_8',
      'CONTROL1': 'button_9', 'CONTROL2': 'button_10', 'CONTROL3': 'button_11',
      'CONTROL4': 'button_12', 'CONTROL5': 'button_13', 'CONTROL6': 'button_14',
      'CONTROL7': 'button_15', 'CONTROL8': 'button_16'
    };
    return mapping[lcxl3Id] || lcxl3Id.toLowerCase();
  }

  private mapControlType(type: ControlType): 'encoder' | 'slider' | 'button' | 'button_group' {
    return type;
  }

  private getDefaultName(controlId: string): string {
    return controlId.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
  }
}
```

**Acceptance Criteria - Phase 3:**
- ✅ Implements CanonicalConverterInterface
- ✅ Converts all 48 LCXL3 controls
- ✅ Preserves labels when requested
- ✅ Generates valid CanonicalMidiMap
- ✅ Correct control ID mapping (device → canonical)

---

## Phase 4: Generalized Deployment Workflow

**Location:** `modules/controller-workflow/src/orchestrator/DeploymentWorkflow.ts`

```typescript
import { EventEmitter } from 'eventemitter3';

export interface WorkflowOptions {
  configSlot: number;
  targets: string[];
  pluginInfo?: PluginDefinition;
  midiChannel?: number;
  preserveLabels?: boolean;
  outputDir?: string;
  autoInstall?: boolean;
  dryRun?: boolean;
}

export interface WorkflowResult {
  success: boolean;
  controllerConfig?: ControllerConfiguration;
  canonicalMap?: CanonicalMidiMap;
  deployments: DeploymentResult[];
  errors: string[];
}

export interface CreateOptions {
  controllerAdapter?: ControllerAdapterInterface;
  targets: string[];
}

export class DeploymentWorkflow extends EventEmitter {
  constructor(
    private controllerAdapter: ControllerAdapterInterface,
    private converter: CanonicalConverterInterface,
    private deployers: Map<string, DAWDeployerInterface>
  ) {
    super();
  }

  static async create(options: CreateOptions): Promise<DeploymentWorkflow> {
    // Auto-detect controller or use specified adapter
    const adapter = options.controllerAdapter || await this.detectController();

    // Get appropriate converter for detected controller
    const converter = this.getConverterFor(adapter);

    // Initialize DAW deployers
    const deployers = new Map<string, DAWDeployerInterface>();
    if (options.targets.includes('ardour')) {
      const { ArdourDeployer } = await import('../adapters/daws/ArdourDeployer.js');
      deployers.set('ardour', new ArdourDeployer());
    }
    if (options.targets.includes('live')) {
      const { LiveDeployer } = await import('../adapters/daws/LiveDeployer.js');
      deployers.set('live', new LiveDeployer());
    }

    return new DeploymentWorkflow(adapter, converter, deployers);
  }

  async execute(options: WorkflowOptions): Promise<WorkflowResult> {
    const errors: string[] = [];

    try {
      // Step 1: Read configuration from controller
      this.emit('progress', { step: 1, message: 'Reading controller configuration...' });
      const config = await this.controllerAdapter.readConfiguration(options.configSlot);

      // Step 2: Convert to canonical format
      this.emit('progress', { step: 2, message: 'Converting to canonical format...' });
      const canonicalMap = this.converter.convert(config, {
        pluginInfo: options.pluginInfo,
        midiChannel: options.midiChannel,
        preserveLabels: options.preserveLabels ?? true
      });

      // Step 3: Save canonical YAML (optional)
      if (options.outputDir) {
        await this.saveCanonical(canonicalMap, options.outputDir);
      }

      // Step 4: Deploy to each target DAW
      this.emit('progress', { step: 3, message: 'Deploying to DAWs...' });
      const deployments: DeploymentResult[] = [];

      for (const target of options.targets) {
        const deployer = this.deployers.get(target);
        if (deployer) {
          const result = await deployer.deploy(canonicalMap, {
            autoInstall: options.autoInstall,
            dryRun: options.dryRun
          });
          deployments.push(result);
          if (!result.success) {
            errors.push(...(result.errors || [`${target} deployment failed`]));
          }
        } else {
          errors.push(`No deployer available for ${target}`);
        }
      }

      return {
        success: deployments.every(d => d.success) && errors.length === 0,
        controllerConfig: config,
        canonicalMap,
        deployments,
        errors
      };

    } catch (error) {
      errors.push(error instanceof Error ? error.message : String(error));
      return {
        success: false,
        deployments: [],
        errors
      };
    }
  }

  getControllerAdapter(): ControllerAdapterInterface {
    return this.controllerAdapter;
  }

  private async saveCanonical(canonicalMap: CanonicalMidiMap, outputDir: string): Promise<void> {
    const yaml = await import('yaml');
    const { writeFileSync, mkdirSync } = await import('fs');
    const { join } = await import('path');

    mkdirSync(outputDir, { recursive: true });
    const filename = `${canonicalMap.device.model.toLowerCase().replace(/\s+/g, '-')}-${canonicalMap.metadata.name.toLowerCase().replace(/\s+/g, '-')}.yaml`;
    const filepath = join(outputDir, filename);

    writeFileSync(filepath, yaml.stringify(canonicalMap));
    this.emit('canonical-saved', { path: filepath });
  }

  private static async detectController(): Promise<ControllerAdapterInterface> {
    // Try to detect connected controller
    // Start with LCXL3, add more controllers later
    try {
      const { LaunchControlXL3Adapter } = await import('../adapters/controllers/LaunchControlXL3Adapter.js');
      return await LaunchControlXL3Adapter.create();
    } catch {
      throw new Error('No supported controller detected');
    }
  }

  private static getConverterFor(adapter: ControllerAdapterInterface): CanonicalConverterInterface {
    // Registry of converters by controller model
    if (adapter.model === 'Launch Control XL 3') {
      const { LaunchControlXL3Converter } = require('../converters/LaunchControlXL3Converter.js');
      return new LaunchControlXL3Converter();
    }
    throw new Error(`No converter available for ${adapter.model}`);
  }
}
```

**Acceptance Criteria - Phase 4:**
- ✅ Works with any ControllerAdapterInterface
- ✅ Auto-detects connected controller
- ✅ Supports multiple DAW targets simultaneously
- ✅ Event-based progress reporting
- ✅ Comprehensive error handling

---

## Phase 5: Universal CLI

**Location:** `modules/controller-workflow/src/cli/deploy.ts`

```typescript
#!/usr/bin/env node
import { Command } from 'commander';
import { DeploymentWorkflow } from '../orchestrator/DeploymentWorkflow.js';

const program = new Command();

program
  .name('controller-deploy')
  .description('Deploy MIDI controller configurations to DAWs')
  .version('1.0.0');

program
  .command('list')
  .description('List connected controllers and available configurations')
  .action(async () => {
    try {
      const workflow = await DeploymentWorkflow.create({ targets: [] });
      const adapter = workflow.getControllerAdapter();

      console.log(`\n📱 Detected: ${adapter.manufacturer} ${adapter.model}`);
      console.log(`\nAvailable configurations:`);

      const configs = await adapter.listConfigurations();
      configs.forEach(slot => {
        const status = slot.isEmpty ? '(empty)' : slot.name;
        console.log(`  Slot ${slot.index}: ${status}`);
      });
    } catch (error) {
      console.error(`\n❌ Error: ${error instanceof Error ? error.message : String(error)}`);
      process.exit(1);
    }
  });

program
  .command('deploy')
  .description('Deploy a controller configuration to DAW(s)')
  .option('-c, --controller <type>', 'Controller type (auto-detect if omitted)')
  .option('-s, --slot <number>', 'Configuration slot number', '0')
  .option('-d, --daw <daws...>', 'Target DAWs (ardour, live)', ['ardour'])
  .option('-p, --plugin <name>', 'Plugin name for AI-powered parameter matching')
  .option('-m, --midi-channel <number>', 'MIDI channel override')
  .option('-o, --output <dir>', 'Output directory for canonical YAML')
  .option('--install', 'Auto-install to DAW config directories', false)
  .option('--dry-run', 'Preview without deploying', false)
  .action(async (options) => {
    console.log('🎛️  Controller Workflow - DAW Deployment\n');

    try {
      const workflow = await DeploymentWorkflow.create({
        targets: options.daw
      });

      workflow.on('progress', ({ step, message }) => {
        console.log(`[${step}/3] ${message}`);
      });

      workflow.on('canonical-saved', ({ path }) => {
        console.log(`     ✓ Saved canonical: ${path}`);
      });

      const result = await workflow.execute({
        configSlot: parseInt(options.slot),
        targets: options.daw,
        pluginInfo: options.plugin ? { name: options.plugin } : undefined,
        midiChannel: options.midiChannel ? parseInt(options.midiChannel) : undefined,
        outputDir: options.output,
        autoInstall: options.install,
        dryRun: options.dryRun
      });

      if (result.success) {
        console.log('\n✅ Deployment complete!\n');
        result.deployments.forEach(d => {
          if (d.outputPath) {
            console.log(`  ${d.dawName}: ${d.outputPath}`);
          }
        });
      } else {
        console.error('\n❌ Deployment failed:\n');
        result.errors.forEach(e => console.error(`  - ${e}`));
        process.exit(1);
      }
    } catch (error) {
      console.error(`\n❌ Error: ${error instanceof Error ? error.message : String(error)}`);
      process.exit(1);
    }
  });

program.parse();
```

**Usage Examples:**
```bash
# List connected controllers
npx controller-deploy list

# Deploy Launch Control XL3 slot 0 to Ardour
npx controller-deploy deploy --slot 0 --daw ardour

# Deploy with AI-powered plugin parameter matching
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour live

# Auto-detect controller, save canonical, and deploy
npx controller-deploy deploy --slot 0 --output ./mappings --install
```

**Acceptance Criteria - Phase 5:**
- ✅ Works with any supported controller
- ✅ Auto-detects connected controller
- ✅ Lists available configurations
- ✅ Supports all workflow options
- ✅ Clear progress indicators and error messages

---

## Phase 6: DAW Deployers

### Ardour Deployer

**Location:** `modules/controller-workflow/src/adapters/daws/ArdourDeployer.ts`

```typescript
import { MidiMapBuilder, ArdourXMLSerializer } from '@oletizi/ardour-midi-maps';

export class ArdourDeployer implements DAWDeployerInterface {
  readonly dawName = 'Ardour';
  readonly version = '8.0';

  async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
    try {
      // Convert canonical → Ardour XML
      const ardourMap = this.convertToArdour(canonicalMap);
      const serializer = new ArdourXMLSerializer();
      const xml = serializer.serializeMidiMap(ardourMap);

      // Determine output path
      const outputPath = options.outputPath || await this.getDefaultOutputPath(canonicalMap);

      if (!options.dryRun) {
        const { writeFileSync, mkdirSync } = await import('fs');
        const { dirname } = await import('path');

        mkdirSync(dirname(outputPath), { recursive: true });
        writeFileSync(outputPath, xml);

        // Install to Ardour config if requested
        if (options.autoInstall) {
          const configDir = await this.getConfigDirectory();
          const { basename, join } = await import('path');
          const installPath = join(configDir, basename(outputPath));
          writeFileSync(installPath, xml);
          return {
            success: true,
            dawName: this.dawName,
            outputPath: installPath,
            installed: true
          };
        }
      }

      return {
        success: true,
        dawName: this.dawName,
        outputPath,
        installed: false
      };

    } catch (error) {
      return {
        success: false,
        dawName: this.dawName,
        errors: [error instanceof Error ? error.message : String(error)]
      };
    }
  }

  async isInstalled(): Promise<boolean> {
    const { existsSync } = await import('fs');
    try {
      const configDir = await this.getConfigDirectory();
      return existsSync(configDir);
    } catch {
      return false;
    }
  }

  async getConfigDirectory(): Promise<string> {
    const { homedir } = await import('os');
    const { join } = await import('path');
    const home = homedir();

    switch (process.platform) {
      case 'darwin':
        return join(home, 'Library', 'Preferences', 'Ardour8', 'midi_maps');
      case 'linux':
        return join(home, '.config', 'ardour8', 'midi_maps');
      case 'win32':
        return join(home, 'AppData', 'Local', 'Ardour8', 'midi_maps');
      default:
        throw new Error(`Unsupported platform: ${process.platform}`);
    }
  }

  private convertToArdour(canonicalMap: CanonicalMidiMap): any {
    // Use existing ardour-midi-maps conversion logic
    const builder = new MidiMapBuilder({
      name: `${canonicalMap.device.manufacturer} ${canonicalMap.device.model}`,
      version: canonicalMap.version
    });

    // Convert controls
    for (const control of canonicalMap.controls) {
      if (control.type === 'button') {
        builder.addNoteBinding({
          channel: control.channel as number,
          note: control.cc!,
          function: control.plugin_parameter ?
            `/route/plugin/parameter S1 1 ${control.plugin_parameter}` :
            'transport-stop'
        });
      } else {
        builder.addCCBinding({
          channel: control.channel as number,
          controller: control.cc!,
          function: control.plugin_parameter ?
            `/route/plugin/parameter S1 1 ${control.plugin_parameter}` :
            'track-set-gain[1]',
          encoder: control.type === 'encoder'
        });
      }
    }

    return builder.build();
  }

  private async getDefaultOutputPath(canonicalMap: CanonicalMidiMap): Promise<string> {
    const { join } = await import('path');
    const filename = `${canonicalMap.device.manufacturer.toLowerCase()}-${canonicalMap.device.model.toLowerCase().replace(/\s+/g, '-')}.map`;
    return join(process.cwd(), 'dist', 'ardour-maps', filename);
  }
}
```

### Live Deployer

**Location:** `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts`

```typescript
export class LiveDeployer implements DAWDeployerInterface {
  readonly dawName = 'Ableton Live';
  readonly version = '11.0';

  async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
    // For now, return placeholder - Live integration is more complex
    // Requires updating Max for Live device configuration
    return {
      success: true,
      dawName: this.dawName,
      outputPath: 'Live deployment not yet implemented',
      installed: false
    };
  }

  async isInstalled(): Promise<boolean> {
    // Check for Ableton Live installation
    return false;
  }

  async getConfigDirectory(): Promise<string> {
    throw new Error('Live config directory not yet implemented');
  }
}
```

**Acceptance Criteria - Phase 6:**
- ✅ Ardour deployer fully functional
- ✅ Generates valid Ardour XML
- ✅ Auto-installs to Ardour config directory
- ✅ Live deployer structure in place (implementation TBD)

---

## Phase 7: Testing & Documentation

### Unit Tests

**Test Files:**
- `src/adapters/controllers/LaunchControlXL3Adapter.test.ts`
- `src/converters/LaunchControlXL3Converter.test.ts`
- `src/orchestrator/DeploymentWorkflow.test.ts`
- `src/adapters/daws/ArdourDeployer.test.ts`

**Test Strategy:**
- Mock dependencies using interfaces
- Test each phase independently
- Integration tests for end-to-end workflow

### Integration Tests

**Test Scenarios:**
1. Mock LCXL3 device → canonical conversion
2. Canonical → Ardour XML generation
3. Full workflow with mock device
4. CLI argument parsing and execution

### Manual Testing Checklist

- [ ] Connect real Launch Control XL3
- [ ] List configurations from all slots
- [ ] Read custom mode and convert to canonical
- [ ] Deploy to local Ardour installation
- [ ] Verify mapping works in Ardour
- [ ] Test with multiple plugins
- [ ] Test batch deployment

### Documentation

**Create:**
- `README.md` - Getting started guide
- `docs/ARCHITECTURE.md` - System design & abstractions
- `docs/ADDING_CONTROLLERS.md` - Guide for new controllers
- `docs/API.md` - API reference
- `examples/` - Usage examples

**Acceptance Criteria - Phase 7:**
- ✅ 80%+ code coverage
- ✅ All integration tests pass
- ✅ Successfully deploys to real Ardour
- ✅ Complete documentation
- ✅ Usage examples for common workflows

---

## Phase 8: AI Parameter Matching Service

**Objective:** Implement Claude AI-powered fuzzy matching to automatically map hardware control names to plugin parameter indices.

**Location:** `modules/controller-workflow/src/services/ParameterMatcher.ts`

### Components

1. **ParameterMatcherInterface** - Core matching interface
2. **ParameterMatcher** - Implementation with Claude integration
3. **Plugin descriptor loader** - Load parameter databases
4. **Claude integration** - Dual method (API + CLI fallback)
5. **Confidence scoring** - Match quality metrics

### Implementation Requirements

- Load plugin descriptors from canonical-midi-maps registry
- Dual Claude integration: Anthropic API (primary), CLI (fallback)
- Semantic matching: exact, abbreviation, synonym recognition
- Confidence scoring: 0-1 scale with reasoning
- Error handling: graceful degradation if AI unavailable
- Caching: Save matches in canonical YAML for reuse
- Timeout handling: 30s default, configurable

### Integration Points

**Deploy workflow (deploy.ts):**
```typescript
if (options.plugin) {
  const matcher = ParameterMatcher.create();
  const descriptor = await loadPluginDescriptor(options.plugin);
  const matches = await matcher.matchParameters(controlNames, descriptor);

  // Add plugin_parameter fields to controls
  config.controls.forEach((control, i) => {
    if (matches[i]?.pluginParameter) {
      control.plugin_parameter = matches[i].pluginParameter;
    }
  });
}
```

### Testing Strategy

- Mock Claude responses for deterministic tests
- Test confidence thresholds (0.6 minimum)
- Test graceful degradation (API failure → CLI fallback)
- Test parameter validation
- Test caching behavior
- Integration test with real TAL-J-8 descriptor

### Acceptance Criteria - Phase 8

- [x] Implements ParameterMatcherInterface
- [x] Supports Anthropic API integration
- [x] Supports Claude CLI fallback
- [x] Confidence scoring (0-1) with reasoning
- [x] Plugin descriptor loading
- [x] Error handling with descriptive messages
- [x] Timeout handling (30s default)
- [x] Parameter index validation
- [x] Unit tests with mocked responses (16 tests)
- [x] Example usage code
- [x] Documentation (README.md in services/)
- [ ] Integration with deploy.ts workflow
- [ ] End-to-end test with real plugin descriptor
- [ ] Performance benchmarks (<5s for 48 controls)

---

## Package Configuration

### modules/controller-workflow/package.json

```json
{
  "name": "@oletizi/controller-workflow",
  "version": "1.0.0",
  "description": "Universal MIDI controller configuration → DAW deployment workflow",
  "type": "module",
  "main": "./dist/index.js",
  "types": "./dist/index.d.ts",
  "bin": {
    "controller-deploy": "./dist/cli/deploy.js"
  },
  "scripts": {
    "build": "tsc -b",
    "dev": "tsc -b --watch",
    "test": "vitest run",
    "test:coverage": "vitest --coverage",
    "lint": "eslint . --ext .ts",
    "typecheck": "tsc --noEmit",
    "cli": "node dist/cli/deploy.js"
  },
  "dependencies": {
    "@oletizi/launch-control-xl3": "workspace:*",
    "@oletizi/canonical-midi-maps": "workspace:*",
    "@oletizi/ardour-midi-maps": "workspace:*",
    "commander": "^11.0.0",
    "eventemitter3": "^5.0.1",
    "yaml": "^2.3.4"
  },
  "devDependencies": {
    "@types/node": "^20.11.0",
    "typescript": "^5.3.3",
    "vitest": "^1.2.0",
    "eslint": "^8.56.0"
  },
  "keywords": [
    "midi",
    "controller",
    "workflow",
    "daw",
    "ardour",
    "ableton",
    "automation",
    "launch-control-xl3"
  ],
  "license": "Apache-2.0",
  "engines": {
    "node": ">=18"
  }
}
```

### modules/controller-workflow/tsconfig.json

```json
{
  "extends": "../../tsconfig.base.json",
  "compilerOptions": {
    "outDir": "./dist",
    "rootDir": "./src",
    "composite": true,
    "declaration": true,
    "declarationMap": true,
    "sourceMap": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist", "**/*.test.ts"]
}
```

---

## Future Controller Support (Extensibility)

### Adding a New Controller

**Required Steps:**

1. **Create Controller Adapter**
   ```typescript
   // src/adapters/controllers/NewControllerAdapter.ts
   export class NewControllerAdapter implements ControllerAdapterInterface {
     // Implement all interface methods
   }
   ```

2. **Create Converter**
   ```typescript
   // src/converters/NewControllerConverter.ts
   export class NewControllerConverter implements CanonicalConverterInterface {
     // Implement conversion logic
   }
   ```

3. **Register in Workflow**
   ```typescript
   // Update DeploymentWorkflow.detectController()
   // Update DeploymentWorkflow.getConverterFor()
   ```

**Potential Future Controllers:**
- Behringer X-Touch / X32
- Novation Launchpad
- Akai APC series
- Native Instruments Maschine
- Any controller with SysEx configuration

---

## Development Timeline

| Phase | Tasks | Estimated Time | Status |
|-------|-------|----------------|--------|
| 1. Core Abstractions | Interfaces & base types | 2-3 hours | ⏳ Pending |
| 2. LCXL3 Adapter | Reference implementation | 3-4 hours | ⏳ Pending |
| 3. LCXL3 Converter | Canonical conversion | 3-4 hours | ⏳ Pending |
| 4. Workflow Orchestrator | Generic pipeline | 3-4 hours | ⏳ Pending |
| 5. CLI | Universal command interface | 2-3 hours | ⏳ Pending |
| 6. DAW Deployers | Ardour & Live adapters | 2-3 hours | ⏳ Pending |
| 7. Testing & Docs | Tests & documentation | 3-4 hours | ⏳ Pending |
| 8. AI Parameter Matching | Claude-powered matching | 4-6 hours | ⏳ Pending |
| **Total** | | **22-31 hours** | |

---

## Success Metrics

### 1. Generalization
- ✅ Works with any controller via adapter pattern
- ✅ Adding new controller = 2 new classes (adapter + converter)
- ✅ Zero changes to orchestrator/CLI for new controllers

### 2. Launch Control XL3 Support
- ✅ Full feature parity with requirements
- ✅ All 48 controls mapped correctly
- ✅ Deploys to Ardour and Live

### 3. Developer Experience
- ✅ Clear extension points for new controllers
- ✅ Interface-based design throughout
- ✅ Comprehensive documentation
- ✅ Type-safe TypeScript

### 4. User Experience
- ✅ Single command deploys custom mode
- ✅ < 10 seconds end-to-end execution
- ✅ Clear progress indicators
- ✅ Helpful error messages

### 5. AI Parameter Matching
- ✅ Automatic parameter mapping via Claude AI
- ✅ High confidence matching (>0.6 threshold)
- ✅ Graceful degradation when AI unavailable
- ✅ < 5 seconds for 48 controls

---

## Implementation Progress Tracking

### ✅ Completed
- [x] Analysis of existing components
- [x] Architecture design
- [x] Workplan documentation

### ⏳ In Progress
- [ ] Phase 1: Core Abstractions

### 📋 To Do
- [ ] Phase 2: LCXL3 Adapter
- [ ] Phase 3: LCXL3 Converter
- [ ] Phase 4: Workflow Orchestrator
- [ ] Phase 5: CLI
- [ ] Phase 6: DAW Deployers
- [ ] Phase 7: Testing & Documentation
- [ ] Phase 8: AI Parameter Matching

---

## Next Steps

1. ✅ **Workplan approved and documented**
2. Create `modules/controller-workflow/` directory structure
3. Initialize package.json and tsconfig.json
4. Implement Phase 1: Core interfaces
5. Implement Phase 2: LCXL3 adapter (reference)
6. Build incrementally through remaining phases
7. Test with real hardware
8. Release as `@oletizi/controller-workflow@1.0.0`

---

## References

### Feature 360 Documentation
- **[360 Overview](../README.md)** - Master navigation and quick reference
- **[Architecture](../architecture.md)** - System architecture and design patterns
- **[Workflow Guide](../workflow.md)** - Complete 3-phase workflow
- **[Implementation Status](../status.md)** - Current progress and tracking
- **[LiveDeployer Architecture](../live-deployer/architecture.md)** - Dual-pipeline mapping system
- **[Goal Document](../goal.md)** - Original feature vision and requirements

### Related Modules
- `@oletizi/launch-control-xl3` - Device library
- `@oletizi/canonical-midi-maps` - Canonical format
- `@oletizi/ardour-midi-maps` - Ardour deployment
- `@oletizi/live-max-cc-router` - Live integration

### Module Documentation
- [Launch Control XL3 README](../../../modules/launch-control-xl3/README.md)
- [Canonical MIDI Maps README](../../../modules/canonical-midi-maps/README.md)
- [Ardour MIDI Maps README](../../../modules/ardour-midi-maps/README.md)
- [Live Max CC Router README](../../../modules/live-max-cc-router/README.md)

### External Resources
- [Ardour MIDI Binding Documentation](https://manual.ardour.org/using-control-surfaces/generic-midi/)
- [Ableton Live Max for Live](https://www.ableton.com/en/live/max-for-live/)

---

**Last Updated:** 2025-10-12 (Phase 8: AI Parameter Matching added)
**Author:** Claude Code AI Assistant
**Status:** Ready for Implementation
