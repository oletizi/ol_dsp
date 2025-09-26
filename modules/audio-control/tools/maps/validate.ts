#!/usr/bin/env node

/**
 * Maps Validation Tool
 *
 * Validates canonical MIDI mapping files for format compliance,
 * MIDI protocol correctness, and plugin parameter mappings.
 */

import { readFile } from 'fs/promises';
import { join, basename, extname } from 'path';
import { glob } from 'glob';

import { CanonicalMapParser } from '@/modules/canonical-midi-maps/src/parsers/yaml-parser';
import type { CanonicalMidiMapOutput } from '@/modules/canonical-midi-maps/src/validators/schema';
import type { ValidationResult, ValidationError, ValidationWarning } from '@/tools/types/workflow';
import type { PluginDescriptor } from '@/modules/canonical-midi-maps/src/types/plugin-descriptor';
import { PerformanceCache, cached } from '../cache/performance-cache.js';

interface MapValidationResult extends ValidationResult {
  /** Map file path */
  filePath: string;

  /** Map identifier */
  mapId: string;

  /** MIDI protocol validation */
  midiValidation: ValidationResult;

  /** Plugin cross-validation */
  pluginValidation?: ValidationResult;

  /** Performance metrics */
  performance: {
    parseTime: number;
    validationTime: number;
    fileSize: number;
  };
}

interface ValidationOptions {
  /** Validate against plugin descriptors */
  checkPlugins?: boolean;

  /** Plugin descriptors directory */
  pluginDescriptorsDir?: string;

  /** Strict MIDI validation */
  strict?: boolean;

  /** Include warnings in results */
  includeWarnings?: boolean;

  /** Maximum allowed file size (bytes) */
  maxFileSize?: number;
}

class MapsValidator {
  private readonly pluginDescriptors = new Map<string, PluginDescriptor>();
  private readonly cache: PerformanceCache;

  constructor(private readonly options: ValidationOptions = {}) {
    // Set defaults
    this.options.checkPlugins ??= false;
    this.options.strict ??= true;
    this.options.includeWarnings ??= true;
    this.options.maxFileSize ??= 1024 * 1024; // 1MB

    // Initialize performance cache
    this.cache = new PerformanceCache({
      cacheDir: '.cache/maps-validation',
      ttl: 10 * 60 * 1000, // 10 minutes for validation cache
    });
  }

  /**
   * Load plugin descriptors for cross-validation
   */
  async loadPluginDescriptors(descriptorsDir: string): Promise<void> {
    try {
      const descriptorFiles = await glob(join(descriptorsDir, '**/*.json'));

      for (const file of descriptorFiles) {
        try {
          const content = await readFile(file, 'utf8');
          const descriptor: PluginDescriptor = JSON.parse(content);

          const pluginId = this.generatePluginId(descriptor);
          this.pluginDescriptors.set(pluginId, descriptor);
        } catch (error) {
          console.warn(`Failed to load plugin descriptor ${file}: ${error instanceof Error ? error.message : 'Unknown error'}`);
        }
      }

      console.log(`Loaded ${this.pluginDescriptors.size} plugin descriptors`);
    } catch (error) {
      console.error(`Failed to load plugin descriptors from ${descriptorsDir}: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Validate a single map file with caching
   */
  async validateMapFile(filePath: string): Promise<MapValidationResult> {
    const startTime = performance.now();

    // Check cache first for validation result
    const cacheKey = { filePath, options: this.options };
    const cached = await this.cache.get<MapValidationResult>('validation', cacheKey);

    if (cached) {
      // Update performance timing to show cache hit
      cached.performance.validationTime = performance.now() - startTime;
      return cached;
    }

    try {
      // Check file size
      const stats = await import('fs/promises').then(fs => fs.stat(filePath));
      if (this.options.maxFileSize && stats.size > this.options.maxFileSize) {
        return {
          filePath,
          mapId: basename(filePath, extname(filePath)),
          valid: false,
          errors: [{
            path: 'file',
            message: `File too large: ${stats.size} bytes exceeds limit of ${this.options.maxFileSize} bytes`,
            code: 'FILE_TOO_LARGE',
            severity: 'high' as const,
          }],
          warnings: [],
          midiValidation: { valid: false, errors: [], warnings: [] },
          performance: {
            parseTime: 0,
            validationTime: performance.now() - startTime,
            fileSize: stats.size,
          },
        };
      }

      // Read and parse the map file
      const parseStart = performance.now();
      const content = await readFile(filePath, 'utf8');
      const fileExtension = extname(filePath).toLowerCase();

      let parseResult: { map?: CanonicalMidiMapOutput; validation: ValidationResult };

      if (fileExtension === '.yaml' || fileExtension === '.yml') {
        parseResult = CanonicalMapParser.parseFromYAML(content);
      } else if (fileExtension === '.json') {
        parseResult = CanonicalMapParser.parseFromJSON(content);
      } else {
        return {
          filePath,
          mapId: basename(filePath, extname(filePath)),
          valid: false,
          errors: [{
            path: 'file',
            message: `Unsupported file format: ${fileExtension}`,
            code: 'UNSUPPORTED_FORMAT',
            severity: 'high' as const,
          }],
          warnings: [],
          midiValidation: { valid: false, errors: [], warnings: [] },
          performance: {
            parseTime: 0,
            validationTime: performance.now() - startTime,
            fileSize: stats.size,
          },
        };
      }

      const parseTime = performance.now() - parseStart;

      if (!parseResult.validation.valid || !parseResult.map) {
        return {
          filePath,
          mapId: basename(filePath, extname(filePath)),
          valid: false,
          errors: parseResult.validation.errors.map(err => ({
            ...err,
            severity: 'high' as const,
          })),
          warnings: parseResult.validation.warnings.map(warn => ({ ...warn })),
          midiValidation: { valid: false, errors: [], warnings: [] },
          performance: {
            parseTime,
            validationTime: performance.now() - startTime,
            fileSize: stats.size,
          },
        };
      }

      // Validate MIDI protocol compliance
      const midiValidation = this.validateMidiProtocol(parseResult.map);

      // Cross-validate with plugin descriptors if requested
      let pluginValidation: ValidationResult | undefined;
      if (this.options.checkPlugins && parseResult.map.plugin) {
        pluginValidation = this.validatePluginMapping(parseResult.map);
      }

      // Combine results
      const allErrors: ValidationError[] = [
        ...parseResult.validation.errors.map(err => ({ ...err, severity: 'high' as const })),
        ...midiValidation.errors,
      ];

      const allWarnings: ValidationWarning[] = [
        ...parseResult.validation.warnings,
        ...midiValidation.warnings,
      ];

      if (pluginValidation) {
        allErrors.push(...pluginValidation.errors);
        allWarnings.push(...pluginValidation.warnings);
      }

      const isValid = allErrors.length === 0;
      const validationTime = performance.now() - startTime;

      const result: MapValidationResult = {
        filePath,
        mapId: this.generateMapId(parseResult.map),
        valid: isValid,
        errors: allErrors,
        warnings: this.options.includeWarnings ? allWarnings : [],
        score: this.calculateValidationScore(allErrors, allWarnings),
        midiValidation,
        pluginValidation,
        performance: {
          parseTime,
          validationTime,
          fileSize: stats.size,
        },
      };

      // Cache successful validation results
      if (isValid || allErrors.length <= 3) {
        await this.cache.set('validation', cacheKey, result, {
          timestamp: Date.now(),
          fileSize: stats.size
        });
      }

      return result;

    } catch (error) {
      return {
        filePath,
        mapId: basename(filePath, extname(filePath)),
        valid: false,
        errors: [{
          path: 'file',
          message: `Validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
          code: 'VALIDATION_ERROR',
          severity: 'critical' as const,
        }],
        warnings: [],
        midiValidation: { valid: false, errors: [], warnings: [] },
        performance: {
          parseTime: 0,
          validationTime: performance.now() - startTime,
          fileSize: 0,
        },
      };
    }
  }

  /**
   * Validate MIDI protocol compliance
   */
  private validateMidiProtocol(map: CanonicalMidiMapOutput): ValidationResult {
    const errors: ValidationError[] = [];
    const warnings: ValidationWarning[] = [];

    // Validate MIDI channel (1-16)
    if (map.midi_channel !== undefined) {
      if (map.midi_channel < 1 || map.midi_channel > 16) {
        errors.push({
          path: 'midi_channel',
          message: `MIDI channel ${map.midi_channel} is out of range (1-16)`,
          code: 'INVALID_MIDI_CHANNEL',
          severity: 'high',
        });
      }
    }

    // Validate controls
    const usedCCs = new Map<string, string[]>();

    map.controls.forEach((control, index) => {
      const basePath = `controls[${index}]`;

      // Validate CC numbers (0-127)
      if (control.cc !== undefined) {
        if (control.cc < 0 || control.cc > 127) {
          errors.push({
            path: `${basePath}.cc`,
            message: `CC ${control.cc} is out of range (0-127)`,
            code: 'INVALID_CC_NUMBER',
            severity: 'high',
          });
        }

        // Track CC usage for duplicates
        const channelKey = typeof control.channel === 'string' ? control.channel :
                          typeof control.channel === 'number' ? control.channel.toString() :
                          map.midi_channel?.toString() || 'global';
        const ccKey = `${channelKey}:${control.cc}`;

        if (!usedCCs.has(ccKey)) {
          usedCCs.set(ccKey, []);
        }
        usedCCs.get(ccKey)!.push(control.id);
      }

      // Validate channel references
      if (typeof control.channel === 'number') {
        if (control.channel < 1 || control.channel > 16) {
          errors.push({
            path: `${basePath}.channel`,
            message: `MIDI channel ${control.channel} is out of range (1-16)`,
            code: 'INVALID_CONTROL_CHANNEL',
            severity: 'high',
          });
        }
      }

      // Validate value ranges
      if (control.range) {
        const [min, max] = control.range;
        if (min < 0 || min > 127 || max < 0 || max > 127) {
          if (this.options.strict) {
            errors.push({
              path: `${basePath}.range`,
              message: `Range [${min}, ${max}] contains values outside MIDI range (0-127)`,
              code: 'INVALID_VALUE_RANGE',
              severity: 'medium',
            });
          } else {
            warnings.push({
              path: `${basePath}.range`,
              message: `Range [${min}, ${max}] may contain non-standard MIDI values`,
              code: 'NON_STANDARD_RANGE',
            });
          }
        }

        if (min >= max) {
          errors.push({
            path: `${basePath}.range`,
            message: `Invalid range: minimum (${min}) must be less than maximum (${max})`,
            code: 'INVALID_RANGE_ORDER',
            severity: 'medium',
          });
        }
      }

      // Validate button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach((button, buttonIndex) => {
          const buttonPath = `${basePath}.buttons[${buttonIndex}]`;

          if (button.cc < 0 || button.cc > 127) {
            errors.push({
              path: `${buttonPath}.cc`,
              message: `Button CC ${button.cc} is out of range (0-127)`,
              code: 'INVALID_BUTTON_CC',
              severity: 'high',
            });
          }
        });
      }
    });

    // Check for duplicate CC assignments
    usedCCs.forEach((controlIds, ccKey) => {
      if (controlIds.length > 1) {
        warnings.push({
          path: 'controls',
          message: `Duplicate CC assignment (${ccKey}) used by controls: ${controlIds.join(', ')}`,
          code: 'DUPLICATE_CC_ASSIGNMENT',
          category: 'midi-protocol',
        });
      }
    });

    return {
      valid: errors.length === 0,
      errors,
      warnings,
    };
  }

  /**
   * Validate plugin parameter mappings
   */
  private validatePluginMapping(map: CanonicalMidiMapOutput): ValidationResult {
    const errors: ValidationError[] = [];
    const warnings: ValidationWarning[] = [];

    if (!map.plugin) {
      return { valid: true, errors, warnings };
    }

    // Find matching plugin descriptor
    const pluginId = this.generatePluginId({
      plugin: {
        manufacturer: map.plugin.manufacturer,
        name: map.plugin.name,
        version: map.plugin.version || '',
        format: map.plugin.format || 'VST3',
      },
    });

    const descriptor = this.pluginDescriptors.get(pluginId);
    if (!descriptor) {
      warnings.push({
        path: 'plugin',
        message: `No plugin descriptor found for ${map.plugin.manufacturer} ${map.plugin.name}`,
        code: 'MISSING_PLUGIN_DESCRIPTOR',
        category: 'plugin-validation',
      });
      return { valid: true, errors, warnings };
    }

    // Validate parameter mappings
    const availableParams = new Map(descriptor.parameters.map(p => [p.index.toString(), p]));

    map.controls.forEach((control, index) => {
      if (control.plugin_parameter !== undefined) {
        const paramId = control.plugin_parameter.toString();
        const param = availableParams.get(paramId);

        if (!param) {
          errors.push({
            path: `controls[${index}].plugin_parameter`,
            message: `Plugin parameter ${paramId} not found in ${descriptor.plugin.name}`,
            code: 'INVALID_PLUGIN_PARAMETER',
            severity: 'high',
          });
        } else if (param.automatable === false) {
          warnings.push({
            path: `controls[${index}].plugin_parameter`,
            message: `Parameter ${param.name} (${paramId}) is not automatable`,
            code: 'NON_AUTOMATABLE_PARAMETER',
            category: 'plugin-validation',
          });
        }
      }

      // Check button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach((button, buttonIndex) => {
          if (button.plugin_parameter !== undefined) {
            const paramId = button.plugin_parameter.toString();
            const param = availableParams.get(paramId);

            if (!param) {
              errors.push({
                path: `controls[${index}].buttons[${buttonIndex}].plugin_parameter`,
                message: `Plugin parameter ${paramId} not found in ${descriptor.plugin.name}`,
                code: 'INVALID_BUTTON_PARAMETER',
                severity: 'high',
              });
            }
          }
        });
      }
    });

    return {
      valid: errors.length === 0,
      errors,
      warnings,
    };
  }

  /**
   * Calculate validation score (0-1)
   */
  private calculateValidationScore(errors: ValidationError[], warnings: ValidationWarning[]): number {
    if (errors.length === 0 && warnings.length === 0) return 1.0;

    const errorPenalty = errors.reduce((sum, error) => {
      switch (error.severity) {
        case 'critical': return sum + 0.5;
        case 'high': return sum + 0.3;
        case 'medium': return sum + 0.2;
        case 'low': return sum + 0.1;
        default: return sum + 0.2;
      }
    }, 0);

    const warningPenalty = warnings.length * 0.05;
    const totalPenalty = Math.min(errorPenalty + warningPenalty, 1.0);

    return Math.max(0, 1.0 - totalPenalty);
  }

  private generatePluginId(descriptor: { plugin: { manufacturer: string; name: string } }): string {
    return `${descriptor.plugin.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${descriptor.plugin.name.toLowerCase().replace(/\s+/g, '-')}`;
  }

  private generateMapId(map: CanonicalMidiMapOutput): string {
    const device = `${map.device.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.device.model.toLowerCase().replace(/\s+/g, '-')}`;
    const plugin = map.plugin ? `_${map.plugin.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.plugin.name.toLowerCase().replace(/\s+/g, '-')}` : '';
    return device + plugin;
  }
}

// CLI interface
async function main() {
  const args = process.argv.slice(2);

  // Handle help flag
  if (args.includes('--help') || args.includes('-h')) {
    console.log('Usage: validate.ts <map-file-or-directory> [options]');
    console.log('');
    console.log('Options:');
    console.log('  --check-plugins         Cross-validate with plugin descriptors');
    console.log('  --descriptors-dir <dir>  Plugin descriptors directory');
    console.log('  --strict                 Enable strict validation mode');
    console.log('  --no-warnings           Suppress warnings in output');
    console.log('  --max-file-size <bytes>  Maximum allowed file size');
    console.log('  -h, --help              Show this help message');
    process.exit(0);
  }

  if (args.length === 0) {
    console.error('Usage: validate.ts <map-file-or-directory> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --check-plugins         Cross-validate with plugin descriptors');
    console.error('  --descriptors-dir <dir>  Plugin descriptors directory');
    console.error('  --strict                 Enable strict validation mode');
    console.error('  --no-warnings           Suppress warnings in output');
    console.error('  --max-file-size <bytes>  Maximum allowed file size');
    console.error('  -h, --help              Show this help message');
    process.exit(1);
  }

  const inputPath = args[0];
  const options: ValidationOptions = {};

  for (let i = 1; i < args.length; i++) {
    switch (args[i]) {
      case '--check-plugins':
        options.checkPlugins = true;
        break;
      case '--descriptors-dir':
        options.pluginDescriptorsDir = args[++i];
        break;
      case '--strict':
        options.strict = true;
        break;
      case '--no-warnings':
        options.includeWarnings = false;
        break;
      case '--max-file-size':
        options.maxFileSize = parseInt(args[++i]);
        break;
    }
  }

  const validator = new MapsValidator(options);

  // Load plugin descriptors if needed
  if (options.checkPlugins && options.pluginDescriptorsDir) {
    await validator.loadPluginDescriptors(options.pluginDescriptorsDir);
  }

  // Determine if input is file or directory
  const { stat } = await import('fs/promises');
  const inputStat = await stat(inputPath);

  let mapFiles: string[] = [];
  if (inputStat.isFile()) {
    mapFiles = [inputPath];
  } else if (inputStat.isDirectory()) {
    mapFiles = await glob(join(inputPath, '**/*.{yaml,yml,json}'));
  }

  if (mapFiles.length === 0) {
    console.error('No map files found');
    process.exit(1);
  }

  console.log(`Validating ${mapFiles.length} map files...`);
  console.log('');

  // Validate all files
  const results: MapValidationResult[] = [];
  let validCount = 0;

  for (const filePath of mapFiles) {
    const result = await validator.validateMapFile(filePath);
    results.push(result);

    if (result.valid) {
      validCount++;
      console.log(`✅ ${result.mapId} (${result.performance.validationTime.toFixed(1)}ms)`);
    } else {
      console.log(`❌ ${result.mapId} (${result.errors.length} errors, ${result.warnings.length} warnings)`);

      // Show first few errors
      result.errors.slice(0, 3).forEach(error => {
        console.log(`   • ${error.path}: ${error.message}`);
      });

      if (result.errors.length > 3) {
        console.log(`   ... and ${result.errors.length - 3} more errors`);
      }
    }
  }

  // Summary
  console.log('');
  console.log(`Validation complete: ${validCount}/${mapFiles.length} maps valid`);

  if (results.length > 1) {
    const avgScore = results.reduce((sum, r) => sum + (r.score || 0), 0) / results.length;
    const avgTime = results.reduce((sum, r) => sum + r.performance.validationTime, 0) / results.length;
    const totalSize = results.reduce((sum, r) => sum + r.performance.fileSize, 0);

    console.log(`Average score: ${(avgScore * 100).toFixed(1)}%`);
    console.log(`Average validation time: ${avgTime.toFixed(1)}ms`);
    console.log(`Total size: ${(totalSize / 1024).toFixed(1)}KB`);
  }

  // Exit with non-zero code if any validation failed
  const hasErrors = results.some(r => !r.valid);
  process.exit(hasErrors ? 1 : 0);
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch(console.error);
}

export { MapsValidator, type MapValidationResult, type ValidationOptions };