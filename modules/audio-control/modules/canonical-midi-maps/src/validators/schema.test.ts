/**
 * Test suite for validation schemas
 */

import { describe, expect, it } from 'vitest';
import { CanonicalMidiMapSchema } from '@/validators/schema.js';
import type { CanonicalMidiMapInput } from '@/validators/schema.js';

describe('CanonicalMidiMapSchema', () => {
  const validMapData: CanonicalMidiMapInput = {
    version: '1.0.0',
    device: {
      manufacturer: 'Test Manufacturer',
      model: 'Test Controller',
    },
    metadata: {
      name: 'Test MIDI Map',
    },
    controls: [],
  };

  describe('basic validation', () => {
    it('should validate a minimal valid map', () => {
      const result = CanonicalMidiMapSchema.safeParse(validMapData);
      expect(result.success).toBe(true);
      if (result.success) {
        expect(result.data.version).toBe('1.0.0');
        expect(result.data.device.manufacturer).toBe('Test Manufacturer');
        expect(result.data.metadata.name).toBe('Test MIDI Map');
        expect(result.data.controls).toEqual([]);
      }
    });

    it('should validate a complete valid map', () => {
      const completeData: CanonicalMidiMapInput = {
        version: '1.0.0',
        device: {
          manufacturer: 'Novation',
          model: 'Launchkey MK3 49',
          firmware: '1.0.5',
        },
        metadata: {
          name: 'Complete Test Map',
          description: 'A comprehensive test mapping',
          author: 'Test Author',
          date: '2024-01-15',
          tags: ['test', 'complete'],
        },
        plugin: {
          manufacturer: 'Native Instruments',
          name: 'Massive X',
          version: '1.4.1',
          format: 'VST3',
          description: 'Wavetable synthesizer',
          notes: 'Requires version 1.4+',
        },
        midi_channel: 1,
        midi_channel_registry: 'channels.yaml',
        controls: [
          {
            id: 'encoder_1',
            name: 'Filter Cutoff',
            type: 'encoder',
            cc: 20,
            channel: 1,
            range: [0, 127],
            description: 'Controls low-pass filter cutoff frequency',
            plugin_parameter: 'filter.cutoff',
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(completeData);
      expect(result.success).toBe(true);
    });

    it('should require version field', () => {
      const dataWithoutVersion = { ...validMapData };
      delete (dataWithoutVersion as any).version;

      const result = CanonicalMidiMapSchema.safeParse(dataWithoutVersion);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('version'))).toBe(true);
      }
    });

    it('should require device field', () => {
      const dataWithoutDevice = { ...validMapData };
      delete (dataWithoutDevice as any).device;

      const result = CanonicalMidiMapSchema.safeParse(dataWithoutDevice);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('device'))).toBe(true);
      }
    });

    it('should require metadata field', () => {
      const dataWithoutMetadata = { ...validMapData };
      delete (dataWithoutMetadata as any).metadata;

      const result = CanonicalMidiMapSchema.safeParse(dataWithoutMetadata);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('metadata'))).toBe(true);
      }
    });

    it('should require controls field', () => {
      const dataWithoutControls = { ...validMapData };
      delete (dataWithoutControls as any).controls;

      const result = CanonicalMidiMapSchema.safeParse(dataWithoutControls);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('controls'))).toBe(true);
      }
    });
  });

  describe('device validation', () => {
    it('should require device manufacturer', () => {
      const invalidData = {
        ...validMapData,
        device: { model: 'Test Model' },
      };

      const result = CanonicalMidiMapSchema.safeParse(invalidData);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('manufacturer'))).toBe(true);
      }
    });

    it('should require device model', () => {
      const invalidData = {
        ...validMapData,
        device: { manufacturer: 'Test Manufacturer' },
      };

      const result = CanonicalMidiMapSchema.safeParse(invalidData);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('model'))).toBe(true);
      }
    });

    it('should accept optional firmware field', () => {
      const dataWithFirmware = {
        ...validMapData,
        device: {
          manufacturer: 'Test Manufacturer',
          model: 'Test Model',
          firmware: '1.0.5',
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithFirmware);
      expect(result.success).toBe(true);
    });
  });

  describe('metadata validation', () => {
    it('should require metadata name', () => {
      const invalidData = {
        ...validMapData,
        metadata: {},
      };

      const result = CanonicalMidiMapSchema.safeParse(invalidData);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('name'))).toBe(true);
      }
    });

    it('should accept all optional metadata fields', () => {
      const dataWithAllMetadata = {
        ...validMapData,
        metadata: {
          name: 'Test Map',
          description: 'A test mapping',
          author: 'Test Author',
          date: '2024-01-15',
          tags: ['test', 'example'],
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithAllMetadata);
      expect(result.success).toBe(true);
    });

    it('should validate tags as array of strings', () => {
      const invalidData = {
        ...validMapData,
        metadata: {
          name: 'Test Map',
          tags: ['valid', 123, 'invalid'] as any,
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(invalidData);
      expect(result.success).toBe(false);
    });
  });

  describe('plugin validation', () => {
    it('should accept optional plugin field', () => {
      const result = CanonicalMidiMapSchema.safeParse(validMapData);
      expect(result.success).toBe(true);
    });

    it('should require plugin name when plugin is provided', () => {
      const dataWithIncompletePlugin = {
        ...validMapData,
        plugin: { manufacturer: 'Test Manufacturer' },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithIncompletePlugin);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('name'))).toBe(true);
      }
    });

    it('should validate plugin format enum', () => {
      const dataWithInvalidFormat = {
        ...validMapData,
        plugin: {
          manufacturer: 'Test',
          name: 'Test Plugin',
          format: 'INVALID_FORMAT' as any,
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidFormat);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('format'))).toBe(true);
      }
    });

    it('should accept valid plugin formats', () => {
      const validFormats = ['VST', 'VST3', 'AU', 'AAX', 'LV2', 'CLAP'];

      validFormats.forEach(format => {
        const dataWithFormat = {
          ...validMapData,
          plugin: {
            manufacturer: 'Test',
            name: 'Test Plugin',
            format: format as any,
          },
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithFormat);
        expect(result.success).toBe(true);
      });
    });
  });

  describe('MIDI channel validation', () => {
    it('should accept valid MIDI channel numbers', () => {
      for (let channel = 1; channel <= 16; channel++) {
        const dataWithChannel = {
          ...validMapData,
          midi_channel: channel,
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithChannel);
        expect(result.success).toBe(true);
      }
    });

    it('should reject invalid MIDI channel numbers', () => {
      const invalidChannels = [0, 17, -1, 100];

      invalidChannels.forEach(channel => {
        const dataWithChannel = {
          ...validMapData,
          midi_channel: channel,
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithChannel);
        expect(result.success).toBe(false);
      });
    });

    it('should accept optional MIDI channel registry', () => {
      const dataWithRegistry = {
        ...validMapData,
        midi_channel_registry: 'channels.yaml',
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithRegistry);
      expect(result.success).toBe(true);
    });
  });

  describe('controls validation', () => {
    it('should accept empty controls array', () => {
      const result = CanonicalMidiMapSchema.safeParse(validMapData);
      expect(result.success).toBe(true);
    });

    it('should validate control IDs are strings', () => {
      const dataWithInvalidId = {
        ...validMapData,
        controls: [
          {
            id: 123 as any,
            name: 'Test Control',
            type: 'encoder' as const,
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidId);
      expect(result.success).toBe(false);
    });

    it('should validate control types', () => {
      const validTypes = ['encoder', 'slider', 'button', 'button_group'];

      validTypes.forEach(type => {
        const dataWithType = {
          ...validMapData,
          controls: [
            {
              id: 'test_control',
              name: 'Test Control',
              type: type as any,
            },
          ],
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithType);
        expect(result.success).toBe(true);
      });
    });

    it('should reject invalid control types', () => {
      const dataWithInvalidType = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'invalid_type' as any,
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidType);
      expect(result.success).toBe(false);
    });

    it('should validate CC numbers are in valid range', () => {
      const validCCs = [0, 1, 64, 127];

      validCCs.forEach(cc => {
        const dataWithCC = {
          ...validMapData,
          controls: [
            {
              id: 'test_control',
              name: 'Test Control',
              type: 'encoder' as const,
              cc: cc,
            },
          ],
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithCC);
        expect(result.success).toBe(true);
      });
    });

    it('should reject invalid CC numbers', () => {
      const invalidCCs = [-1, 128, 200];

      invalidCCs.forEach(cc => {
        const dataWithInvalidCC = {
          ...validMapData,
          controls: [
            {
              id: 'test_control',
              name: 'Test Control',
              type: 'encoder' as const,
              cc: cc,
            },
          ],
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidCC);
        expect(result.success).toBe(false);
      });
    });

    it('should accept string or number for channel', () => {
      const dataWithStringChannel = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'encoder' as const,
            channel: 'registry_ref',
          },
        ],
      };

      const dataWithNumberChannel = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'encoder' as const,
            channel: 5,
          },
        ],
      };

      expect(CanonicalMidiMapSchema.safeParse(dataWithStringChannel).success).toBe(true);
      expect(CanonicalMidiMapSchema.safeParse(dataWithNumberChannel).success).toBe(true);
    });

    it('should validate range as array of two numbers', () => {
      const dataWithRange = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'encoder' as const,
            range: [0, 127],
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithRange);
      expect(result.success).toBe(true);
    });

    it('should reject invalid range arrays', () => {
      const invalidRanges = [
        [0], // Too short
        [0, 64, 127], // Too long
        ['0', '127'], // Wrong type
      ];

      invalidRanges.forEach(range => {
        const dataWithInvalidRange = {
          ...validMapData,
          controls: [
            {
              id: 'test_control',
              name: 'Test Control',
              type: 'encoder' as const,
              range: range as any,
            },
          ],
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidRange);
        expect(result.success).toBe(false);
      });
    });

    it('should validate mode enum', () => {
      const validModes = ['toggle', 'momentary'];

      validModes.forEach(mode => {
        const dataWithMode = {
          ...validMapData,
          controls: [
            {
              id: 'test_control',
              name: 'Test Control',
              type: 'button' as const,
              mode: mode as any,
            },
          ],
        };

        const result = CanonicalMidiMapSchema.safeParse(dataWithMode);
        expect(result.success).toBe(true);
      });
    });

    it('should accept string or number for plugin_parameter', () => {
      const dataWithStringParam = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'encoder' as const,
            plugin_parameter: 'filter.cutoff',
          },
        ],
      };

      const dataWithNumberParam = {
        ...validMapData,
        controls: [
          {
            id: 'test_control',
            name: 'Test Control',
            type: 'encoder' as const,
            plugin_parameter: 42,
          },
        ],
      };

      expect(CanonicalMidiMapSchema.safeParse(dataWithStringParam).success).toBe(true);
      expect(CanonicalMidiMapSchema.safeParse(dataWithNumberParam).success).toBe(true);
    });
  });

  describe('button group validation', () => {
    it('should validate button groups with buttons array', () => {
      const dataWithButtonGroup = {
        ...validMapData,
        controls: [
          {
            id: 'transport_group',
            name: 'Transport Controls',
            type: 'button_group' as const,
            buttons: [
              {
                id: 'play',
                name: 'Play',
                cc: 60,
                channel: 1,
                mode: 'momentary' as const,
              },
              {
                id: 'stop',
                name: 'Stop',
                cc: 61,
                channel: 1,
                mode: 'momentary' as const,
                plugin_parameter: 'transport.stop',
              },
            ],
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithButtonGroup);
      expect(result.success).toBe(true);
    });

    it('should validate individual button definitions', () => {
      const dataWithInvalidButton = {
        ...validMapData,
        controls: [
          {
            id: 'transport_group',
            name: 'Transport Controls',
            type: 'button_group' as const,
            buttons: [
              {
                id: 'play',
                name: 'Play',
                cc: 128, // Invalid CC
                channel: 1,
                mode: 'momentary' as const,
              },
            ],
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithInvalidButton);
      expect(result.success).toBe(false);
    });

    it('should require all mandatory button fields', () => {
      const dataWithIncompleteButton = {
        ...validMapData,
        controls: [
          {
            id: 'transport_group',
            name: 'Transport Controls',
            type: 'button_group' as const,
            buttons: [
              {
                id: 'play',
                name: 'Play',
                // Missing cc, channel, mode
              } as any,
            ],
          },
        ],
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithIncompleteButton);
      expect(result.success).toBe(false);
      if (!result.success) {
        expect(result.error.errors.some(e => e.path.includes('cc'))).toBe(true);
        expect(result.error.errors.some(e => e.path.includes('channel'))).toBe(true);
        expect(result.error.errors.some(e => e.path.includes('mode'))).toBe(true);
      }
    });
  });

  describe('edge cases and error handling', () => {
    it('should reject null values', () => {
      const result = CanonicalMidiMapSchema.safeParse(null);
      expect(result.success).toBe(false);
    });

    it('should reject undefined values', () => {
      const result = CanonicalMidiMapSchema.safeParse(undefined);
      expect(result.success).toBe(false);
    });

    it('should reject non-object values', () => {
      const result = CanonicalMidiMapSchema.safeParse('string');
      expect(result.success).toBe(false);
    });

    it('should handle empty objects', () => {
      const result = CanonicalMidiMapSchema.safeParse({});
      expect(result.success).toBe(false);
      if (!result.success) {
        // Should have errors for all required fields
        const errorPaths = result.error.errors.map(e => e.path.join('.'));
        expect(errorPaths).toContain('version');
        expect(errorPaths).toContain('device');
        expect(errorPaths).toContain('metadata');
        expect(errorPaths).toContain('controls');
      }
    });

    it('should handle very long string values', () => {
      const longString = 'a'.repeat(10000);
      const dataWithLongStrings = {
        ...validMapData,
        metadata: {
          name: longString,
          description: longString,
          author: longString,
        },
        device: {
          manufacturer: longString,
          model: longString,
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithLongStrings);
      expect(result.success).toBe(true);
    });

    it('should handle unicode characters', () => {
      const dataWithUnicode = {
        ...validMapData,
        metadata: {
          name: '测试地图 MIDI Map ñiño',
          description: 'Unicode test: 🎵🎹🎛️',
          author: 'Authör Naïve',
        },
        device: {
          manufacturer: 'Tëst Mäñufäctürër',
          model: 'Ünicödé Modël',
        },
      };

      const result = CanonicalMidiMapSchema.safeParse(dataWithUnicode);
      expect(result.success).toBe(true);
    });
  });
});