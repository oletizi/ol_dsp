#!/usr/bin/env node

/**
 * CLI tool for validating Ardour MIDI map XML files.
 * Validates XML structure, MIDI parameters, and Ardour compatibility.
 *
 * Features:
 * - XML syntax and structure validation
 * - MIDI parameter range validation (channels 1-16, CC 0-127)
 * - Ardour binding function validation
 * - Device info validation
 * - Batch validation of multiple files
 * - Detailed error reporting with line numbers
 *
 * @example
 * ```bash
 * # Validate single XML file
 * validate-ardour-maps my-controller.xml
 *
 * # Validate all XML files in directory
 * validate-ardour-maps ardour-maps/
 *
 * # Strict validation with detailed output
 * validate-ardour-maps --strict --verbose maps/
 * ```
 */

import { readFileSync, statSync, readdirSync } from 'fs';
import { join, extname, basename } from 'path';
import { ArdourXMLSerializer } from '@/serializers/xml-serializer.js';
import type { ArdourMidiMap, ArdourBinding } from '@/types/ardour.js';

interface ValidateOptions {
  strict: boolean;
  verbose: boolean;
  help: boolean;
  quiet: boolean;
  checkFunctions: boolean;
}

interface ValidationStats {
  total: number;
  passed: number;
  failed: number;
  warnings: number;
}

interface ValidationResult {
  valid: boolean;
  errors: Array<{ message: string; line?: number; type: string }>;
  warnings: Array<{ message: string; line?: number; type: string }>;
  map?: ArdourMidiMap;
}

class ArdourMapValidator {
  private stats: ValidationStats = {
    total: 0,
    passed: 0,
    failed: 0,
    warnings: 0,
  };

  // Known Ardour functions for validation
  private readonly knownFunctions = new Set([
    'transport-start',
    'transport-stop',
    'transport-toggle-roll',
    'transport-record-enable',
    'transport-goto-start',
    'transport-goto-end',
    'transport-loop-toggle',
    'transport-play-selection',
    'rec-enable',
    'rec-disable',
    'track-set-gain',
    'track-set-solo',
    'track-set-mute',
    'track-set-send-gain',
    'track-set-return-gain',
    'track-set-pan-azimuth',
    'track-set-monitor-disk',
    'track-set-monitor-input',
    'track-set-monitor-auto',
    'master-set-gain',
    'master-set-solo',
    'master-set-mute',
    'select-track',
    'select-prev-track',
    'select-next-track',
    'bank-up',
    'bank-down',
    'channel-left',
    'channel-right',
    'zoom-in',
    'zoom-out',
    'zoom-to-session',
    'zoom-to-selection',
    'temporal-zoom-in',
    'temporal-zoom-out',
  ]);

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): { paths: string[]; options: ValidateOptions } {
    const options: ValidateOptions = {
      strict: false,
      verbose: false,
      help: false,
      quiet: false,
      checkFunctions: true,
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
      } else if (arg === '--no-function-check') {
        options.checkFunctions = false;
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
    console.log(`Ardour MIDI Map Validator

Usage:
  validate-ardour-maps [options] <file|directory>...

Arguments:
  <file|directory>     One or more XML files or directories to validate

Options:
  -h, --help           Show this help message
  -s, --strict         Treat warnings as errors
  -v, --verbose        Show detailed validation information
  -q, --quiet          Only show errors (suppress success messages)
  --no-function-check  Skip Ardour function name validation

Examples:
  validate-ardour-maps controller.xml                    # Validate single file
  validate-ardour-maps ardour-maps/                     # Validate all XML files in directory
  validate-ardour-maps --strict --verbose controller.xml # Strict validation with details
  validate-ardour-maps --no-function-check maps/        # Skip function validation

Exit Codes:
  0    All files valid
  1    Validation errors found
  2    Invalid command line arguments`);
  }

  /**
   * Validate XML structure and extract bindings
   */
  private validateXMLStructure(content: string, filePath: string): ValidationResult {
    const result: ValidationResult = {
      valid: true,
      errors: [],
      warnings: [],
    };

    try {
      // Basic XML structure checks
      const lines = content.split('\n');
      let lineNumber = 0;

      // Check for XML declaration
      if (!content.trim().startsWith('<?xml')) {
        result.warnings.push({
          message: 'Missing XML declaration',
          line: 1,
          type: 'XML_STRUCTURE',
        });
      }

      // Check for root element
      const rootMatch = content.match(/<ArdourMIDIBindings[^>]*>/i);
      if (!rootMatch) {
        result.errors.push({
          message: 'Missing or invalid ArdourMIDIBindings root element',
          type: 'XML_STRUCTURE',
        });
        result.valid = false;
        return result;
      }

      // Check for required attributes
      const rootElement = rootMatch[0];
      if (!rootElement.includes('name=')) {
        result.errors.push({
          message: 'ArdourMIDIBindings element missing required "name" attribute',
          type: 'XML_STRUCTURE',
        });
        result.valid = false;
      }

      // Try to parse with serializer
      const serializer = new ArdourXMLSerializer();
      try {
        const map = serializer.parseMidiMap(content);
        result.map = map;
      } catch (error) {
        result.errors.push({
          message: `XML parsing failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
          type: 'XML_PARSING',
        });
        result.valid = false;
      }

      return result;
    } catch (error) {
      result.errors.push({
        message: `Validation error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        type: 'VALIDATION_ERROR',
      });
      result.valid = false;
      return result;
    }
  }

  /**
   * Validate MIDI parameters in bindings
   */
  private validateMidiBindings(map: ArdourMidiMap): Array<{ message: string; type: string }> {
    const issues: Array<{ message: string; type: string }> = [];

    // Track CC usage for duplicate detection
    const ccUsage = new Map<string, string[]>();

    for (const binding of map.bindings) {
      // Validate MIDI channel
      if (binding.channel < 1 || binding.channel > 16) {
        issues.push({
          message: `Invalid MIDI channel ${binding.channel} (must be 1-16)`,
          type: 'MIDI_CHANNEL',
        });
      }

      // Validate CC numbers
      if (binding.ctl !== undefined) {
        if (binding.ctl < 0 || binding.ctl > 127) {
          issues.push({
            message: `Invalid MIDI CC ${binding.ctl} (must be 0-127)`,
            type: 'MIDI_CC',
          });
        } else {
          // Track CC usage
          const key = `ch${binding.channel}-cc${binding.ctl}`;
          if (!ccUsage.has(key)) {
            ccUsage.set(key, []);
          }
          const bindingId = binding.function || binding.action || 'unknown';
          ccUsage.get(key)!.push(bindingId);
        }
      }

      // Validate note numbers
      if (binding.note !== undefined) {
        if (binding.note < 0 || binding.note > 127) {
          issues.push({
            message: `Invalid MIDI note ${binding.note} (must be 0-127)`,
            type: 'MIDI_NOTE',
          });
        }
      }

      // Validate RPN/NRPN values
      if (binding.rpn !== undefined) {
        if (binding.rpn < 0 || binding.rpn > 16383) {
          issues.push({
            message: `Invalid RPN value ${binding.rpn} (must be 0-16383)`,
            type: 'MIDI_RPN',
          });
        }
      }

      if (binding.nrpn !== undefined) {
        if (binding.nrpn < 0 || binding.nrpn > 16383) {
          issues.push({
            message: `Invalid NRPN value ${binding.nrpn} (must be 0-16383)`,
            type: 'MIDI_NRPN',
          });
        }
      }

      // Validate threshold
      if (binding.threshold !== undefined) {
        if (binding.threshold < 0 || binding.threshold > 127) {
          issues.push({
            message: `Invalid threshold ${binding.threshold} (must be 0-127)`,
            type: 'MIDI_THRESHOLD',
          });
        }
      }

      // Check for required action/function/uri
      if (!binding.function && !binding.action && !binding.uri) {
        issues.push({
          message: 'Binding must have either function, action, or uri attribute',
          type: 'BINDING_TARGET',
        });
      }
    }

    // Check for duplicate CC assignments
    for (const [key, bindings] of ccUsage) {
      if (bindings.length > 1) {
        issues.push({
          message: `Duplicate CC assignment ${key}: ${bindings.join(', ')}`,
          type: 'DUPLICATE_CC',
        });
      }
    }

    return issues;
  }

  /**
   * Validate Ardour function names
   */
  private validateArdourFunctions(map: ArdourMidiMap): Array<{ message: string; type: string }> {
    const issues: Array<{ message: string; type: string }> = [];

    for (const binding of map.bindings) {
      if (binding.function) {
        // Extract base function name (remove track numbers, etc.)
        const baseFunction = binding.function.replace(/\[[^\]]+\]/g, '');

        if (!this.knownFunctions.has(baseFunction)) {
          issues.push({
            message: `Unknown Ardour function: ${binding.function}`,
            type: 'ARDOUR_FUNCTION',
          });
        }
      }
    }

    return issues;
  }

  /**
   * Validate device info
   */
  private validateDeviceInfo(map: ArdourMidiMap): Array<{ message: string; type: string }> {
    const issues: Array<{ message: string; type: string }> = [];

    if (map.deviceInfo) {
      const info = map.deviceInfo['device-info'];

      // Validate bank size
      if (info['bank-size'] !== undefined) {
        if (typeof info['bank-size'] !== 'number' || info['bank-size'] <= 0) {
          issues.push({
            message: `Invalid bank-size: ${info['bank-size']} (must be positive number)`,
            type: 'DEVICE_INFO',
          });
        }
      }

      // Validate threshold
      if (info.threshold !== undefined) {
        if (typeof info.threshold !== 'number' || info.threshold < 0 || info.threshold > 127) {
          issues.push({
            message: `Invalid device threshold: ${info.threshold} (must be 0-127)`,
            type: 'DEVICE_INFO',
          });
        }
      }
    }

    return issues;
  }

  /**
   * Validate a single Ardour MIDI map file
   */
  private validateFile(filePath: string, options: ValidateOptions): boolean {
    this.stats.total++;

    try {
      const content = readFileSync(filePath, 'utf8');
      const ext = extname(filePath).toLowerCase();

      if (ext !== '.xml') {
        if (!options.quiet) {
          console.error(`❌ ${filePath}: Not an XML file (expected .xml extension)`);
        }
        this.stats.failed++;
        return false;
      }

      // Validate XML structure
      const result = this.validateXMLStructure(content, filePath);

      if (!result.map) {
        this.stats.failed++;
        if (!options.quiet) {
          console.error(`❌ ${filePath}: Failed to parse XML`);
          if (options.verbose) {
            for (const error of result.errors) {
              console.error(`  Error: ${error.message}`);
            }
          }
        }
        return false;
      }

      // Validate MIDI bindings
      const midiIssues = this.validateMidiBindings(result.map);
      result.errors.push(...midiIssues.filter(i => ['MIDI_CHANNEL', 'MIDI_CC', 'MIDI_NOTE', 'MIDI_RPN', 'MIDI_NRPN', 'MIDI_THRESHOLD', 'BINDING_TARGET'].includes(i.type)));
      result.warnings.push(...midiIssues.filter(i => i.type === 'DUPLICATE_CC'));

      // Validate Ardour functions if enabled
      if (options.checkFunctions) {
        const functionIssues = this.validateArdourFunctions(result.map);
        result.warnings.push(...functionIssues);
      }

      // Validate device info
      const deviceIssues = this.validateDeviceInfo(result.map);
      result.errors.push(...deviceIssues);

      // Determine if validation passed
      const hasErrors = result.errors.length > 0;
      const hasWarnings = result.warnings.length > 0;
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
        for (const error of result.errors) {
          const location = error.line ? ` (line ${error.line})` : '';
          console.error(`    • ${error.message}${location}`);
        }
      }

      if (hasWarnings && (options.verbose || shouldFail)) {
        console.warn('  Warnings:');
        for (const warning of result.warnings) {
          const location = warning.line ? ` (line ${warning.line})` : '';
          console.warn(`    • ${warning.message}${location}`);
        }
      }

      if (options.verbose && result.map) {
        console.log(`  Map: ${result.map.name}`);
        console.log(`  Bindings: ${result.map.bindings.length}`);
        if (result.map.version) {
          console.log(`  Version: ${result.map.version}`);
        }
        if (result.map.deviceInfo) {
          console.log(`  Device: ${result.map.deviceInfo['device-name']}`);
        }
      }

      this.stats.warnings += result.warnings.length;

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
   * Validate all XML files in a directory
   */
  private validateDirectory(dirPath: string, options: ValidateOptions): boolean {
    try {
      const entries = readdirSync(dirPath);
      const xmlFiles = entries.filter(entry => {
        const ext = extname(entry).toLowerCase();
        return ext === '.xml';
      });

      if (xmlFiles.length === 0) {
        if (!options.quiet) {
          console.warn(`⚠️  No XML files found in ${dirPath}`);
        }
        return true;
      }

      if (!options.quiet && options.verbose) {
        console.log(`📁 Validating ${xmlFiles.length} XML files in ${dirPath}`);
      }

      let allValid = true;
      for (const file of xmlFiles) {
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
  const validator = new ArdourMapValidator();
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

export { ArdourMapValidator, main };