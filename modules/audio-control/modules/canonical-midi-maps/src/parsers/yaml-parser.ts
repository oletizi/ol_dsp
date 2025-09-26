import { parse as parseYAML, stringify as stringifyYAML } from 'yaml';
import { CanonicalMidiMapSchema, type CanonicalMidiMapOutput } from '@/validators/schema.js';
import type { ValidationResult } from '@/types/canonical.js';

/**
 * Parser for canonical MIDI map files in YAML and JSON formats.
 * Provides validation, parsing, and serialization with comprehensive error reporting.
 *
 * Features:
 * - Bidirectional YAML/JSON conversion
 * - Zod schema validation with detailed error messages
 * - Warning generation for best practices
 * - Duplicate CC detection
 * - Missing metadata warnings
 *
 * @example
 * ```typescript
 * const yamlContent = `
 * version: "1.0.0"
 * device:
 *   manufacturer: "Novation"
 *   model: "Launchkey MK3"
 * metadata:
 *   name: "My Map"
 * controls: []
 * `;
 *
 * const result = CanonicalMapParser.parseFromYAML(yamlContent);
 * if (result.validation.valid && result.map) {
 *   console.log('Parsed map:', result.map.metadata.name);
 * } else {
 *   console.error('Validation errors:', result.validation.errors);
 * }
 * ```
 */
export class CanonicalMapParser {
  /**
   * Parses a canonical MIDI map from YAML format.
   * Validates the parsed data against the canonical schema and returns both the map and validation results.
   *
   * @param yamlContent - YAML string to parse
   * @returns Object containing the parsed map (if valid) and validation results
   *
   * @example
   * ```typescript
   * const yamlMap = `
   * version: "1.0.0"
   * device:
   *   manufacturer: "Akai"
   *   model: "MPK Mini MK3"
   * metadata:
   *   name: "Akai MPK Mini Setup"
   *   description: "Basic controls for music production"
   * controls:
   *   - id: "volume"
   *     name: "Volume"
   *     type: "slider"
   *     cc: 7
   *     channel: 1
   * `;
   *
   * const result = CanonicalMapParser.parseFromYAML(yamlMap);
   * if (result.validation.valid && result.map) {
   *   console.log(`Loaded: ${result.map.metadata.name}`);
   *   console.log(`Controls: ${result.map.controls.length}`);
   * }
   * ```
   */
  static parseFromYAML(yamlContent: string): { map?: CanonicalMidiMapOutput; validation: ValidationResult } {
    try {
      const rawData: unknown = parseYAML(yamlContent);
      const result = CanonicalMidiMapSchema.safeParse(rawData);

      if (result.success) {
        return {
          map: result.data,
          validation: {
            valid: true,
            errors: [],
            warnings: [],
          },
        };
      } else {
        return {
          validation: {
            valid: false,
            errors: result.error.errors.map(err => ({
              path: err.path.join('.'),
              message: err.message,
              code: err.code,
            })),
            warnings: [],
          },
        };
      }
    } catch (error) {
      return {
        validation: {
          valid: false,
          errors: [
            {
              path: 'root',
              message: error instanceof Error ? error.message : 'Unknown parsing error',
              code: 'PARSE_ERROR',
            },
          ],
          warnings: [],
        },
      };
    }
  }

  /**
   * Parses a canonical MIDI map from JSON format.
   * Validates the parsed data against the canonical schema and returns both the map and validation results.
   *
   * @param jsonContent - JSON string to parse
   * @returns Object containing the parsed map (if valid) and validation results
   *
   * @example
   * ```typescript
   * const jsonMap = JSON.stringify({
   *   version: "1.0.0",
   *   device: { manufacturer: "Novation", model: "Launchkey MK3" },
   *   metadata: { name: "Studio Setup" },
   *   controls: [
   *     { id: "filter", name: "Filter", type: "encoder", cc: 20, channel: 1 }
   *   ]
   * });
   *
   * const result = CanonicalMapParser.parseFromJSON(jsonMap);
   * if (result.validation.valid && result.map) {
   *   console.log('JSON map loaded successfully');
   * }
   * ```
   */
  static parseFromJSON(jsonContent: string): { map?: CanonicalMidiMapOutput; validation: ValidationResult } {
    try {
      const rawData: unknown = JSON.parse(jsonContent);
      const result = CanonicalMidiMapSchema.safeParse(rawData);

      if (result.success) {
        return {
          map: result.data,
          validation: {
            valid: true,
            errors: [],
            warnings: [],
          },
        };
      } else {
        return {
          validation: {
            valid: false,
            errors: result.error.errors.map(err => ({
              path: err.path.join('.'),
              message: err.message,
              code: err.code,
            })),
            warnings: [],
          },
        };
      }
    } catch (error) {
      return {
        validation: {
          valid: false,
          errors: [
            {
              path: 'root',
              message: error instanceof Error ? error.message : 'Unknown parsing error',
              code: 'PARSE_ERROR',
            },
          ],
          warnings: [],
        },
      };
    }
  }

  /**
   * Serializes a canonical MIDI map to YAML format.
   * Uses formatted YAML output with proper indentation and line width for readability.
   *
   * @param map - The canonical MIDI map to serialize
   * @returns YAML string representation
   *
   * @example
   * ```typescript
   * const map: CanonicalMidiMapOutput = {
   *   version: "1.0.0",
   *   device: { manufacturer: "Akai", model: "MPK Mini" },
   *   metadata: { name: "My Setup" },
   *   controls: []
   * };
   *
   * const yaml = CanonicalMapParser.serializeToYAML(map);
   * console.log(yaml);
   * // Output:
   * // version: "1.0.0"
   * // device:
   * //   manufacturer: Akai
   * //   model: MPK Mini
   * // ...
   * ```
   */
  static serializeToYAML(map: CanonicalMidiMapOutput): string {
    return stringifyYAML(map, {
      indent: 2,
      lineWidth: 100,
      minContentWidth: 20,
    });
  }

  /**
   * Serializes a canonical MIDI map to JSON format.
   * Optionally formats the JSON for human readability.
   *
   * @param map - The canonical MIDI map to serialize
   * @param pretty - Whether to format JSON with indentation (default: true)
   * @returns JSON string representation
   *
   * @example
   * ```typescript
   * const map: CanonicalMidiMapOutput = {
   *   version: "1.0.0",
   *   device: { manufacturer: "Novation", model: "Launchkey" },
   *   metadata: { name: "Production Setup" },
   *   controls: []
   * };
   *
   * // Pretty-printed JSON (default)
   * const prettyJson = CanonicalMapParser.serializeToJSON(map);
   *
   * // Compact JSON
   * const compactJson = CanonicalMapParser.serializeToJSON(map, false);
   * ```
   */
  static serializeToJSON(map: CanonicalMidiMapOutput, pretty = true): string {
    return JSON.stringify(map, null, pretty ? 2 : 0);
  }

  /**
   * Validates a parsed object against the canonical MIDI map schema.
   * Provides detailed validation results including errors and warnings.
   *
   * @param map - Object to validate (typically from JSON.parse or yaml.parse)
   * @returns Validation result with errors, warnings, and validity status
   *
   * @example
   * ```typescript
   * const unknownData = JSON.parse(someJsonString);
   * const validation = CanonicalMapParser.validate(unknownData);
   *
   * if (validation.valid) {
   *   console.log('Map is valid!');
   *   if (validation.warnings.length > 0) {
   *     console.log('Warnings:', validation.warnings.map(w => w.message));
   *   }
   * } else {
   *   console.error('Validation failed:');
   *   validation.errors.forEach(error => {
   *     console.error(`${error.path}: ${error.message}`);
   *   });
   * }
   * ```
   */
  static validate(map: unknown): ValidationResult {
    const result = CanonicalMidiMapSchema.safeParse(map);

    if (result.success) {
      return {
        valid: true,
        errors: [],
        warnings: this.generateWarnings(result.data),
      };
    } else {
      return {
        valid: false,
        errors: result.error.errors.map(err => ({
          path: err.path.join('.'),
          message: err.message,
          code: err.code,
        })),
        warnings: [],
      };
    }
  }

  /**
   * Generates warnings for MIDI map best practices.
   * Checks for common issues like duplicate CC assignments and missing metadata.
   *
   * @private
   * @param map - Validated canonical MIDI map
   * @returns Array of warning objects
   */
  private static generateWarnings(map: CanonicalMidiMapOutput): Array<{ path: string; message: string; code: string }> {
    const warnings: Array<{ path: string; message: string; code: string }> = [];

    // Check for duplicate CC numbers
    const ccNumbers = new Map<string, string[]>();
    map.controls.forEach((control) => {
      if (control.cc !== undefined) {
        const key = `cc-${control.channel || 'global'}-${control.cc}`;
        if (!ccNumbers.has(key)) {
          ccNumbers.set(key, []);
        }
        const existingControls = ccNumbers.get(key);
        if (existingControls) {
          existingControls.push(control.id);
        }
      }
    });

    ccNumbers.forEach((controlIds, key) => {
      if (controlIds.length > 1) {
        warnings.push({
          path: 'controls',
          message: `Duplicate CC assignment (${key}) used by controls: ${controlIds.join(', ')}`,
          code: 'DUPLICATE_CC_ASSIGNMENT',
        });
      }
    });

    // Check for missing metadata
    if (!map.metadata.description) {
      warnings.push({
        path: 'metadata.description',
        message: 'Consider adding a description for better documentation',
        code: 'MISSING_DESCRIPTION',
      });
    }

    if (!map.metadata.author) {
      warnings.push({
        path: 'metadata.author',
        message: 'Consider adding an author for better tracking',
        code: 'MISSING_AUTHOR',
      });
    }

    return warnings;
  }
}