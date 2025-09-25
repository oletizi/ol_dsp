#!/usr/bin/env npx tsx
/**
 * Plugin Spec Generator for Canonical MIDI Maps
 *
 * Generates comprehensive JSON plugin descriptors for all installed plugins
 * using the enhanced plughost executable.
 *
 * Usage:
 *   pnpm generate:plugin-specs              # Generate all plugin specs
 *   pnpm generate:plugin-specs --format AudioUnit  # Only AudioUnit plugins
 *   pnpm generate:plugin-specs --quick       # Skip slow/problematic plugins
 */

import { join, dirname } from 'path';
import { mkdirSync, existsSync } from 'fs';
import { fileURLToPath } from 'url';
import { createPluginSpecGeneratorComponents } from './plugin-generator/index.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Path to plughost relative to this module
const PLUGHOST_PATH = join(__dirname, '../../../../../../cmake-build/modules/juce/host/plughost_artefacts/plughost');
const OUTPUT_DIR = join(__dirname, '../../plugin-descriptors');


/**
 * Interface for the main plugin specification generator orchestrator
 */
export interface IPluginSpecGeneratorOrchestrator {
  run(): Promise<void>;
}

/**
 * Plugin specification generator orchestrator configuration
 */
export interface PluginSpecGeneratorOrchestratorConfig {
  plughostPath: string;
  outputDir: string;
  args?: any;
}

/**
 * Main plugin specification generator orchestrator
 */
export class PluginSpecGeneratorOrchestrator implements IPluginSpecGeneratorOrchestrator {
  private readonly components;
  private readonly config;

  constructor(config: PluginSpecGeneratorOrchestratorConfig) {
    this.config = config;
    this.components = createPluginSpecGeneratorComponents({
      plughostPath: config.plughostPath,
      outputDir: config.outputDir,
      args: config.args
    });
  }





  async run(): Promise<void> {
    const { cli, args, pluginDiscovery, specGenerator } = this.components;

    if (args.help) {
      cli.showHelp(this.config.outputDir);
      return;
    }

    // Check plughost exists
    if (!existsSync(this.config.plughostPath)) {
      console.error(`❌ plughost not found at: ${this.config.plughostPath}`);
      console.error('Please run "make all" from the project root first.');
      process.exit(1);
    }

    // Ensure output directory exists
    mkdirSync(this.config.outputDir, { recursive: true });

    console.log(`🚀 Starting plugin spec generation...`);
    console.log(`📁 Output directory: ${this.config.outputDir}`);

    const plugins = await pluginDiscovery.getPluginList();

    if (plugins.length === 0) {
      console.log('❌ No plugins found');
      return;
    }

    console.log(`📋 Found ${plugins.length} plugins to process\n`);

    let successCount = 0;
    let skippedCount = 0;

    for (let i = 0; i < plugins.length; i++) {
      const plugin = plugins[i]!; // We know this exists since i < plugins.length
      console.log(`\n[${i + 1}/${plugins.length}] ${plugin.name} (${plugin.format})`);

      // Skip system plugins
      if (pluginDiscovery.shouldSkipPlugin(plugin)) {
        console.log(`  ⏭️  Skipping system plugin`);
        skippedCount++;
        continue;
      }

      // Progress tracking
      const startTime = Date.now();

      try {
        const spec = await specGenerator.generatePluginSpec(plugin);

        if (spec) {
          const filepath = specGenerator.savePluginSpec(plugin, spec, this.config.outputDir);
          const elapsed = ((Date.now() - startTime) / 1000).toFixed(1);
          console.log(`  ✅ Saved: ${filepath.replace(this.config.outputDir, '.')} (${elapsed}s)`);
          successCount++;
        } else {
          console.log(`  ⚠️  Skipped (no parameters or failed)`);
          skippedCount++;
        }
      } catch (error) {
        const elapsed = ((Date.now() - startTime) / 1000).toFixed(1);
        console.log(`  ❌ Error: ${error instanceof Error ? error.message : error} (${elapsed}s)`);
        skippedCount++;
      }

      // Show progress summary every 10 plugins
      if ((i + 1) % 10 === 0) {
        console.log(`\n📊 Progress: ${i + 1}/${plugins.length} processed, ${successCount} specs generated, ${skippedCount} skipped`);
      }
    }

    console.log(`\n🎉 Generation complete!`);
    console.log(`✅ Successfully generated: ${successCount} specs`);
    console.log(`⏭️  Skipped: ${skippedCount} plugins`);
    console.log(`📁 Output directory: ${this.config.outputDir}`);
  }
}

/**
 * Factory function to create plugin spec generator orchestrator
 */
export function createPluginSpecGeneratorOrchestrator(
  config: PluginSpecGeneratorOrchestratorConfig
): IPluginSpecGeneratorOrchestrator {
  return new PluginSpecGeneratorOrchestrator(config);
}

// ES module equivalent of require.main === module
if (import.meta.url === `file://${process.argv[1]}`) {
  const generator = createPluginSpecGeneratorOrchestrator({
    plughostPath: PLUGHOST_PATH,
    outputDir: OUTPUT_DIR
  });

  generator.run().catch(error => {
    console.error('❌ Fatal error:', error instanceof Error ? error.message : error);
    process.exit(1);
  });
}