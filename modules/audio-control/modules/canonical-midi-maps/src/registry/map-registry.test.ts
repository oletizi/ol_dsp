/**
 * Test suite for map registry functionality
 */

import { describe, expect, it, beforeEach } from 'vitest';
import { CanonicalMapRegistry, defaultRegistry } from '@/registry/map-registry.js';
import type { MapRegistryEntry } from '@/registry/map-registry.js';

describe('CanonicalMapRegistry', () => {
  let registry: CanonicalMapRegistry;

  const createMockEntry = (overrides = {}): MapRegistryEntry => ({
    id: 'test-entry-1',
    filePath: 'test/path.yaml',
    metadata: {
      name: 'Test MIDI Map',
      version: '1.0.0',
      description: 'A test mapping for unit tests',
      author: 'Test Author',
      tags: ['test', 'unit-test', 'mock'],
    },
    controller: {
      manufacturer: 'Test Manufacturer',
      model: 'Test Controller Model',
    },
    plugin: {
      manufacturer: 'Test Plugin Manufacturer',
      name: 'Test Plugin',
      format: 'VST3',
    },
    ...overrides,
  });

  beforeEach(() => {
    registry = new CanonicalMapRegistry();
  });

  describe('register and get', () => {
    it('should register and retrieve a map entry', () => {
      const entry = createMockEntry();
      registry.register(entry);

      const retrieved = registry.get('test-entry-1');
      expect(retrieved).toEqual(entry);
    });

    it('should replace existing entries with same ID', () => {
      const entry1 = createMockEntry({ metadata: { name: 'First Entry', version: '1.0.0' } });
      const entry2 = createMockEntry({ metadata: { name: 'Second Entry', version: '2.0.0' } });

      registry.register(entry1);
      registry.register(entry2);

      const retrieved = registry.get('test-entry-1');
      expect(retrieved?.metadata.name).toBe('Second Entry');
      expect(retrieved?.metadata.version).toBe('2.0.0');
    });

    it('should return undefined for non-existent entries', () => {
      const retrieved = registry.get('non-existent-id');
      expect(retrieved).toBeUndefined();
    });
  });

  describe('unregister', () => {
    it('should remove existing entries', () => {
      const entry = createMockEntry();
      registry.register(entry);

      const removed = registry.unregister('test-entry-1');
      expect(removed).toBe(true);

      const retrieved = registry.get('test-entry-1');
      expect(retrieved).toBeUndefined();
    });

    it('should return false for non-existent entries', () => {
      const removed = registry.unregister('non-existent-id');
      expect(removed).toBe(false);
    });
  });

  describe('getAll', () => {
    it('should return all registered entries', () => {
      const entry1 = createMockEntry({ id: 'entry-1' });
      const entry2 = createMockEntry({ id: 'entry-2' });
      const entry3 = createMockEntry({ id: 'entry-3' });

      registry.register(entry1);
      registry.register(entry2);
      registry.register(entry3);

      const allEntries = registry.getAll();
      expect(allEntries).toHaveLength(3);
      expect(allEntries).toContainEqual(entry1);
      expect(allEntries).toContainEqual(entry2);
      expect(allEntries).toContainEqual(entry3);
    });

    it('should return empty array for empty registry', () => {
      const allEntries = registry.getAll();
      expect(allEntries).toEqual([]);
    });
  });

  describe('findByController', () => {
    beforeEach(() => {
      registry.register(createMockEntry({
        id: 'novation-1',
        controller: { manufacturer: 'Novation', model: 'Launchkey MK3 49' },
      }));
      registry.register(createMockEntry({
        id: 'novation-2',
        controller: { manufacturer: 'Novation', model: 'Launchkey Mini MK3' },
      }));
      registry.register(createMockEntry({
        id: 'akai-1',
        controller: { manufacturer: 'Akai', model: 'MPK Mini MK3' },
      }));
      registry.register(createMockEntry({
        id: 'native-1',
        controller: { manufacturer: 'Native Instruments', model: 'Komplete Kontrol S61' },
      }));
    });

    it('should find entries by manufacturer', () => {
      const novationEntries = registry.findByController('Novation');
      expect(novationEntries).toHaveLength(2);
      expect(novationEntries.every(e => e.controller.manufacturer === 'Novation')).toBe(true);
    });

    it('should find entries by manufacturer and model', () => {
      const launchkeyEntries = registry.findByController('Novation', 'Launchkey MK3');
      expect(launchkeyEntries).toHaveLength(1);
      expect(launchkeyEntries[0].controller.model).toBe('Launchkey MK3 49');
    });

    it('should support case insensitive search', () => {
      const entries = registry.findByController('novation');
      expect(entries).toHaveLength(2);

      const specificEntries = registry.findByController('NOVATION', 'launchkey');
      expect(specificEntries).toHaveLength(2);
    });

    it('should support partial matching', () => {
      const entries = registry.findByController('Nav'); // Partial match for 'Native'
      expect(entries).toHaveLength(1);
      expect(entries[0].id).toBe('native-1');

      const modelEntries = registry.findByController('Novation', 'Mini');
      expect(modelEntries).toHaveLength(1);
      expect(modelEntries[0].id).toBe('novation-2');
    });

    it('should return empty array for no matches', () => {
      const entries = registry.findByController('NonExistent');
      expect(entries).toEqual([]);
    });
  });

  describe('findByPlugin', () => {
    beforeEach(() => {
      registry.register(createMockEntry({
        id: 'massive-x-1',
        plugin: { manufacturer: 'Native Instruments', name: 'Massive X', format: 'VST3' },
      }));
      registry.register(createMockEntry({
        id: 'battery-1',
        plugin: { manufacturer: 'Native Instruments', name: 'Battery 4', format: 'VST3' },
      }));
      registry.register(createMockEntry({
        id: 'pigments-1',
        plugin: { manufacturer: 'Arturia', name: 'Pigments', format: 'VST3' },
      }));
      registry.register(createMockEntry({
        id: 'serum-1',
        plugin: { manufacturer: 'Xfer Records', name: 'Serum', format: 'VST3' },
      }));
    });

    it('should find entries by plugin manufacturer', () => {
      const niEntries = registry.findByPlugin('Native Instruments');
      expect(niEntries).toHaveLength(2);
      expect(niEntries.every(e => e.plugin.manufacturer === 'Native Instruments')).toBe(true);
    });

    it('should find entries by manufacturer and plugin name', () => {
      const massiveEntries = registry.findByPlugin('Native Instruments', 'Massive X');
      expect(massiveEntries).toHaveLength(1);
      expect(massiveEntries[0].plugin.name).toBe('Massive X');
    });

    it('should support case insensitive search', () => {
      const entries = registry.findByPlugin('native instruments');
      expect(entries).toHaveLength(2);

      const specificEntries = registry.findByPlugin('ARTURIA', 'pigments');
      expect(specificEntries).toHaveLength(1);
    });

    it('should support partial matching', () => {
      const entries = registry.findByPlugin('Art'); // Partial match for 'Arturia'
      expect(entries).toHaveLength(1);
      expect(entries[0].id).toBe('pigments-1');

      const nameEntries = registry.findByPlugin('Native Instruments', 'Bat'); // 'Battery'
      expect(nameEntries).toHaveLength(1);
      expect(nameEntries[0].id).toBe('battery-1');
    });

    it('should return empty array for no matches', () => {
      const entries = registry.findByPlugin('NonExistent Plugin Company');
      expect(entries).toEqual([]);
    });
  });

  describe('findByTags', () => {
    beforeEach(() => {
      registry.register(createMockEntry({
        id: 'synth-1',
        metadata: {
          name: 'Synth Map 1',
          version: '1.0.0',
          tags: ['synthesizer', 'novation', 'massive-x'],
        },
      }));
      registry.register(createMockEntry({
        id: 'drum-1',
        metadata: {
          name: 'Drum Map 1',
          version: '1.0.0',
          tags: ['drums', 'percussion', 'battery'],
        },
      }));
      registry.register(createMockEntry({
        id: 'multi-1',
        metadata: {
          name: 'Multi Map 1',
          version: '1.0.0',
          tags: ['synthesizer', 'drums', 'studio'],
        },
      }));
      registry.register(createMockEntry({
        id: 'no-tags',
        metadata: {
          name: 'No Tags Map',
          version: '1.0.0',
          // No tags property
        },
      }));
    });

    it('should find entries by single tag', () => {
      const synthEntries = registry.findByTags(['synthesizer']);
      expect(synthEntries).toHaveLength(2);
      expect(synthEntries.map(e => e.id)).toContain('synth-1');
      expect(synthEntries.map(e => e.id)).toContain('multi-1');
    });

    it('should find entries by multiple tags (OR logic)', () => {
      const entries = registry.findByTags(['drums', 'novation']);
      expect(entries).toHaveLength(3); // drum-1, multi-1, synth-1
      expect(entries.map(e => e.id).sort()).toEqual(['drum-1', 'multi-1', 'synth-1']);
    });

    it('should support case insensitive tag matching', () => {
      const entries = registry.findByTags(['SYNTHESIZER']);
      expect(entries).toHaveLength(2);
    });

    it('should support partial tag matching', () => {
      const entries = registry.findByTags(['synth']);
      expect(entries).toHaveLength(2);
      expect(entries.map(e => e.id)).toContain('synth-1');
      expect(entries.map(e => e.id)).toContain('multi-1');
    });

    it('should return empty array for entries without tags', () => {
      const entries = registry.findByTags(['nonexistent']);
      expect(entries).toEqual([]);
    });

    it('should handle entries with undefined tags', () => {
      registry.register(createMockEntry({
        id: 'undefined-tags',
        metadata: {
          name: 'Undefined Tags Map',
          version: '1.0.0',
          tags: undefined,
        } as any,
      }));

      const entries = registry.findByTags(['any-tag']);
      expect(entries.find(e => e.id === 'undefined-tags')).toBeUndefined();
    });
  });

  describe('search', () => {
    beforeEach(() => {
      registry.register(createMockEntry({
        id: 'massive-novation',
        metadata: {
          name: 'Novation Launchkey → Massive X',
          version: '1.0.0',
          description: 'Professional synthesizer mapping for studio production',
          tags: ['synthesizer', 'novation', 'native-instruments'],
        },
        controller: { manufacturer: 'Novation', model: 'Launchkey MK3 49' },
        plugin: { manufacturer: 'Native Instruments', name: 'Massive X' },
      }));
      registry.register(createMockEntry({
        id: 'pigments-akai',
        metadata: {
          name: 'Akai MPK → Arturia Pigments',
          version: '1.0.0',
          description: 'Compact controller mapping for wavetable synthesis',
          tags: ['synthesizer', 'akai', 'arturia'],
        },
        controller: { manufacturer: 'Akai', model: 'MPK Mini MK3' },
        plugin: { manufacturer: 'Arturia', name: 'Pigments' },
      }));
    });

    it('should search in metadata names', () => {
      const results = registry.search('Novation');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('massive-novation');
    });

    it('should search in descriptions', () => {
      const results = registry.search('wavetable');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('pigments-akai');
    });

    it('should search in controller manufacturers', () => {
      const results = registry.search('Akai');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('pigments-akai');
    });

    it('should search in controller models', () => {
      const results = registry.search('MK3');
      expect(results).toHaveLength(2);
    });

    it('should search in plugin manufacturers', () => {
      const results = registry.search('Native');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('massive-novation');
    });

    it('should search in plugin names', () => {
      const results = registry.search('Massive');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('massive-novation');
    });

    it('should search in tags', () => {
      const results = registry.search('arturia');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('pigments-akai');
    });

    it('should be case insensitive', () => {
      const results = registry.search('MASSIVE');
      expect(results).toHaveLength(1);
      expect(results[0].id).toBe('massive-novation');
    });

    it('should support partial matching', () => {
      const results = registry.search('synth'); // Should find 'synthesizer' tag
      expect(results).toHaveLength(2);
    });

    it('should return empty array for no matches', () => {
      const results = registry.search('completely-nonexistent-term');
      expect(results).toEqual([]);
    });
  });

  describe('getStats', () => {
    beforeEach(() => {
      registry.register(createMockEntry({
        id: 'entry-1',
        metadata: { name: 'Entry 1', version: '1.0.0', tags: ['tag1', 'tag2'] },
        controller: { manufacturer: 'Novation', model: 'Launchkey' },
        plugin: { manufacturer: 'Native Instruments', name: 'Massive', format: 'VST3' },
      }));
      registry.register(createMockEntry({
        id: 'entry-2',
        metadata: { name: 'Entry 2', version: '1.0.0', tags: ['tag2', 'tag3'] },
        controller: { manufacturer: 'Akai', model: 'MPK' },
        plugin: { manufacturer: 'Arturia', name: 'Pigments', format: 'VST3' },
      }));
      registry.register(createMockEntry({
        id: 'entry-3',
        metadata: { name: 'Entry 3', version: '1.0.0', tags: ['tag1'] },
        controller: { manufacturer: 'Novation', model: 'Another' },
        plugin: { manufacturer: 'Native Instruments', name: 'Battery', format: 'AU' },
      }));
    });

    it('should return correct statistics', () => {
      const stats = registry.getStats();

      expect(stats.totalMaps).toBe(3);
      expect(stats.controllerManufacturers).toEqual(['Akai', 'Novation']);
      expect(stats.pluginManufacturers).toEqual(['Arturia', 'Native Instruments']);
      expect(stats.formats).toEqual(['AU', 'VST3']);
      expect(stats.tags).toEqual(['tag1', 'tag2', 'tag3']);
    });

    it('should return empty stats for empty registry', () => {
      const emptyRegistry = new CanonicalMapRegistry();
      const stats = emptyRegistry.getStats();

      expect(stats.totalMaps).toBe(0);
      expect(stats.controllerManufacturers).toEqual([]);
      expect(stats.pluginManufacturers).toEqual([]);
      expect(stats.formats).toEqual([]);
      expect(stats.tags).toEqual([]);
    });

    it('should handle entries without formats or tags', () => {
      const minimalRegistry = new CanonicalMapRegistry();
      minimalRegistry.register(createMockEntry({
        id: 'minimal',
        metadata: { name: 'Minimal', version: '1.0.0' },
        plugin: { manufacturer: 'Test', name: 'Plugin' }, // No format
      }));

      const stats = minimalRegistry.getStats();

      expect(stats.totalMaps).toBe(1);
      expect(stats.formats).toEqual([]);
      expect(stats.tags).toEqual([]);
    });

    it('should deduplicate and sort results', () => {
      const duplicateRegistry = new CanonicalMapRegistry();
      // Add entries with duplicate manufacturers/formats
      duplicateRegistry.register(createMockEntry({
        id: 'dup-1',
        controller: { manufacturer: 'Novation', model: 'Model1' },
        plugin: { manufacturer: 'Native Instruments', name: 'Plugin1', format: 'VST3' },
        metadata: { name: 'Dup1', version: '1.0.0', tags: ['tag1'] },
      }));
      duplicateRegistry.register(createMockEntry({
        id: 'dup-2',
        controller: { manufacturer: 'Novation', model: 'Model2' },
        plugin: { manufacturer: 'Native Instruments', name: 'Plugin2', format: 'VST3' },
        metadata: { name: 'Dup2', version: '1.0.0', tags: ['tag1'] },
      }));

      const stats = duplicateRegistry.getStats();

      expect(stats.controllerManufacturers).toEqual(['Novation']);
      expect(stats.pluginManufacturers).toEqual(['Native Instruments']);
      expect(stats.formats).toEqual(['VST3']);
      expect(stats.tags).toEqual(['tag1']);
    });
  });

  describe('defaultRegistry', () => {
    it('should be pre-populated with built-in maps', () => {
      const allMaps = defaultRegistry.getAll();
      expect(allMaps.length).toBeGreaterThan(0);
    });

    it('should include expected built-in maps', () => {
      const novationMap = defaultRegistry.get('novation-launchkey-mk3-massive-x');
      expect(novationMap).toBeDefined();
      expect(novationMap?.controller.manufacturer).toBe('Novation');
      expect(novationMap?.plugin.name).toBe('Massive X');

      const akaiMap = defaultRegistry.get('akai-mpk-mini-arturia-pigments');
      expect(akaiMap).toBeDefined();
      expect(akaiMap?.controller.manufacturer).toBe('Akai');
      expect(akaiMap?.plugin.name).toBe('Pigments');
    });

    it('should allow additional registrations', () => {
      const initialCount = defaultRegistry.getAll().length;

      const customEntry = createMockEntry({ id: 'custom-test-entry' });
      defaultRegistry.register(customEntry);

      expect(defaultRegistry.getAll().length).toBe(initialCount + 1);
      expect(defaultRegistry.get('custom-test-entry')).toEqual(customEntry);

      // Clean up
      defaultRegistry.unregister('custom-test-entry');
    });
  });

  describe('edge cases and error handling', () => {
    it('should handle entries with empty or null fields gracefully', () => {
      registry.register(createMockEntry({
        id: 'edge-case',
        metadata: {
          name: '',
          version: '1.0.0',
          description: undefined,
          author: null as any,
          tags: [],
        },
        controller: { manufacturer: '', model: '' },
        plugin: { manufacturer: '', name: '', format: undefined },
      }));

      // Should still be retrievable
      const entry = registry.get('edge-case');
      expect(entry).toBeDefined();

      // Search should handle empty fields
      const results = registry.search('');
      expect(results).toHaveLength(1);

      const stats = registry.getStats();
      expect(stats.totalMaps).toBe(1);
    });

    it('should handle unicode and special characters', () => {
      registry.register(createMockEntry({
        id: 'unicode-test',
        metadata: {
          name: 'Test Map 测试 ñiño',
          version: '1.0.0',
          tags: ['测试', 'español'],
        },
        controller: { manufacturer: 'Tëst Mäñufäctürër', model: 'Spëciál Modël' },
        plugin: { manufacturer: 'Ünicödé Plügïns', name: 'Spéciål Sÿnth' },
      }));

      const results = registry.search('测试');
      expect(results).toHaveLength(1);

      const tagResults = registry.findByTags(['español']);
      expect(tagResults).toHaveLength(1);
    });

    it('should handle very long field values', () => {
      const longString = 'a'.repeat(1000);
      registry.register(createMockEntry({
        id: 'long-test',
        metadata: {
          name: longString,
          version: '1.0.0',
          description: longString,
        },
      }));

      const results = registry.search('aaa');
      expect(results).toHaveLength(1);
    });
  });
});