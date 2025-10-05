# Controller Workflow

Universal MIDI controller configuration → DAW deployment workflow framework.

## Overview

The `controller-workflow` module provides a generalized framework for:

1. **Interrogating** MIDI controllers for configuration data
2. **Converting** controller-specific formats to canonical MIDI mappings
3. **Deploying** to multiple DAWs (Ardour, Ableton Live, etc.)

This module implements the **adapter pattern** to support any MIDI controller with programmable modes, providing a unified API for controller configuration management and DAW deployment.

## Features

- **Universal Controller Support**: Pluggable adapter system for any MIDI controller with custom modes
- **Canonical Format**: DAW-agnostic YAML/JSON representation of MIDI mappings
- **Multi-DAW Deployment**: Convert once, deploy to multiple DAWs
- **CLI & Programmatic API**: Use from command line or integrate into your tools
- **Type-Safe**: Full TypeScript support with strict mode enabled
- **Event-Driven**: Progress events for monitoring long-running operations

## Quick Start

### Installation

```bash
pnpm add @oletizi/controller-workflow
```

### CLI Usage

```bash
# List configuration slots on connected controller
controller-deploy list

# Deploy slot 0 to Ardour
controller-deploy deploy --slot 0 --daw ardour

# Deploy with plugin name and auto-install
controller-deploy deploy --slot 2 --daw ardour --plugin "TAL-Filter" --install

# Preview deployment without writing files
controller-deploy deploy --slot 0 --daw ardour live --dry-run
```

### Programmatic Usage

```typescript
import { DeploymentWorkflow } from '@oletizi/controller-workflow';

// Auto-detect connected controller and deploy
const workflow = await DeploymentWorkflow.create({
  targets: ['ardour']
});

// Listen for progress events
workflow.on('progress', ({ step, message }) => {
  console.log(`Step ${step}: ${message}`);
});

// Execute workflow
const result = await workflow.execute({
  configSlot: 0,
  targets: ['ardour'],
  preserveLabels: true,
  autoInstall: true,
  outputDir: './output'
});

if (result.success) {
  console.log('Deployment successful!');
  console.log('Canonical YAML:', result.canonicalPath);
  console.log('Deployments:', result.deployments);
}

// Clean up
await workflow.cleanup();
```

## Architecture

The module is built around three core interfaces that work together:

### 1. ControllerAdapterInterface

Wraps controller-specific libraries to provide uniform access to:
- Controller connection and device information
- Configuration slot management
- Reading/writing controller configurations

**Example**: `LaunchControlXL3Adapter` implements this for the Launch Control XL3

### 2. CanonicalConverterInterface

Converts controller-specific configuration formats to the canonical MIDI map format:
- Controller configuration → canonical MIDI map
- Validation and metadata generation
- Control ID mapping and label preservation

**Example**: `LaunchControlXL3Converter` implements this for LCXL3 configs

### 3. DAWDeployerInterface

Deploys canonical MIDI maps to DAW-specific formats:
- Canonical map → DAW format conversion
- Installation to DAW configuration directories
- Multi-platform support

**Example**: `ArdourDeployer` implements this for Ardour DAW

### DeploymentWorkflow Orchestrator

The `DeploymentWorkflow` class orchestrates the complete workflow:

```
┌──────────────┐     ┌────────────────────┐     ┌──────────────┐
│  Controller  │────▶│ Canonical MIDI Map │────▶│  DAW Config  │
│   Hardware   │     │   (YAML/JSON)      │     │   (Ardour)   │
└──────────────┘     └────────────────────┘     └──────────────┘
       ▲                       ▲                        ▲
       │                       │                        │
  Controller            Canonical                  DAW
   Adapter              Converter                Deployer
```

## Supported Controllers

| Controller | Status | Adapter | Converter |
|-----------|--------|---------|-----------|
| Launch Control XL3 | ✅ Complete | `LaunchControlXL3Adapter` | `LaunchControlXL3Converter` |
| Behringer X-Touch | 🔜 Planned | - | - |
| Akai APC series | 🔜 Planned | - | - |

## Supported DAWs

| DAW | Status | Deployer | Format |
|-----|--------|----------|--------|
| Ardour 8.0+ | ✅ Complete | `ArdourDeployer` | `.map` XML |
| Ableton Live 11.0+ | 🔜 Planned | - | Control Surface Script |

## CLI Reference

### `controller-deploy list`

List all configuration slots on the connected controller.

```bash
controller-deploy list
```

**Output:**
```
🎛️  Controller: Novation Launch Control XL 3

Configuration Slots:
──────────────────────────────────────────────────
  ● Slot 00: TAL-Filter
  ● Slot 01: Dexed
  ○ Slot 02: (empty)
  ● Slot 03: My Custom Setup
  ...
──────────────────────────────────────────────────

Total slots: 16
```

### `controller-deploy deploy`

Deploy controller configuration to DAW formats.

**Options:**

- `-c, --controller <type>` - Controller type (auto-detect if omitted)
- `-s, --slot <number>` - Configuration slot number (default: 0)
- `-d, --daw <daws...>` - Target DAWs (default: ['ardour'])
- `-p, --plugin <name>` - Plugin name for parameter mapping
- `-m, --midi-channel <number>` - MIDI channel override (0-15)
- `-o, --output <dir>` - Output directory (default: './output')
- `--install` - Auto-install to DAW config directories
- `--dry-run` - Preview deployment without writing files

**Examples:**

```bash
# Basic deployment
controller-deploy deploy --slot 0 --daw ardour

# With plugin name and auto-install
controller-deploy deploy -s 2 -d ardour -p "TAL-Filter" --install

# Multiple DAWs
controller-deploy deploy -s 0 -d ardour live --install

# Custom MIDI channel and output directory
controller-deploy deploy -s 1 -d ardour -m 5 -o ./my-maps

# Dry run to preview
controller-deploy deploy -s 0 -d ardour live --dry-run
```

## Extending the Framework

### Adding a New Controller

See [docs/ADDING_CONTROLLERS.md](./docs/ADDING_CONTROLLERS.md) for a complete guide.

**Quick overview:**

1. Implement `ControllerAdapterInterface` for your controller
2. Implement `CanonicalConverterInterface` for format conversion
3. Register in `DeploymentWorkflow.detectController()` method

### Adding a New DAW

1. Implement `DAWDeployerInterface` for your DAW
2. Handle platform-specific configuration paths
3. Implement canonical → DAW format conversion
4. Add to deployer registry

## Development

```bash
# Install dependencies
pnpm install

# Build
pnpm build

# Watch mode
pnpm dev

# Type checking
pnpm typecheck

# Tests
pnpm test
```

## Documentation

- [Architecture Guide](./docs/ARCHITECTURE.md) - System design and component overview
- [Adding Controllers](./docs/ADDING_CONTROLLERS.md) - Step-by-step guide for controller integration
- [API Reference](./docs/API.md) - Complete API documentation

## Examples

- [basic-deployment.ts](./examples/basic-deployment.ts) - Simple deployment example
- [custom-adapter-example.ts](./examples/custom-adapter-example.ts) - Creating a custom controller adapter
- [batch-deployment.ts](./examples/batch-deployment.ts) - Deploying multiple slots to multiple DAWs

## License

Apache-2.0

## Related Modules

- [@oletizi/launch-control-xl3](../launch-control-xl3) - Launch Control XL3 device library
- [@oletizi/canonical-midi-maps](../canonical-midi-maps) - Canonical MIDI map format
- [@oletizi/ardour-midi-maps](../ardour-midi-maps) - Ardour MIDI map utilities
