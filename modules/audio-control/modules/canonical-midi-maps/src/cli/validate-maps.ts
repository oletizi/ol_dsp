#!/usr/bin/env node

/**
 * CLI tool for validating canonical MIDI map files.
 * Supports YAML and JSON formats with detailed error reporting.
 *
 * Features:
 * - Single file validation
 * - Batch validation of directories
 * - Detailed error reporting with line numbers
 * - Warning detection for best practices
 * - Exit codes for CI/CD integration
 *
 * @example
 * ```bash
 * # Validate single file
 * validate-maps my-controller.yaml
 *
 * # Validate all maps in directory
 * validate-maps maps/
 *
 * # Strict validation (warnings as errors)
 * validate-maps --strict maps/
 * ```
 */

import { readFileSync, statSync, readdirSync } from 'fs';
import { join, extname, basename } from 'path';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';

interface ValidateOptions {
  strict: boolean;
  verbose: boolean;
  help: boolean;
  quiet: boolean;
}

interface ValidationStats {
  total: number;
  passed: number;
  failed: number;
  warnings: number;
}

class MapValidator {
  private stats: ValidationStats = {
    total: 0,
    passed: 0,
    failed: 0,
    warnings: 0,
  };

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): { paths: string[]; options: ValidateOptions } {
    const options: ValidateOptions = {
      strict: false,
      verbose: false,
      help: false,
      quiet: false,
    };
    const paths: string[] = [];

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];

      if (arg === '--help' || arg === '-h') {
        options.help = true;
      } else if (arg === '--strict' || arg === '-s') {
        options.strict = true;
      } else if (arg === '--verbose' || arg === '-v') {
        options.verbose = true;
      } else if (arg === '--quiet' || arg === '-q') {
        options.quiet = true;
      } else if (!arg.startsWith('-')) {
        paths.push(arg);
      } else {
        console.error(`Unknown option: ${arg}`);
        process.exit(1);
      }
    }

    return { paths, options };
  }

  /**
   * Display help information
   */
  showHelp(): void {
    console.log(`Canonical MIDI Map Validator

Usage:
  validate-maps [options] <file|directory>...

Arguments:
  <file|directory>     One or more YAML/JSON files or directories to validate

Options:
  -h, --help          Show this help message
  -s, --strict        Treat warnings as errors
  -v, --verbose       Show detailed validation information
  -q, --quiet         Only show errors (suppress success messages)

Examples:
  validate-maps controller.yaml                    # Validate single file
  validate-maps maps/                             # Validate all maps in directory
  validate-maps --strict controller.yaml         # Strict validation
  validate-maps --verbose maps/ custom.json      # Verbose output for multiple inputs

Exit Codes:
  0    All files valid
  1    Validation errors found
  2    Invalid command line arguments`);
  }

  /**
   * Validate a single MIDI map file
   */
  private validateFile(filePath: string, options: ValidateOptions): boolean {
    this.stats.total++;

    try {
      const content = readFileSync(filePath, 'utf8');
      const ext = extname(filePath).toLowerCase();

      let result;
      if (ext === '.yaml' || ext === '.yml') {
        result = CanonicalMapParser.parseFromYAML(content);
      } else if (ext === '.json') {
        result = CanonicalMapParser.parseFromJSON(content);
      } else {
        if (!options.quiet) {
          console.error(`❌ ${filePath}: Unsupported file type (use .yaml, .yml, or .json)`);
        }
        this.stats.failed++;
        return false;
      }

      const hasErrors = !result.validation.valid;
      const hasWarnings = result.validation.warnings.length > 0;
      const shouldFail = hasErrors || (options.strict && hasWarnings);

      if (shouldFail) {
        this.stats.failed++;
        if (!options.quiet) {
          console.error(`❌ ${filePath}: Validation failed`);
        }
      } else {
        this.stats.passed++;
        if (!options.quiet) {
          console.log(`✅ ${filePath}: Valid${hasWarnings ? ' (with warnings)' : ''}`);
        }
      }

      if (hasErrors && options.verbose) {
        console.error('  Errors:');
        for (const error of result.validation.errors) {
          console.error(`    • ${error.path}: ${error.message}`);
        }
      }

      if (hasWarnings) {
        this.stats.warnings += result.validation.warnings.length;
        if (options.verbose || shouldFail) {
          console.warn('  Warnings:');
          for (const warning of result.validation.warnings) {
            console.warn(`    • ${warning.path}: ${warning.message}`);
          }
        }
      }

      if (options.verbose && result.map) {
        console.log(`  Device: ${result.map.device.manufacturer} ${result.map.device.model}`);
        console.log(`  Controls: ${result.map.controls.length}`);
        if (result.map.plugin) {
          console.log(`  Plugin: ${result.map.plugin.manufacturer} ${result.map.plugin.name}`);
        }
      }

      return !shouldFail;
    } catch (error) {
      this.stats.failed++;
      if (!options.quiet) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        console.error(`❌ ${filePath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Validate all files in a directory
   */
  private validateDirectory(dirPath: string, options: ValidateOptions): boolean {
    try {
      const entries = readdirSync(dirPath);
      const mapFiles = entries.filter(entry => {
        const ext = extname(entry).toLowerCase();
        return ['.yaml', '.yml', '.json'].includes(ext);
      });

      if (mapFiles.length === 0) {
        if (!options.quiet) {
          console.warn(`⚠️  No MIDI map files found in ${dirPath}`);
        }
        return true;
      }

      if (!options.quiet && options.verbose) {
        console.log(`📁 Validating ${mapFiles.length} files in ${dirPath}`);
      }

      let allValid = true;
      for (const file of mapFiles) {
        const filePath = join(dirPath, file);
        const isValid = this.validateFile(filePath, options);
        allValid = allValid && isValid;
      }

      return allValid;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Unknown error';
      if (!options.quiet) {
        console.error(`❌ Error reading directory ${dirPath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Main validation function
   */
  async validate(paths: string[], options: ValidateOptions): Promise<boolean> {
    if (paths.length === 0) {
      console.error('Error: No files or directories specified');
      this.showHelp();
      return false;
    }

    let allValid = true;

    for (const path of paths) {
      try {
        const stat = statSync(path);

        if (stat.isDirectory()) {
          const isValid = this.validateDirectory(path, options);
          allValid = allValid && isValid;
        } else if (stat.isFile()) {
          const isValid = this.validateFile(path, options);
          allValid = allValid && isValid;
        } else {
          if (!options.quiet) {
            console.error(`❌ ${path}: Not a file or directory`);
          }
          allValid = false;
        }
      } catch (error) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        if (!options.quiet) {
          console.error(`❌ ${path}: ${message}`);
        }
        allValid = false;
      }
    }

    // Show summary
    if (!options.quiet && this.stats.total > 1) {
      console.log('\n📊 Validation Summary:');
      console.log(`  Total files: ${this.stats.total}`);
      console.log(`  ✅ Passed: ${this.stats.passed}`);
      console.log(`  ❌ Failed: ${this.stats.failed}`);
      if (this.stats.warnings > 0) {
        console.log(`  ⚠️  Warnings: ${this.stats.warnings}`);
      }
    }

    return allValid;
  }
}

/**
 * Main CLI entry point
 */
async function main(): Promise<void> {
  const validator = new MapValidator();
  const { paths, options } = validator.parseArgs();

  if (options.help) {
    validator.showHelp();
    process.exit(0);
  }

  try {
    const isValid = await validator.validate(paths, options);
    process.exit(isValid ? 0 : 1);
  } catch (error) {
    const message = error instanceof Error ? error.message : 'Unknown error';
    console.error(`Fatal error: ${message}`);
    process.exit(2);
  }
}

// Run CLI if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  await main();
}

export { MapValidator, main };