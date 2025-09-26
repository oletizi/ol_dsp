/**
 * CLI argument parsing and help display for plugin spec generator
 */

import type { GeneratorArgs } from './types.js';

/**
 * Interface for CLI argument parsing
 */
export interface ICli {
  parseArgs(args?: string[]): GeneratorArgs;
  showHelp(outputDir: string): void;
}

/**
 * Default CLI implementation
 */
export class Cli implements ICli {
  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): GeneratorArgs {
    const result: GeneratorArgs = {
      format: undefined,
      quick: false,
      help: false
    };

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];
      if (arg === '--help' || arg === '-h') {
        result.help = true;
      } else if (arg === '--quick') {
        result.quick = true;
      } else if (arg === '--format' && i + 1 < args.length) {
        result.format = args[i + 1];
        i++; // Skip next arg
      }
    }

    return result;
  }

  /**
   * Display help information
   */
  showHelp(outputDir: string): void {
    console.log(`Plugin Spec Generator for Canonical MIDI Maps

Usage:
  generate-plugin-specs.ts [options]

Options:
  --help, -h          Show this help message
  --format <format>   Only generate specs for specific format (AudioUnit, VST3, VST)
  --quick            Skip slow/problematic plugins for faster generation

Examples:
  generate-plugin-specs.ts                    # Generate all plugin specs
  generate-plugin-specs.ts --format AudioUnit # Only AudioUnit plugins
  generate-plugin-specs.ts --quick           # Fast generation, skip problematic plugins

Output:
  Plugin descriptors are saved to: ${outputDir}
  File naming: {manufacturer}_{plugin_name}.json`);
  }
}

/**
 * Factory function to create CLI
 */
export function createCli(): ICli {
  return new Cli();
}