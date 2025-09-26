/**
 * Test suite for YAML parser functionality
 */

import { describe, expect, it } from 'vitest';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';
import type { CanonicalMidiMap } from '@/types/canonical.js';

describe('CanonicalMapParser', () => {
  const validMapData: CanonicalMidiMap = {
    version: '1.0.0',
    device: {
      manufacturer: 'Test Manufacturer',
      model: 'Test Controller',
    },
    metadata: {
      name: 'Test MIDI Map',
      description: 'A test mapping',
      author: 'Test Author',
      tags: ['test', 'controller'],
    },
    plugin: {
      manufacturer: 'Test Plugin Manufacturer',
      name: 'Test Plugin',
      format: 'VST3',
    },
    midi_channel: 1,
    controls: [
      {
        id: 'test_control',
        name: 'Test Control',
        type: 'encoder',
        cc: 1,
        channel: 1,
      },
    ],
  };

  const validYaml = `version: "1.0.0"
device:
  manufacturer: "Test Manufacturer"
  model: "Test Controller"
metadata:
  name: "Test MIDI Map"
  description: "A test mapping"
  author: "Test Author"
  tags:
    - test
    - controller
plugin:
  manufacturer: "Test Plugin Manufacturer"
  name: "Test Plugin"
  format: "VST3"
midi_channel: 1
controls:
  - id: "test_control"
    name: "Test Control"
    type: "encoder"
    cc: 1
    channel: 1`;

  const validJson = JSON.stringify(validMapData, null, 2);

  describe('parseFromYAML', () => {
    it('should parse valid YAML successfully', () => {
      const result = CanonicalMapParser.parseFromYAML(validYaml);

      expect(result.validation.valid).toBe(true);
      expect(result.validation.errors).toHaveLength(0);
      expect(result.map).toBeDefined();
      expect(result.map!.version).toBe('1.0.0');
      expect(result.map!.device.manufacturer).toBe('Test Manufacturer');
      expect(result.map!.metadata.name).toBe('Test MIDI Map');
      expect(result.map!.controls).toHaveLength(1);
    });

    it('should handle minimal valid YAML', () => {
      const minimalYaml = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Minimal Map"
controls: []`;

      const result = CanonicalMapParser.parseFromYAML(minimalYaml);

      expect(result.validation.valid).toBe(true);
      expect(result.map).toBeDefined();
      expect(result.map!.controls).toHaveLength(0);
    });

    it('should return validation errors for invalid YAML structure', () => {
      const invalidYaml = `version: "1.0.0"
device:
  manufacturer: "Test"
  # Missing model field
metadata:
  name: "Invalid Map"
controls: []`;

      const result = CanonicalMapParser.parseFromYAML(invalidYaml);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.length).toBeGreaterThan(0);
      expect(result.map).toBeUndefined();
    });

    it('should handle malformed YAML gracefully', () => {
      const malformedYaml = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model
# Unclosed quote`;

      const result = CanonicalMapParser.parseFromYAML(malformedYaml);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors).toHaveLength(1);
      expect(result.validation.errors[0].code).toBe('PARSE_ERROR');
      expect(result.map).toBeUndefined();
    });

    it('should validate control definitions', () => {
      const yamlWithInvalidControl = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Test Map"
controls:
  - id: "invalid_control"
    name: "Invalid Control"
    type: "invalid_type"
    cc: 999`;

      const result = CanonicalMapParser.parseFromYAML(yamlWithInvalidControl);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.length).toBeGreaterThan(0);
    });

    it('should validate MIDI channel ranges', () => {
      const yamlWithInvalidChannel = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Test Map"
controls:
  - id: "test_control"
    name: "Test Control"
    type: "encoder"
    channel: 17`;

      const result = CanonicalMapParser.parseFromYAML(yamlWithInvalidChannel);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.some(e => e.message.includes('channel'))).toBe(true);
    });

    it('should validate CC number ranges', () => {
      const yamlWithInvalidCC = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Test Map"
controls:
  - id: "test_control"
    name: "Test Control"
    type: "encoder"
    cc: 128`;

      const result = CanonicalMapParser.parseFromYAML(yamlWithInvalidCC);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.some(e => e.message.includes('cc') || e.message.includes('127'))).toBe(true);
    });
  });

  describe('parseFromJSON', () => {
    it('should parse valid JSON successfully', () => {
      const result = CanonicalMapParser.parseFromJSON(validJson);

      expect(result.validation.valid).toBe(true);
      expect(result.validation.errors).toHaveLength(0);
      expect(result.map).toBeDefined();
      expect(result.map!.version).toBe('1.0.0');
      expect(result.map!.device.manufacturer).toBe('Test Manufacturer');
    });

    it('should handle malformed JSON gracefully', () => {
      const malformedJson = `{
        "version": "1.0.0",
        "device": {
          "manufacturer": "Test"
          // Missing comma
          "model": "Model"
        }
      }`;

      const result = CanonicalMapParser.parseFromJSON(malformedJson);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors).toHaveLength(1);
      expect(result.validation.errors[0].code).toBe('PARSE_ERROR');
    });

    it('should validate JSON structure same as YAML', () => {
      const invalidJson = JSON.stringify({
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          // Missing model
        },
        metadata: {
          name: 'Test',
        },
        controls: [],
      });

      const result = CanonicalMapParser.parseFromJSON(invalidJson);

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.length).toBeGreaterThan(0);
    });
  });

  describe('serializeToYAML', () => {
    it('should serialize valid data to formatted YAML', () => {
      const yaml = CanonicalMapParser.serializeToYAML(validMapData);

      expect(yaml).toContain('version: "1.0.0"');
      expect(yaml).toContain('manufacturer: "Test Manufacturer"');
      expect(yaml).toContain('- test');
      expect(yaml).toContain('- controller');

      // Verify it can be parsed back
      const reparsed = CanonicalMapParser.parseFromYAML(yaml);
      expect(reparsed.validation.valid).toBe(true);
    });

    it('should handle objects without optional fields', () => {
      const minimalData = {
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          model: 'Model',
        },
        metadata: {
          name: 'Minimal',
        },
        controls: [],
      };

      const yaml = CanonicalMapParser.serializeToYAML(minimalData as any);

      expect(yaml).toContain('version: "1.0.0"');
      expect(yaml).toContain('controls: []');

      // Verify it can be parsed back
      const reparsed = CanonicalMapParser.parseFromYAML(yaml);
      expect(reparsed.validation.valid).toBe(true);
    });
  });

  describe('serializeToJSON', () => {
    it('should serialize to pretty formatted JSON by default', () => {
      const json = CanonicalMapParser.serializeToJSON(validMapData);

      expect(json).toContain('{\n');
      expect(json).toContain('  "version"');
      expect(json).toContain('    "manufacturer"');

      // Verify it can be parsed back
      const reparsed = CanonicalMapParser.parseFromJSON(json);
      expect(reparsed.validation.valid).toBe(true);
    });

    it('should serialize to compact JSON when pretty=false', () => {
      const json = CanonicalMapParser.serializeToJSON(validMapData, false);

      expect(json).not.toContain('\n');
      expect(json).not.toContain('  ');
      expect(json).toContain('{"version":"1.0.0"');

      // Verify it can be parsed back
      const reparsed = CanonicalMapParser.parseFromJSON(json);
      expect(reparsed.validation.valid).toBe(true);
    });
  });

  describe('validate', () => {
    it('should validate correct data structure', () => {
      const result = CanonicalMapParser.validate(validMapData);

      expect(result.valid).toBe(true);
      expect(result.errors).toHaveLength(0);
    });

    it('should generate warnings for missing optional fields', () => {
      const dataWithoutOptionals = {
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          model: 'Model',
        },
        metadata: {
          name: 'Test Map',
          // No description or author
        },
        controls: [],
      };

      const result = CanonicalMapParser.validate(dataWithoutOptionals);

      expect(result.valid).toBe(true);
      expect(result.warnings.length).toBeGreaterThan(0);
      expect(result.warnings.some(w => w.code === 'MISSING_DESCRIPTION')).toBe(true);
      expect(result.warnings.some(w => w.code === 'MISSING_AUTHOR')).toBe(true);
    });

    it('should detect duplicate CC assignments', () => {
      const dataWithDuplicateCC = {
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          model: 'Model',
        },
        metadata: {
          name: 'Test Map',
          description: 'Test',
          author: 'Test',
        },
        controls: [
          {
            id: 'control1',
            name: 'Control 1',
            type: 'encoder' as const,
            cc: 1,
            channel: 1,
          },
          {
            id: 'control2',
            name: 'Control 2',
            type: 'slider' as const,
            cc: 1,
            channel: 1,
          },
        ],
      };

      const result = CanonicalMapParser.validate(dataWithDuplicateCC);

      expect(result.valid).toBe(true);
      expect(result.warnings.some(w => w.code === 'DUPLICATE_CC_ASSIGNMENT')).toBe(true);
      expect(result.warnings.some(w => w.message.includes('control1, control2'))).toBe(true);
    });

    it('should handle different channels for same CC', () => {
      const dataWithSameCCDifferentChannels = {
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          model: 'Model',
        },
        metadata: {
          name: 'Test Map',
          description: 'Test',
          author: 'Test',
        },
        controls: [
          {
            id: 'control1',
            name: 'Control 1',
            type: 'encoder' as const,
            cc: 1,
            channel: 1,
          },
          {
            id: 'control2',
            name: 'Control 2',
            type: 'encoder' as const,
            cc: 1,
            channel: 2,
          },
        ],
      };

      const result = CanonicalMapParser.validate(dataWithSameCCDifferentChannels);

      expect(result.valid).toBe(true);
      // Should not warn about duplicate CC since they're on different channels
      expect(result.warnings.every(w => w.code !== 'DUPLICATE_CC_ASSIGNMENT')).toBe(true);
    });

    it('should return errors for invalid data', () => {
      const invalidData = {
        version: '1.0.0',
        // Missing required device field
        metadata: {
          name: 'Invalid Map',
        },
        controls: [],
      };

      const result = CanonicalMapParser.validate(invalidData);

      expect(result.valid).toBe(false);
      expect(result.errors.length).toBeGreaterThan(0);
    });
  });

  describe('edge cases and error handling', () => {
    it('should handle empty YAML input', () => {
      const result = CanonicalMapParser.parseFromYAML('');

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors).toHaveLength(1);
    });

    it('should handle empty JSON input', () => {
      const result = CanonicalMapParser.parseFromJSON('');

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors).toHaveLength(1);
    });

    it('should handle null input gracefully', () => {
      const result = CanonicalMapParser.parseFromYAML('null');

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.length).toBeGreaterThan(0);
    });

    it('should handle non-object input', () => {
      const result = CanonicalMapParser.parseFromYAML('"just a string"');

      expect(result.validation.valid).toBe(false);
      expect(result.validation.errors.length).toBeGreaterThan(0);
    });

    it('should handle controls without CC numbers', () => {
      const yamlWithoutCC = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Test Map"
controls:
  - id: "test_control"
    name: "Test Control"
    type: "button"`;

      const result = CanonicalMapParser.parseFromYAML(yamlWithoutCC);

      // This should be valid for certain control types
      expect(result.validation.valid).toBe(true);
    });

    it('should handle button groups correctly', () => {
      const yamlWithButtonGroup = `version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Model"
metadata:
  name: "Test Map"
controls:
  - id: "transport_group"
    name: "Transport Controls"
    type: "button_group"
    buttons:
      - id: "play"
        name: "Play"
        cc: 60
        channel: 1
        mode: "momentary"
      - id: "stop"
        name: "Stop"
        cc: 61
        channel: 1
        mode: "momentary"`;

      const result = CanonicalMapParser.parseFromYAML(yamlWithButtonGroup);

      expect(result.validation.valid).toBe(true);
      expect(result.map!.controls[0].buttons).toHaveLength(2);
    });
  });
});