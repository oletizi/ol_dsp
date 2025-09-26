#!/usr/bin/env node

/**
 * CLI tool for converting canonical MIDI maps to Ardour XML format.
 * Bridges the gap between the canonical format and Ardour-specific XML.
 *
 * Features:
 * - Convert canonical YAML/JSON to Ardour XML
 * - Batch conversion of multiple files
 * - Function mapping configuration
 * - Device info generation
 * - Validation of both input and output
 * - Custom mapping rules and templates
 *
 * @example
 * ```bash
 * # Convert single file
 * convert-to-ardour controller.yaml
 *
 * # Convert with custom function mapping
 * convert-to-ardour --functions custom-functions.json controller.yaml
 *
 * # Batch convert directory
 * convert-to-ardour --output ardour-maps/ canonical-maps/
 * ```
 */

import { readFileSync, writeFileSync, statSync, readdirSync, mkdirSync } from 'fs';
import { join, dirname, basename, extname } from 'path';
import { CanonicalMapParser } from '../../../canonical-midi-maps/src/parsers/yaml-parser.js';
import { ArdourXMLSerializer } from '@/serializers/xml-serializer.js';
import type { CanonicalMidiMapOutput } from '../../../canonical-midi-maps/src/validators/schema.js';
import type { ArdourMidiMap, ArdourBinding, ArdourDeviceInfo } from '@/types/ardour.js';

interface ConvertOptions {
  output?: string;
  functions?: string;
  deviceInfo?: string;
  validate: boolean;
  overwrite: boolean;
  verbose: boolean;
  quiet: boolean;
  help: boolean;
  dryRun: boolean;
}

interface ConversionStats {
  total: number;
  converted: number;
  failed: number;
  skipped: number;
}

interface FunctionMapping {
  [controlId: string]: string | {
    function?: string;
    action?: string;
    uri?: string;
    encoder?: 'yes' | 'no';
    momentary?: 'yes' | 'no';
    threshold?: number;
  };
}

class CanonicalToArdourConverter {
  private stats: ConversionStats = {
    total: 0,
    converted: 0,
    failed: 0,
    skipped: 0,
  };

  private readonly defaultFunctionMappings: Record<string, string> = {
    // Volume controls
    'volume': 'master-set-gain',
    'master-volume': 'master-set-gain',
    'track-volume': 'track-set-gain[1]',
    'channel-volume': 'track-set-gain[1]',

    // Transport controls
    'play': 'transport-toggle-roll',
    'stop': 'transport-stop',
    'record': 'transport-record-enable',
    'rewind': 'transport-goto-start',
    'forward': 'transport-goto-end',
    'loop': 'transport-loop-toggle',

    // Track controls
    'solo': 'track-set-solo[1]',
    'mute': 'track-set-mute[1]',
    'pan': 'track-set-pan-azimuth[1]',
    'send': 'track-set-send-gain[1][1]',

    // Navigation
    'track-left': 'select-prev-track',
    'track-right': 'select-next-track',
    'bank-left': 'bank-down',
    'bank-right': 'bank-up',

    // Zoom
    'zoom-in': 'zoom-in',
    'zoom-out': 'zoom-out',
    'zoom-fit': 'zoom-to-session',
  };

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): { paths: string[]; options: ConvertOptions } {
    const options: ConvertOptions = {
      validate: true,
      overwrite: false,
      verbose: false,
      quiet: false,
      help: false,
      dryRun: false,
    };
    const paths: string[] = [];

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];

      if (arg === '--help' || arg === '-h') {
        options.help = true;
      } else if (arg === '--output' || arg === '-o') {
        if (i + 1 < args.length) {
          options.output = args[i + 1];
          i++;
        } else {
          console.error('--output requires a path argument');
          process.exit(1);
        }
      } else if (arg === '--functions' || arg === '-f') {
        if (i + 1 < args.length) {
          options.functions = args[i + 1];
          i++;
        } else {
          console.error('--functions requires a path argument');
          process.exit(1);
        }
      } else if (arg === '--device-info' || arg === '-d') {
        if (i + 1 < args.length) {
          options.deviceInfo = args[i + 1];
          i++;
        } else {
          console.error('--device-info requires a path argument');
          process.exit(1);
        }
      } else if (arg === '--no-validate') {
        options.validate = false;
      } else if (arg === '--overwrite') {
        options.overwrite = true;
      } else if (arg === '--verbose' || arg === '-v') {
        options.verbose = true;
      } else if (arg === '--quiet' || arg === '-q') {
        options.quiet = true;
      } else if (arg === '--dry-run') {
        options.dryRun = true;
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
    console.log(`Canonical MIDI Map to Ardour XML Converter

Usage:
  convert-to-ardour [options] <file|directory>...

Arguments:
  <file|directory>     One or more canonical MIDI map files or directories

Options:
  -h, --help           Show this help message
  -o, --output <dir>   Output directory for XML files (default: same as input)
  -f, --functions <file> JSON file with custom function mappings
  -d, --device-info <file> JSON file with custom device info templates
  --no-validate        Skip validation of input and output
  --overwrite          Overwrite existing files without prompt
  -v, --verbose        Show detailed conversion information
  -q, --quiet          Only show errors
  --dry-run            Show what would be converted without writing files

Function Mapping:
  The converter maps canonical control IDs to Ardour functions using:
  1. Custom function mappings (--functions file)
  2. Built-in default mappings
  3. Generic parameter mappings for unmapped controls

Examples:
  convert-to-ardour controller.yaml                        # Convert single file
  convert-to-ardour --output ardour/ canonical-maps/       # Convert directory
  convert-to-ardour --functions custom.json controller.yaml # With custom mappings
  convert-to-ardour --dry-run --verbose maps/              # Preview conversion

Output:
  - XML files compatible with Ardour DAW
  - Device information embedded in XML
  - MIDI bindings with proper Ardour function names

Exit Codes:
  0    All conversions successful
  1    Some conversions failed
  2    Invalid command line arguments`);
  }

  /**
   * Load custom function mappings from JSON file
   */
  private loadFunctionMappings(filePath: string): FunctionMapping {
    try {
      const content = readFileSync(filePath, 'utf8');
      return JSON.parse(content);
    } catch (error) {
      throw new Error(`Failed to load function mappings from ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Load custom device info templates from JSON file
   */
  private loadDeviceInfoTemplates(filePath: string): Record<string, ArdourDeviceInfo> {
    try {
      const content = readFileSync(filePath, 'utf8');
      return JSON.parse(content);
    } catch (error) {
      throw new Error(`Failed to load device info templates from ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Convert canonical control to Ardour binding
   */
  private convertControl(
    control: CanonicalMidiMapOutput['controls'][0],
    canonicalMap: CanonicalMidiMapOutput,
    functionMappings: FunctionMapping
  ): ArdourBinding[] {
    const bindings: ArdourBinding[] = [];

    // Determine MIDI channel
    let midiChannel: number;
    if (typeof control.channel === 'number') {
      midiChannel = control.channel;
    } else if (typeof control.channel === 'string') {
      // Handle channel registry reference (simplified - just use default)
      midiChannel = canonicalMap.midi_channel || 1;
    } else {
      midiChannel = canonicalMap.midi_channel || 1;
    }

    // Get function mapping
    const mapping = functionMappings[control.id] || this.defaultFunctionMappings[control.id];

    if (control.type === 'button_group' && control.buttons) {
      // Handle button groups
      for (const button of control.buttons) {
        const buttonMapping = functionMappings[button.id] || mapping;

        if (typeof buttonMapping === 'string') {
          bindings.push({
            channel: midiChannel,
            ctl: button.cc,
            function: buttonMapping,
            momentary: button.mode === 'momentary' ? 'yes' : 'no',
          });
        } else if (typeof buttonMapping === 'object') {
          bindings.push({
            channel: midiChannel,
            ctl: button.cc,
            ...(buttonMapping.function && { function: buttonMapping.function }),
            ...(buttonMapping.action && { action: buttonMapping.action }),
            ...(buttonMapping.uri && { uri: buttonMapping.uri }),
            ...(buttonMapping.encoder && { encoder: buttonMapping.encoder }),
            momentary: buttonMapping.momentary || (button.mode === 'momentary' ? 'yes' : 'no'),
            ...(buttonMapping.threshold !== undefined && { threshold: buttonMapping.threshold }),
          });
        }
      }
    } else if (control.cc !== undefined) {
      // Handle regular controls
      if (typeof mapping === 'string') {
        const binding: ArdourBinding = {
          channel: midiChannel,
          ctl: control.cc,
          function: mapping,
        };

        // Add control-specific attributes
        if (control.type === 'encoder') {
          binding.encoder = 'yes';
        }
        if (control.type === 'button') {
          binding.momentary = control.mode === 'momentary' ? 'yes' : 'no';
        }

        bindings.push(binding);
      } else if (typeof mapping === 'object') {
        bindings.push({
          channel: midiChannel,
          ctl: control.cc,
          ...(mapping.function && { function: mapping.function }),
          ...(mapping.action && { action: mapping.action }),
          ...(mapping.uri && { uri: mapping.uri }),
          ...(mapping.encoder && { encoder: mapping.encoder }),
          ...(mapping.momentary && { momentary: mapping.momentary }),
          ...(mapping.threshold !== undefined && { threshold: mapping.threshold }),
        });
      } else {
        // Generate generic mapping for unmapped controls
        const genericFunction = this.generateGenericFunction(control, canonicalMap);
        if (genericFunction) {
          bindings.push({
            channel: midiChannel,
            ctl: control.cc,
            function: genericFunction,
            ...(control.type === 'encoder' && { encoder: 'yes' }),
            ...(control.type === 'button' && { momentary: control.mode === 'momentary' ? 'yes' : 'no' }),
          });
        }
      }
    }

    return bindings;
  }

  /**
   * Generate generic Ardour function for unmapped controls
   */
  private generateGenericFunction(
    control: CanonicalMidiMapOutput['controls'][0],
    canonicalMap: CanonicalMidiMapOutput
  ): string | null {
    // If control has plugin parameter mapping, create generic parameter control
    if (control.plugin_parameter) {
      // This would ideally map to plugin parameter automation
      // For now, map to track controls based on parameter type
      const paramName = typeof control.plugin_parameter === 'string' ?
        control.plugin_parameter.toLowerCase() :
        `param-${control.plugin_parameter}`;

      if (paramName.includes('gain') || paramName.includes('volume')) {
        return 'track-set-gain[1]';
      } else if (paramName.includes('pan')) {
        return 'track-set-pan-azimuth[1]';
      } else if (paramName.includes('send')) {
        return 'track-set-send-gain[1][1]';
      } else {
        // Generic parameter control - this would need plugin-specific handling
        return `track-set-gain[1]`; // Fallback to gain control
      }
    }

    // Map by control type and name
    const controlName = control.name.toLowerCase();

    if (controlName.includes('volume') || controlName.includes('gain')) {
      return 'track-set-gain[1]';
    } else if (controlName.includes('pan')) {
      return 'track-set-pan-azimuth[1]';
    } else if (controlName.includes('solo')) {
      return 'track-set-solo[1]';
    } else if (controlName.includes('mute')) {
      return 'track-set-mute[1]';
    } else if (controlName.includes('send')) {
      return 'track-set-send-gain[1][1]';
    }

    // Default mapping based on control type
    switch (control.type) {
      case 'slider':
        return 'track-set-gain[1]';
      case 'encoder':
        return 'track-set-pan-azimuth[1]';
      case 'button':
        return 'track-set-mute[1]';
      default:
        return null;
    }
  }

  /**
   * Generate device info for Ardour map
   */
  private generateDeviceInfo(
    canonicalMap: CanonicalMidiMapOutput,
    deviceInfoTemplates?: Record<string, ArdourDeviceInfo>
  ): ArdourDeviceInfo | undefined {
    // Try to find matching template
    const deviceKey = `${canonicalMap.device.manufacturer}-${canonicalMap.device.model}`.toLowerCase().replace(/\s+/g, '-');

    if (deviceInfoTemplates && deviceInfoTemplates[deviceKey]) {
      return deviceInfoTemplates[deviceKey];
    }

    // Generate basic device info
    const hasEncoders = canonicalMap.controls.some(c => c.type === 'encoder');
    const hasSliders = canonicalMap.controls.some(c => c.type === 'slider');
    const hasButtons = canonicalMap.controls.some(c => c.type === 'button' || c.type === 'button_group');

    return {
      'device-name': `${canonicalMap.device.manufacturer} ${canonicalMap.device.model}`,
      'device-info': {
        'motorized': 'no',
        'has-lcd': 'no',
        'has-meters': 'no',
        'has-timecode': 'no',
        'has-master-fader': hasSliders ? 'yes' : 'no',
        'has-global-controls': hasButtons ? 'yes' : 'no',
        'uses-logic-control-buttons': 'no',
        'uses-mackie-control-buttons': 'no',
      },
    };
  }

  /**
   * Convert a single canonical MIDI map file to Ardour XML
   */
  private async convertFile(
    inputPath: string,
    options: ConvertOptions,
    functionMappings: FunctionMapping,
    deviceInfoTemplates?: Record<string, ArdourDeviceInfo>
  ): Promise<boolean> {
    this.stats.total++;

    try {
      // Read and parse canonical map
      const content = readFileSync(inputPath, 'utf8');
      const ext = extname(inputPath).toLowerCase();

      let parseResult;
      if (ext === '.yaml' || ext === '.yml') {
        parseResult = CanonicalMapParser.parseFromYAML(content);
      } else if (ext === '.json') {
        parseResult = CanonicalMapParser.parseFromJSON(content);
      } else {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Unsupported file format (use .yaml, .yml, or .json)`);
        }
        this.stats.failed++;
        return false;
      }

      // Validate canonical map
      if (options.validate && !parseResult.validation.valid) {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Canonical map validation failed`);
          if (options.verbose) {
            for (const error of parseResult.validation.errors) {
              console.error(`  • ${error.path}: ${error.message}`);
            }
          }
        }
        this.stats.failed++;
        return false;
      }

      if (!parseResult.map) {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Failed to parse canonical map`);
        }
        this.stats.failed++;
        return false;
      }

      const canonicalMap = parseResult.map;

      // Convert to Ardour format
      const ardourBindings: ArdourBinding[] = [];

      for (const control of canonicalMap.controls) {
        const bindings = this.convertControl(control, canonicalMap, functionMappings);
        ardourBindings.push(...bindings);
      }

      const ardourMap: ArdourMidiMap = {
        name: canonicalMap.metadata.name,
        version: canonicalMap.version,
        bindings: ardourBindings,
        deviceInfo: this.generateDeviceInfo(canonicalMap, deviceInfoTemplates),
      };

      // Generate XML
      const serializer = new ArdourXMLSerializer();
      const xmlContent = serializer.serializeMidiMap(ardourMap);

      // Validate generated XML if requested
      if (options.validate) {
        try {
          serializer.parseMidiMap(xmlContent); // Round-trip validation
        } catch (error) {
          if (!options.quiet) {
            console.error(`❌ ${inputPath}: Generated XML validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
          }
          this.stats.failed++;
          return false;
        }
      }

      // Determine output path
      const baseName = basename(inputPath, extname(inputPath));
      const outputPath = options.output ?
        join(options.output, `${baseName}.xml`) :
        join(dirname(inputPath), `${baseName}.xml`);

      if (options.dryRun) {
        if (!options.quiet) {
          console.log(`📋 ${inputPath} → ${outputPath} (dry run)`);
          if (options.verbose) {
            console.log(`  Device: ${ardourMap.deviceInfo?.['device-name'] || ardourMap.name}`);
            console.log(`  Bindings: ${ardourMap.bindings.length}`);
            console.log(`  Functions: ${new Set(ardourMap.bindings.map(b => b.function).filter(Boolean)).size} unique`);
          }
        }
        this.stats.converted++;
        return true;
      }

      // Check if output file exists
      const outputExists = (() => {
        try {
          statSync(outputPath);
          return true;
        } catch {
          return false;
        }
      })();

      if (outputExists && !options.overwrite) {
        if (!options.quiet) {
          console.warn(`⚠️  ${outputPath}: File exists, use --overwrite to replace`);
        }
        this.stats.skipped++;
        return true;
      }

      // Create output directory if needed
      const outputDir = dirname(outputPath);
      try {
        mkdirSync(outputDir, { recursive: true });
      } catch (error) {
        // Directory might already exist
      }

      // Write XML file
      writeFileSync(outputPath, xmlContent, 'utf8');

      if (!options.quiet) {
        console.log(`✅ ${basename(inputPath)} → ${basename(outputPath)}`);
      }

      if (options.verbose) {
        console.log(`  Device: ${ardourMap.deviceInfo?.['device-name'] || ardourMap.name}`);
        console.log(`  Bindings: ${ardourMap.bindings.length}`);
        console.log(`  Functions: ${new Set(ardourMap.bindings.map(b => b.function).filter(Boolean)).size} unique`);
        console.log(`  Controls converted: ${canonicalMap.controls.length}`);

        // Show warnings if any
        if (parseResult.validation.warnings.length > 0) {
          console.warn('  Input warnings:');
          for (const warning of parseResult.validation.warnings) {
            console.warn(`    • ${warning.path}: ${warning.message}`);
          }
        }
      }

      this.stats.converted++;
      return true;
    } catch (error) {
      this.stats.failed++;
      if (!options.quiet) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        console.error(`❌ ${inputPath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Convert all files in a directory
   */
  private async convertDirectory(
    dirPath: string,
    options: ConvertOptions,
    functionMappings: FunctionMapping,
    deviceInfoTemplates?: Record<string, ArdourDeviceInfo>
  ): Promise<boolean> {
    try {
      const entries = readdirSync(dirPath);
      const mapFiles = entries.filter(entry => {
        const ext = extname(entry).toLowerCase();
        return ['.yaml', '.yml', '.json'].includes(ext);
      });

      if (mapFiles.length === 0) {
        if (!options.quiet) {
          console.warn(`⚠️  No canonical MIDI map files found in ${dirPath}`);
        }
        return true;
      }

      if (!options.quiet && options.verbose) {
        console.log(`📁 Converting ${mapFiles.length} files in ${dirPath}`);
      }

      let allSuccessful = true;
      for (const file of mapFiles) {
        const filePath = join(dirPath, file);
        const success = await this.convertFile(filePath, options, functionMappings, deviceInfoTemplates);
        allSuccessful = allSuccessful && success;
      }

      return allSuccessful;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Unknown error';
      if (!options.quiet) {
        console.error(`❌ Error reading directory ${dirPath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Main conversion function
   */
  async convert(paths: string[], options: ConvertOptions): Promise<boolean> {
    if (paths.length === 0) {
      console.error('Error: No files or directories specified');
      this.showHelp();
      return false;
    }

    // Load function mappings
    let functionMappings: FunctionMapping = {};
    if (options.functions) {
      try {
        functionMappings = this.loadFunctionMappings(options.functions);
        if (!options.quiet && options.verbose) {
          console.log(`📋 Loaded ${Object.keys(functionMappings).length} custom function mappings`);
        }
      } catch (error) {
        console.error(`Error loading function mappings: ${error instanceof Error ? error.message : 'Unknown error'}`);
        return false;
      }
    }

    // Load device info templates
    let deviceInfoTemplates: Record<string, ArdourDeviceInfo> | undefined;
    if (options.deviceInfo) {
      try {
        deviceInfoTemplates = this.loadDeviceInfoTemplates(options.deviceInfo);
        if (!options.quiet && options.verbose) {
          console.log(`📱 Loaded ${Object.keys(deviceInfoTemplates).length} device info templates`);
        }
      } catch (error) {
        console.error(`Error loading device info templates: ${error instanceof Error ? error.message : 'Unknown error'}`);
        return false;
      }
    }

    let allSuccessful = true;

    for (const path of paths) {
      try {
        const stat = statSync(path);

        if (stat.isDirectory()) {
          const success = await this.convertDirectory(path, options, functionMappings, deviceInfoTemplates);
          allSuccessful = allSuccessful && success;
        } else if (stat.isFile()) {
          const success = await this.convertFile(path, options, functionMappings, deviceInfoTemplates);
          allSuccessful = allSuccessful && success;
        } else {
          if (!options.quiet) {
            console.error(`❌ ${path}: Not a file or directory`);
          }
          allSuccessful = false;
        }
      } catch (error) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        if (!options.quiet) {
          console.error(`❌ ${path}: ${message}`);
        }
        allSuccessful = false;
      }
    }

    // Show summary
    if (!options.quiet && this.stats.total > 1) {
      console.log('\n📊 Conversion Summary:');
      console.log(`  Total files: ${this.stats.total}`);
      console.log(`  ✅ Converted: ${this.stats.converted}`);
      console.log(`  ⚠️  Skipped: ${this.stats.skipped}`);
      console.log(`  ❌ Failed: ${this.stats.failed}`);
    }

    return allSuccessful;
  }
}

/**
 * Main CLI entry point
 */
async function main(): Promise<void> {
  const converter = new CanonicalToArdourConverter();
  const { paths, options } = converter.parseArgs();

  if (options.help) {
    converter.showHelp();
    process.exit(0);
  }

  try {
    const success = await converter.convert(paths, options);
    process.exit(success ? 0 : 1);
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

export { CanonicalToArdourConverter, main };