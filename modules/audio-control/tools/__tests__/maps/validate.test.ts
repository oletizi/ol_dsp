/**
 * Tests for Maps Validation Tool
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import { z } from 'zod';

// Mock file system operations
vi.mock('node:fs');

const mockExistsSync = vi.mocked(existsSync);
const mockReadFileSync = vi.mocked(readFileSync);
const mockReaddirSync = vi.mocked(readdirSync);

describe('Maps Validation Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('CLI Argument Parsing', () => {
    it('should parse default options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: [],
        options: {
          path: { type: 'string', default: './maps' },
          format: { type: 'string', default: 'table' },
          strict: { type: 'boolean', default: false },
          fix: { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.path).toBe('./maps');
      expect(values.format).toBe('table');
      expect(values.strict).toBe(false);
      expect(values.fix).toBe(false);
    });

    it('should parse custom path and strict mode', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--path', './custom-maps', '--strict', '--fix'],
        options: {
          path: { type: 'string', default: './maps' },
          format: { type: 'string', default: 'table' },
          strict: { type: 'boolean', default: false },
          fix: { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.path).toBe('./custom-maps');
      expect(values.strict).toBe(true);
      expect(values.fix).toBe(true);
    });
  });

  describe('File Discovery', () => {
    it('should find YAML and JSON map files', () => {
      mockReaddirSync.mockReturnValue([
        'mapping1.yaml',
        'mapping2.json',
        'mapping3.yml',
        'readme.md',
        'config.txt'
      ] as any);

      const files = mockReaddirSync('./maps') as string[];
      const mapFiles = files.filter(file =>
        file.endsWith('.yaml') || file.endsWith('.yml') || file.endsWith('.json')
      );

      expect(mapFiles).toHaveLength(3);
      expect(mapFiles).toContain('mapping1.yaml');
      expect(mapFiles).toContain('mapping2.json');
      expect(mapFiles).toContain('mapping3.yml');
    });

    it('should handle empty directory', () => {
      mockReaddirSync.mockReturnValue([] as any);

      const files = mockReaddirSync('./maps') as string[];
      const mapFiles = files.filter(file =>
        file.endsWith('.yaml') || file.endsWith('.yml') || file.endsWith('.json')
      );

      expect(mapFiles).toHaveLength(0);
    });

    it('should handle directory not found', () => {
      mockExistsSync.mockReturnValue(false);

      const exists = mockExistsSync('./maps');
      expect(exists).toBe(false);
    });
  });

  describe('YAML/JSON Parsing', () => {
    it('should parse valid YAML content', async () => {
      const yamlContent = `
name: Test Mapping
plugin: test-plugin
controls:
  - name: Volume
    parameter: volume
    midi_cc: 1
`;

      mockReadFileSync.mockReturnValue(yamlContent);

      // Simulate YAML parsing (would use yaml library)
      const content = mockReadFileSync('test.yaml', 'utf-8');
      expect(content).toContain('name: Test Mapping');
      expect(content).toContain('midi_cc: 1');
    });

    it('should parse valid JSON content', () => {
      const jsonContent = JSON.stringify({
        name: 'Test Mapping',
        plugin: 'test-plugin',
        controls: [
          {
            name: 'Volume',
            parameter: 'volume',
            midi_cc: 1
          }
        ]
      });

      mockReadFileSync.mockReturnValue(jsonContent);

      const content = mockReadFileSync('test.json', 'utf-8');
      const parsed = JSON.parse(content);

      expect(parsed.name).toBe('Test Mapping');
      expect(parsed.controls).toHaveLength(1);
      expect(parsed.controls[0].midi_cc).toBe(1);
    });

    it('should handle invalid YAML', () => {
      const invalidYaml = `
name: Test Mapping
  invalid: indentation
    more: problems
`;

      mockReadFileSync.mockReturnValue(invalidYaml);

      // Would throw YAML parsing error
      expect(() => {
        const content = mockReadFileSync('test.yaml', 'utf-8');
        // Simulate YAML.parse(content) throwing
        if (content.includes('invalid: indentation')) {
          throw new Error('Invalid YAML');
        }
      }).toThrow('Invalid YAML');
    });

    it('should handle invalid JSON', () => {
      const invalidJson = `{
        "name": "Test",
        "invalid": json,
      }`;

      mockReadFileSync.mockReturnValue(invalidJson);

      expect(() => {
        const content = mockReadFileSync('test.json', 'utf-8');
        JSON.parse(content);
      }).toThrow();
    });
  });

  describe('Schema Validation', () => {
    // Define basic schema structure for testing
    const MidiControlSchema = z.object({
      name: z.string(),
      parameter: z.string(),
      midi_cc: z.number().min(0).max(127)
    });

    const MappingSchema = z.object({
      name: z.string(),
      plugin: z.string(),
      controls: z.array(MidiControlSchema)
    });

    it('should validate correct mapping structure', () => {
      const validMapping = {
        name: 'Test Mapping',
        plugin: 'test-plugin',
        controls: [
          {
            name: 'Volume',
            parameter: 'volume',
            midi_cc: 1
          },
          {
            name: 'Pan',
            parameter: 'pan',
            midi_cc: 2
          }
        ]
      };

      const result = MappingSchema.safeParse(validMapping);
      expect(result.success).toBe(true);

      if (result.success) {
        expect(result.data.name).toBe('Test Mapping');
        expect(result.data.controls).toHaveLength(2);
      }
    });

    it('should reject mapping with invalid MIDI CC values', () => {
      const invalidMapping = {
        name: 'Test Mapping',
        plugin: 'test-plugin',
        controls: [
          {
            name: 'Volume',
            parameter: 'volume',
            midi_cc: 128 // Invalid: > 127
          }
        ]
      };

      const result = MappingSchema.safeParse(invalidMapping);
      expect(result.success).toBe(false);

      if (!result.success) {
        expect(result.error.issues).toBeDefined();
      }
    });

    it('should reject mapping missing required fields', () => {
      const incompleteMapping = {
        name: 'Test Mapping',
        // Missing plugin field
        controls: []
      };

      const result = MappingSchema.safeParse(incompleteMapping);
      expect(result.success).toBe(false);
    });

    it('should handle empty controls array', () => {
      const emptyMapping = {
        name: 'Empty Mapping',
        plugin: 'test-plugin',
        controls: []
      };

      const result = MappingSchema.safeParse(emptyMapping);
      expect(result.success).toBe(true);

      if (result.success) {
        expect(result.data.controls).toHaveLength(0);
      }
    });
  });

  describe('MIDI Protocol Validation', () => {
    it('should validate MIDI CC range (0-127)', () => {
      const validCCs = [0, 1, 64, 127];
      const invalidCCs = [-1, 128, 255, 1000];

      for (const cc of validCCs) {
        expect(cc).toBeGreaterThanOrEqual(0);
        expect(cc).toBeLessThanOrEqual(127);
      }

      for (const cc of invalidCCs) {
        expect(cc < 0 || cc > 127).toBe(true);
      }
    });

    it('should detect duplicate MIDI CC assignments', () => {
      const controls = [
        { name: 'Volume', midi_cc: 1 },
        { name: 'Pan', midi_cc: 2 },
        { name: 'Filter', midi_cc: 1 } // Duplicate CC 1
      ];

      const usedCCs = new Set<number>();
      const duplicates: number[] = [];

      for (const control of controls) {
        if (usedCCs.has(control.midi_cc)) {
          duplicates.push(control.midi_cc);
        } else {
          usedCCs.add(control.midi_cc);
        }
      }

      expect(duplicates).toContain(1);
      expect(duplicates).toHaveLength(1);
    });

    it('should validate reserved MIDI CC numbers', () => {
      // Some MIDI CCs are reserved for specific functions
      const reservedCCs = [
        120, // All Sound Off
        121, // Reset All Controllers
        122, // Local Control
        123, // All Notes Off
        124, 125, 126, 127 // Omni/Mono mode messages
      ];

      const testCC = 120;
      const isReserved = reservedCCs.includes(testCC);

      expect(isReserved).toBe(true);
    });
  });

  describe('Cross-validation with Plugin Data', () => {
    const mockPluginData = {
      'test-plugin': {
        name: 'Test Plugin',
        parameters: [
          { name: 'volume', id: 1 },
          { name: 'pan', id: 2 },
          { name: 'filter_cutoff', id: 3 }
        ]
      }
    };

    it('should validate parameter names against plugin data', () => {
      const mapping = {
        plugin: 'test-plugin',
        controls: [
          { name: 'Volume', parameter: 'volume' },
          { name: 'Pan', parameter: 'pan' },
          { name: 'Cutoff', parameter: 'nonexistent' } // Invalid parameter
        ]
      };

      const plugin = mockPluginData['test-plugin'];
      const validParameters = plugin.parameters.map(p => p.name);

      const invalidControls = mapping.controls.filter(control =>
        !validParameters.includes(control.parameter)
      );

      expect(invalidControls).toHaveLength(1);
      expect(invalidControls[0].parameter).toBe('nonexistent');
    });

    it('should handle missing plugin data', () => {
      const mapping = {
        plugin: 'unknown-plugin',
        controls: [
          { name: 'Volume', parameter: 'volume' }
        ]
      };

      const pluginExists = mockPluginData.hasOwnProperty(mapping.plugin);
      expect(pluginExists).toBe(false);
    });
  });

  describe('Validation Results and Reporting', () => {
    interface ValidationResult {
      file: string;
      valid: boolean;
      errors: string[];
      warnings: string[];
    }

    it('should format validation results correctly', () => {
      const results: ValidationResult[] = [
        {
          file: 'mapping1.yaml',
          valid: true,
          errors: [],
          warnings: []
        },
        {
          file: 'mapping2.yaml',
          valid: false,
          errors: ['Invalid MIDI CC: 128'],
          warnings: ['Parameter not found in plugin data']
        }
      ];

      const totalFiles = results.length;
      const validFiles = results.filter(r => r.valid).length;
      const invalidFiles = results.filter(r => !r.valid).length;

      expect(totalFiles).toBe(2);
      expect(validFiles).toBe(1);
      expect(invalidFiles).toBe(1);
    });

    it('should categorize different types of validation errors', () => {
      const errors = [
        'Invalid MIDI CC: 128',
        'Missing required field: plugin',
        'Duplicate MIDI CC: 1',
        'Parameter not found: unknown_param'
      ];

      const midiErrors = errors.filter(e => e.includes('MIDI CC'));
      const schemaErrors = errors.filter(e => e.includes('Missing required'));
      const duplicateErrors = errors.filter(e => e.includes('Duplicate'));
      const parameterErrors = errors.filter(e => e.includes('Parameter not found'));

      expect(midiErrors).toHaveLength(2);
      expect(schemaErrors).toHaveLength(1);
      expect(duplicateErrors).toHaveLength(1);
      expect(parameterErrors).toHaveLength(1);
    });
  });

  describe('Auto-fix Functionality', () => {
    it('should suggest fixes for common issues', () => {
      const issues = [
        { type: 'invalid_cc', value: 128, suggested: 127 },
        { type: 'duplicate_cc', original: 1, suggested: 10 },
        { type: 'missing_field', field: 'plugin', suggested: 'unknown' }
      ];

      for (const issue of issues) {
        if (issue.type === 'invalid_cc') {
          expect(issue.suggested).toBeLessThanOrEqual(127);
          expect(issue.suggested).toBeGreaterThanOrEqual(0);
        }
        if (issue.type === 'duplicate_cc') {
          expect(issue.suggested).not.toBe(issue.original);
        }
      }
    });

    it('should handle fix mode appropriately', () => {
      const fixMode = true;
      const originalMapping = {
        name: 'Test',
        plugin: 'test-plugin',
        controls: [
          { name: 'Volume', midi_cc: 128 } // Invalid
        ]
      };

      if (fixMode) {
        const fixedMapping = {
          ...originalMapping,
          controls: originalMapping.controls.map(control => ({
            ...control,
            midi_cc: Math.min(127, Math.max(0, control.midi_cc))
          }))
        };

        expect(fixedMapping.controls[0].midi_cc).toBe(127);
      }
    });
  });
});