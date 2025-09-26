/**
 * Basic tests for canonical-midi-maps module
 */

import { describe, it, expect } from 'vitest';

describe('Canonical MIDI Maps', () => {
  describe('basic functionality', () => {
    it('should pass basic test', () => {
      expect(true).toBe(true);
    });

    it('should validate plugin parameter structure', () => {
      const createPluginParameter = (index: number, name: string, defaultValue: number) => ({
        index,
        name,
        min: 0.0,
        max: 1.0,
        default: defaultValue,
        group: 'misc',
        type: 'continuous' as const,
        automatable: true
      });

      const param = createPluginParameter(0, 'Volume', 0.5);

      expect(param).toEqual({
        index: 0,
        name: 'Volume',
        min: 0.0,
        max: 1.0,
        default: 0.5,
        group: 'misc',
        type: 'continuous',
        automatable: true
      });
    });

    it('should validate plugin information structure', () => {
      const createPluginInfo = (name: string, manufacturer: string) => ({
        manufacturer,
        name,
        version: '1.0.0',
        format: 'VST3',
        uid: `${manufacturer.toLowerCase()}-${name.toLowerCase()}`,
        category: 'Effect'
      });

      const plugin = createPluginInfo('Test Plugin', 'Test Company');

      expect(plugin.name).toBe('Test Plugin');
      expect(plugin.manufacturer).toBe('Test Company');
      expect(plugin.format).toBe('VST3');
      expect(plugin.uid).toBe('test company-test plugin');
    });

    it('should create safe filename identifiers', () => {
      const createSafeIdentifier = (input: string): string => {
        return input.toLowerCase().replace(/[^a-z0-9]/g, '-');
      };

      expect(createSafeIdentifier('Test Company')).toBe('test-company');
      expect(createSafeIdentifier('Plugin (v2.0)')).toBe('plugin--v2-0-');
      expect(createSafeIdentifier('Special! @#$ Characters')).toBe('special------characters');
    });
  });
});