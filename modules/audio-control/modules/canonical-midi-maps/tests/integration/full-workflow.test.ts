/**
 * Integration tests for the complete canonical MIDI maps workflow
 */

import { describe, expect, it } from 'vitest';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';
import { CanonicalMapRegistry } from '@/registry/map-registry.js';
import { createParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

describe('Full Workflow Integration', () => {
  describe('YAML to Registry workflow', () => {
    it('should parse YAML and register in registry', () => {
      const yamlContent = `
version: "1.0.0"
device:
  manufacturer: "Novation"
  model: "Launchkey MK3 49"
metadata:
  name: "Novation Launchkey MK3 → Test Plugin"
  description: "Integration test mapping"
  author: "Test Suite"
  tags:
    - integration
    - test
    - novation
plugin:
  manufacturer: "Test Plugin Co"
  name: "Test Synthesizer"
  format: "VST3"
midi_channel: 1
controls:
  - id: "filter_cutoff"
    name: "Filter Cutoff"
    type: "encoder"
    cc: 20
    channel: 1
    range: [0, 127]
    description: "Controls the low-pass filter cutoff frequency"
    plugin_parameter: "filter.cutoff"
  - id: "envelope_attack"
    name: "Attack"
    type: "encoder"
    cc: 21
    channel: 1
    plugin_parameter: "env.attack"
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
        mode: "momentary"
`;

      // Parse the YAML
      const parseResult = CanonicalMapParser.parseFromYAML(yamlContent);
      expect(parseResult.validation.valid).toBe(true);
      expect(parseResult.map).toBeDefined();

      const map = parseResult.map!;

      // Verify parsed structure
      expect(map.device.manufacturer).toBe('Novation');
      expect(map.controls).toHaveLength(3);
      expect(map.controls[2].buttons).toHaveLength(2);

      // Register in registry
      const registry = new CanonicalMapRegistry();
      registry.register({
        id: 'integration-test-map',
        filePath: 'test/integration-test.yaml',
        metadata: {
          name: map.metadata.name,
          version: map.version,
          description: map.metadata.description,
          author: map.metadata.author,
          tags: map.metadata.tags,
        },
        controller: {
          manufacturer: map.device.manufacturer,
          model: map.device.model,
        },
        plugin: {
          manufacturer: map.plugin!.manufacturer,
          name: map.plugin!.name,
          format: map.plugin!.format,
        },
      });

      // Verify registry functionality
      const retrievedEntry = registry.get('integration-test-map');
      expect(retrievedEntry).toBeDefined();
      expect(retrievedEntry!.controller.manufacturer).toBe('Novation');

      // Test search functionality
      const searchResults = registry.search('novation');
      expect(searchResults).toHaveLength(1);
      expect(searchResults[0].id).toBe('integration-test-map');

      // Test tag-based search
      const tagResults = registry.findByTags(['integration']);
      expect(tagResults).toHaveLength(1);

      // Test plugin search
      const pluginResults = registry.findByPlugin('Test Plugin Co');
      expect(pluginResults).toHaveLength(1);
    });
  });

  describe('Parameter categorization workflow', () => {
    it('should categorize parameters from parsed controls', () => {
      const yamlContent = `
version: "1.0.0"
device:
  manufacturer: "Test"
  model: "Controller"
metadata:
  name: "Parameter Test Map"
controls:
  - id: "master_volume"
    name: "Master Volume"
    type: "slider"
    cc: 7
  - id: "filter_cutoff"
    name: "Filter Cutoff"
    type: "encoder"
    cc: 20
  - id: "env_attack"
    name: "Envelope Attack"
    type: "encoder"
    cc: 21
  - id: "lfo_rate"
    name: "LFO Rate"
    type: "encoder"
    cc: 22
  - id: "chorus_depth"
    name: "Chorus Depth"
    type: "encoder"
    cc: 23
  - id: "amp_gain"
    name: "Amplifier Gain"
    type: "encoder"
    cc: 24
  - id: "osc_wave"
    name: "Oscillator Wave"
    type: "encoder"
    cc: 25
`;

      const parseResult = CanonicalMapParser.parseFromYAML(yamlContent);
      expect(parseResult.validation.valid).toBe(true);

      const categorizer = createParameterCategorizer();
      const map = parseResult.map!;

      // Test categorization of different parameter types
      const categorizations = map.controls.map(control => ({
        id: control.id,
        name: control.name,
        category: categorizer.categorizeParameter(control.name),
      }));

      expect(categorizations.find(c => c.id === 'master_volume')?.category).toBe('master');
      expect(categorizations.find(c => c.id === 'filter_cutoff')?.category).toBe('filter');
      expect(categorizations.find(c => c.id === 'env_attack')?.category).toBe('envelope');
      expect(categorizations.find(c => c.id === 'lfo_rate')?.category).toBe('lfo');
      expect(categorizations.find(c => c.id === 'chorus_depth')?.category).toBe('effects');
      expect(categorizations.find(c => c.id === 'amp_gain')?.category).toBe('amplifier');
      expect(categorizations.find(c => c.id === 'osc_wave')?.category).toBe('oscillator');
    });
  });

  describe('Round-trip serialization', () => {
    it('should maintain data integrity through YAML round-trip', () => {
      const originalYaml = `
version: "1.0.0"
device:
  manufacturer: "Akai"
  model: "MPK Mini MK3"
  firmware: "1.0.5"
metadata:
  name: "Round-trip Test Map"
  description: "Testing serialization consistency"
  author: "Integration Test"
  date: "2024-01-15"
  tags:
    - test
    - roundtrip
plugin:
  manufacturer: "Arturia"
  name: "Pigments"
  version: "4.0.1"
  format: "VST3"
  description: "Wavetable synthesizer"
midi_channel: 2
midi_channel_registry: "channels.yaml"
controls:
  - id: "volume"
    name: "Volume"
    type: "slider"
    cc: 7
    channel: "main_channel"
    range: [0, 127]
    description: "Master volume control"
    plugin_parameter: "master.volume"
`;

      // Parse original YAML
      const parseResult = CanonicalMapParser.parseFromYAML(originalYaml.trim());
      expect(parseResult.validation.valid).toBe(true);

      const map = parseResult.map!;

      // Serialize back to YAML
      const serializedYaml = CanonicalMapParser.serializeToYAML(map);

      // Parse the serialized YAML
      const reparsedResult = CanonicalMapParser.parseFromYAML(serializedYaml);
      expect(reparsedResult.validation.valid).toBe(true);

      const reparsedMap = reparsedResult.map!;

      // Verify key data integrity
      expect(reparsedMap.version).toBe(map.version);
      expect(reparsedMap.device).toEqual(map.device);
      expect(reparsedMap.metadata).toEqual(map.metadata);
      expect(reparsedMap.plugin).toEqual(map.plugin);
      expect(reparsedMap.midi_channel).toBe(map.midi_channel);
      expect(reparsedMap.midi_channel_registry).toBe(map.midi_channel_registry);
      expect(reparsedMap.controls).toEqual(map.controls);
    });

    it('should maintain data integrity through JSON round-trip', () => {
      const originalData = {
        version: '1.0.0',
        device: {
          manufacturer: 'Native Instruments',
          model: 'Komplete Kontrol S61',
        },
        metadata: {
          name: 'JSON Round-trip Test',
          description: 'Testing JSON serialization',
          author: 'Test Suite',
          tags: ['json', 'test'],
        },
        plugin: {
          manufacturer: 'Native Instruments',
          name: 'Massive X',
          format: 'VST3' as const,
        },
        controls: [
          {
            id: 'mod_wheel',
            name: 'Modulation Wheel',
            type: 'encoder' as const,
            cc: 1,
            channel: 1,
            range: [0, 127],
          },
        ],
      };

      // Serialize to JSON
      const jsonString = CanonicalMapParser.serializeToJSON(originalData);

      // Parse the JSON
      const parseResult = CanonicalMapParser.parseFromJSON(jsonString);
      expect(parseResult.validation.valid).toBe(true);

      const parsedMap = parseResult.map!;

      // Verify data integrity
      expect(parsedMap).toEqual(originalData);
    });
  });

  describe('Error handling integration', () => {
    it('should provide comprehensive error reporting', () => {
      const invalidYaml = `
version: "1.0.0"
device:
  manufacturer: "Test"
  # Missing model field
metadata:
  # Missing name field
plugin:
  manufacturer: "Test"
  name: "Test Plugin"
  format: "INVALID_FORMAT"
midi_channel: 17  # Invalid channel
controls:
  - id: "bad_control"
    name: "Bad Control"
    type: "invalid_type"
    cc: 128  # Invalid CC
    channel: 0  # Invalid channel if it were a number
`;

      const parseResult = CanonicalMapParser.parseFromYAML(invalidYaml);

      expect(parseResult.validation.valid).toBe(false);
      expect(parseResult.validation.errors.length).toBeGreaterThan(0);
      expect(parseResult.map).toBeUndefined();

      // Check for specific error types
      const errorMessages = parseResult.validation.errors.map(e => e.message.toLowerCase());
      const hasDeviceError = errorMessages.some(msg => msg.includes('model') || msg.includes('device'));
      const hasMetadataError = errorMessages.some(msg => msg.includes('name') || msg.includes('metadata'));
      const hasFormatError = errorMessages.some(msg => msg.includes('format'));
      const hasChannelError = errorMessages.some(msg => msg.includes('channel') || msg.includes('16'));
      const hasTypeError = errorMessages.some(msg => msg.includes('type'));
      const hasCCError = errorMessages.some(msg => msg.includes('cc') || msg.includes('127'));

      expect(hasDeviceError || hasMetadataError || hasFormatError || hasChannelError || hasTypeError || hasCCError)
        .toBe(true);
    });
  });

  describe('Performance and scalability', () => {
    it('should handle maps with many controls efficiently', () => {
      // Create a map with 100 controls
      const controls = Array.from({ length: 100 }, (_, i) => ({
        id: `control_${i}`,
        name: `Control ${i}`,
        type: 'encoder' as const,
        cc: i % 128, // Cycle through CC numbers
        channel: (i % 16) + 1, // Cycle through channels
        plugin_parameter: `param_${i}`,
      }));

      const largeMapData = {
        version: '1.0.0',
        device: {
          manufacturer: 'Test',
          model: 'Large Controller',
        },
        metadata: {
          name: 'Large Map Test',
          description: 'Testing performance with many controls',
        },
        controls,
      };

      const startTime = Date.now();

      // Validate the large map
      const validationResult = CanonicalMapParser.validate(largeMapData);
      expect(validationResult.valid).toBe(true);

      // Serialize and parse
      const yamlString = CanonicalMapParser.serializeToYAML(largeMapData as any);
      const parseResult = CanonicalMapParser.parseFromYAML(yamlString);

      const endTime = Date.now();
      const processingTime = endTime - startTime;

      expect(parseResult.validation.valid).toBe(true);
      expect(parseResult.map!.controls).toHaveLength(100);

      // Performance assertion - should complete within reasonable time
      expect(processingTime).toBeLessThan(1000); // 1 second max
    });

    it('should handle registry with many entries efficiently', () => {
      const registry = new CanonicalMapRegistry();

      // Add 100 entries
      const entries = Array.from({ length: 100 }, (_, i) => ({
        id: `map_${i}`,
        filePath: `maps/map_${i}.yaml`,
        metadata: {
          name: `Map ${i}`,
          version: '1.0.0',
          tags: [`tag_${i % 10}`, 'common_tag'], // Some overlap in tags
        },
        controller: {
          manufacturer: `Manufacturer ${i % 5}`, // Some overlap in manufacturers
          model: `Model ${i}`,
        },
        plugin: {
          manufacturer: `Plugin Co ${i % 3}`, // Some overlap in plugin manufacturers
          name: `Plugin ${i}`,
        },
      }));

      const startTime = Date.now();

      entries.forEach(entry => registry.register(entry));

      // Test search performance
      const searchResults = registry.search('tag_5');
      const controllerResults = registry.findByController('Manufacturer 2');
      const pluginResults = registry.findByPlugin('Plugin Co 1');
      const tagResults = registry.findByTags(['common_tag']);
      const stats = registry.getStats();

      const endTime = Date.now();
      const processingTime = endTime - startTime;

      // Verify results are correct
      expect(registry.getAll()).toHaveLength(100);
      expect(searchResults.length).toBeGreaterThan(0);
      expect(controllerResults.length).toBeGreaterThan(0);
      expect(pluginResults.length).toBeGreaterThan(0);
      expect(tagResults).toHaveLength(100); // All entries have 'common_tag'
      expect(stats.totalMaps).toBe(100);

      // Performance assertion
      expect(processingTime).toBeLessThan(100); // 100ms max for all operations
    });
  });
});