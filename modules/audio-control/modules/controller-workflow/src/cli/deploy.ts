#!/usr/bin/env node
/**
 * Universal MIDI Controller → DAW Deployment CLI
 *
 * Provides command-line interface for reading controller configurations
 * and deploying them to DAW-specific formats.
 *
 * @module controller-workflow/cli
 */

import { Command } from 'commander';
import * as fs from 'fs/promises';
import * as path from 'path';
import { stringify as yamlStringify } from 'yaml';
import { LaunchControlXL3Adapter } from '../adapters/controllers/LaunchControlXL3Adapter.js';
import { LaunchControlXL3Converter } from '../converters/LaunchControlXL3Converter.js';
import { ArdourDeployer } from '../adapters/daws/ArdourDeployer.js';
import type { ControllerAdapterInterface } from '../types/controller-adapter.js';
import type { DAWDeployerInterface } from '../types/daw-deployer.js';
import type { CanonicalMidiMap, PluginDefinition } from '@oletizi/canonical-midi-maps';

/**
 * CLI Progress Events
 */
interface ProgressEvent {
  step: number;
  total: number;
  message: string;
}

/**
 * Deployment Result
 */
interface DeploymentSummary {
  success: boolean;
  controller: string;
  slot: number;
  canonicalPath: string;
  dawPaths: Record<string, string>;
  errors: string[];
}

/**
 * Main CLI Program
 */
const program = new Command();

program
  .name('controller-deploy')
  .description('Universal MIDI controller configuration to DAW deployment tool')
  .version('1.0.0');

/**
 * List Command - Display controller configuration slots
 */
program
  .command('list')
  .description('List all configuration slots on the connected controller')
  .action(async () => {
    try {
      const controller = await detectController();
      await controller.connect();

      console.log(`\n🎛️  Controller: ${controller.manufacturer} ${controller.model}\n`);

      const slots = await controller.listConfigurations();

      console.log('Configuration Slots:');
      console.log('──────────────────────────────────────────────────');

      for (const slot of slots) {
        const status = slot.isEmpty ? '(empty)' : slot.name || '(unnamed)';
        const icon = slot.isEmpty ? '○' : '●';
        console.log(`  ${icon} Slot ${slot.index.toString().padStart(2, '0')}: ${status}`);
      }

      console.log('──────────────────────────────────────────────────');
      console.log(`\nTotal slots: ${slots.length}\n`);

      await controller.disconnect();
      process.exit(0);
    } catch (error: unknown) {
      handleError(error);
    }
  });

/**
 * Deploy Command - Read controller config and deploy to DAWs
 */
program
  .command('deploy')
  .description('Deploy controller configuration to DAW formats')
  .option('-c, --controller <type>', 'Controller type (auto-detect if omitted)')
  .option('-s, --slot <number>', 'Configuration slot number', '0')
  .option('-d, --daw <daws...>', 'Target DAWs (e.g., ardour, live)', ['ardour'])
  .option('-p, --plugin <name>', 'Plugin name for parameter mapping')
  .option('-m, --midi-channel <number>', 'MIDI channel override (0-15)')
  .option('-o, --output <dir>', 'Output directory for canonical YAML', './output')
  .option('--install', 'Auto-install to DAW config directories', false)
  .option('--dry-run', 'Preview deployment without writing files', false)
  .action(async (options) => {
    try {
      const result = await executeDeployment(options);

      if (result.success) {
        displaySuccessSummary(result);
        process.exit(0);
      } else {
        displayErrorSummary(result);
        process.exit(1);
      }
    } catch (error: unknown) {
      handleError(error);
    }
  });

/**
 * Execute full deployment workflow
 */
async function executeDeployment(options: {
  controller?: string;
  slot: string;
  daw: string[];
  plugin?: string;
  midiChannel?: string;
  output: string;
  install: boolean;
  dryRun: boolean;
}): Promise<DeploymentSummary> {
  const slotNumber = parseInt(options.slot, 10);
  const midiChannel = options.midiChannel ? parseInt(options.midiChannel, 10) : undefined;

  if (isNaN(slotNumber) || slotNumber < 0 || slotNumber > 15) {
    throw new Error('Slot number must be between 0 and 15');
  }

  if (midiChannel !== undefined && (isNaN(midiChannel) || midiChannel < 0 || midiChannel > 15)) {
    throw new Error('MIDI channel must be between 0 and 15');
  }

  console.log('\n🎛️  Controller Workflow - DAW Deployment\n');

  // Step 1: Read controller configuration
  emitProgress({ step: 1, total: 3, message: 'Reading controller configuration...' });

  const controller = await detectController();
  await controller.connect();

  const deviceInfo = await controller.getDeviceInfo();
  const config = await controller.readConfiguration(slotNumber);

  console.log(`     ✓ Controller: ${deviceInfo.manufacturer} ${deviceInfo.model}`);
  console.log(`     ✓ Configuration: "${config.name}" from slot ${slotNumber}\n`);

  // Step 2: Convert to canonical format
  emitProgress({ step: 2, total: 3, message: 'Converting to canonical format...' });

  const converter = new LaunchControlXL3Converter();

  // Build plugin info if provided
  let pluginInfo: PluginDefinition | undefined;
  if (options.plugin) {
    pluginInfo = {
      manufacturer: 'Unknown',
      name: options.plugin,
    };
  }

  const canonicalMap: CanonicalMidiMap = converter.convert(config, {
    ...(pluginInfo ? { pluginInfo } : {}),
    ...(midiChannel !== undefined ? { midiChannel } : {}),
    preserveLabels: true,
  });

  // Write canonical YAML
  let canonicalPath = '';
  if (!options.dryRun) {
    await fs.mkdir(options.output, { recursive: true });
    const canonicalFilename = `${sanitizeFilename(config.name)}.yaml`;
    canonicalPath = path.join(options.output, canonicalFilename);

    const yamlContent = yamlStringify(canonicalMap);
    await fs.writeFile(canonicalPath, yamlContent, 'utf-8');

    console.log(`     ✓ Saved canonical: ${canonicalPath}\n`);
  } else {
    canonicalPath = path.join(options.output, `${sanitizeFilename(config.name)}.yaml`);
    console.log(`     ✓ Canonical format generated (dry-run - not saved)\n`);
  }

  // Step 3: Deploy to DAWs
  emitProgress({ step: 3, total: 3, message: 'Deploying to DAWs...' });

  const dawPaths: Record<string, string> = {};
  const errors: string[] = [];

  for (const daw of options.daw) {
    try {
      const deployer = createDeployer(daw);
      const dawPath = await deployer.deploy(canonicalMap, {
        autoInstall: options.install,
        outputPath: path.join(options.output, `${sanitizeFilename(config.name)}.${getFileExtension(daw)}`),
        dryRun: options.dryRun,
      });

      if (dawPath.success && dawPath.outputPath) {
        dawPaths[daw] = dawPath.outputPath;
        console.log(`     ✓ ${capitalize(daw)}: ${dawPath.outputPath}`);
      } else {
        const deployErrors = dawPath.errors?.join(', ') || 'Unknown error';
        errors.push(`${daw}: ${deployErrors}`);
        console.log(`     ✗ ${capitalize(daw)}: ${deployErrors}`);
      }
    } catch (error: unknown) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      errors.push(`${daw}: ${errorMessage}`);
      console.log(`     ✗ ${capitalize(daw)}: ${errorMessage}`);
    }
  }

  await controller.disconnect();

  return {
    success: errors.length === 0,
    controller: `${deviceInfo.manufacturer} ${deviceInfo.model}`,
    slot: slotNumber,
    canonicalPath,
    dawPaths,
    errors,
  };
}

/**
 * Create DAW deployer based on DAW type
 */
function createDeployer(daw: string): DAWDeployerInterface {
  const normalizedDAW = daw.toLowerCase();

  switch (normalizedDAW) {
    case 'ardour':
      return ArdourDeployer.create();

    case 'live':
    case 'ableton':
      throw new Error('Ableton Live deployment not yet implemented');

    default:
      throw new Error(`Unsupported DAW: ${daw}`);
  }
}

/**
 * Get file extension for DAW format
 */
function getFileExtension(daw: string): string {
  const normalizedDAW = daw.toLowerCase();

  switch (normalizedDAW) {
    case 'ardour':
      return 'map';
    case 'live':
    case 'ableton':
      return 'amxd';
    default:
      return 'txt';
  }
}

/**
 * Auto-detect connected MIDI controller
 * Currently supports Launch Control XL3
 */
async function detectController(): Promise<ControllerAdapterInterface> {
  // Attempt to detect LCXL3
  try {
    const adapter = await LaunchControlXL3Adapter.create();
    return adapter;
  } catch (error: unknown) {
    throw new Error(
      'No supported controller detected. Ensure your controller is connected via USB.\n' +
      'Supported controllers: Novation Launch Control XL 3'
    );
  }
}

/**
 * Emit progress message
 */
function emitProgress(event: ProgressEvent): void {
  console.log(`[${event.step}/${event.total}] ${event.message}`);
}

/**
 * Display successful deployment summary
 */
function displaySuccessSummary(result: DeploymentSummary): void {
  console.log('\n✅ Deployment complete!\n');

  console.log(`  Canonical YAML: ${result.canonicalPath}`);

  if (Object.keys(result.dawPaths).length > 0) {
    console.log('\n  DAW Formats:');
    for (const [daw, dawPath] of Object.entries(result.dawPaths)) {
      console.log(`    ${capitalize(daw)}: ${dawPath}`);
    }
  }

  console.log('');
}

/**
 * Display error summary
 */
function displayErrorSummary(result: DeploymentSummary): void {
  console.error('\n❌ Deployment failed\n');

  if (result.errors.length > 0) {
    console.error('  Errors:');
    for (const error of result.errors) {
      console.error(`    - ${error}`);
    }
    console.error('');
  }
}

/**
 * Handle uncaught errors
 */
function handleError(error: unknown): void {
  const message = error instanceof Error ? error.message : String(error);
  console.error(`\n❌ Error: ${message}\n`);
  process.exit(1);
}

/**
 * Sanitize filename for filesystem safety
 */
function sanitizeFilename(name: string): string {
  return name
    .replace(/[^a-z0-9_-]/gi, '_')
    .replace(/_+/g, '_')
    .toLowerCase();
}

/**
 * Capitalize first letter of string
 */
function capitalize(str: string): string {
  return str.charAt(0).toUpperCase() + str.slice(1);
}

// Parse and execute CLI
program.parse();
