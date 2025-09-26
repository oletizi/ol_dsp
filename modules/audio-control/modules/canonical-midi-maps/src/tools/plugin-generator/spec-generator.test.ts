/**
 * Test suite for plugin specification generator
 */

import { describe, expect, it, beforeEach, vi } from 'vitest';
import { writeFileSync } from 'fs';
import { join } from 'path';
import { SpecGenerator, createSpecGenerator } from '@/tools/plugin-generator/spec-generator.js';
import type {
  PluginInfo,
  PluginParameter,
  GeneratorArgs,
  ProcessedParameter,
  ParameterGroup
} from '@/tools/plugin-generator/types.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';
import type { IParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

// Mock fs module
vi.mock('fs', () => ({
  writeFileSync: vi.fn(),
}));

describe('SpecGenerator', () => {
  let specGenerator: SpecGenerator;
  let mockProcessManager: PlughostProcessManager;
  let mockParameterCategorizer: IParameterCategorizer;
  let mockArgs: GeneratorArgs;

  const createMockPluginInfo = (overrides = {}): PluginInfo => ({
    manufacturer: 'Test Manufacturer',
    name: 'Test Plugin',
    version: '1.0.0',
    format: 'VST3',
    uid: 'test-uid-123',
    category: 'Instrument',
    ...overrides,
  });

  const createMockPluginParameter = (overrides = {}): PluginParameter => ({
    index: 0,
    name: 'Test Parameter',
    label: 'Test Label',
    text: 'Test Text',
    default_value: 0.5,
    current_value: 0.5,
    automatable: true,
    meta_parameter: false,
    discrete: false,
    ...overrides,
  });

  beforeEach(() => {
    // Mock process manager
    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn().mockReturnValue(false),
    } as any;

    // Mock parameter categorizer
    mockParameterCategorizer = {
      categorizeParameter: vi.fn().mockReturnValue('misc'),
    };

    mockArgs = {
      format: undefined,
      quick: false,
      help: false,
    };

    specGenerator = new SpecGenerator({
      processManager: mockProcessManager,
      parameterCategorizer: mockParameterCategorizer,
      args: mockArgs,
    });

    // Clear all mocks
    vi.clearAllMocks();
  });

  describe('generatePluginSpec', () => {
    it('should generate a complete plugin specification', async () => {
      const plugin = createMockPluginInfo();
      const mockParameters = [
        createMockPluginParameter({ index: 0, name: 'Volume' }),
        createMockPluginParameter({ index: 1, name: 'Cutoff', discrete: true }),
      ];

      // Mock plughost response
      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(`Some output\n${mockOutput}\nMore output`);

      // Mock categorizer responses
      (mockParameterCategorizer.categorizeParameter as any)
        .mockReturnValueOnce('master')
        .mockReturnValueOnce('filter');

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).not.toBeNull();
      expect(spec!.plugin).toEqual({
        manufacturer: 'Test Manufacturer',
        name: 'Test Plugin',
        version: '1.0.0',
        format: 'VST3',
        uid: 'test-uid-123',
      });

      expect(spec!.metadata).toMatchObject({
        version: '1.0.0',
        author: 'plughost-generator',
        description: 'Auto-generated parameter descriptor for Test Plugin',
        parameter_count: 2,
        tags: ['auto-generated', 'instrument', 'test-manufacturer'],
      });

      expect(spec!.parameters).toHaveLength(2);
      expect(spec!.parameters[0]).toMatchObject({
        index: 0,
        name: 'Volume',
        min: 0,
        max: 1,
        default: 0.5,
        group: 'master',
        type: 'continuous',
        automatable: true,
      });

      expect(spec!.parameters[1]).toMatchObject({
        index: 1,
        name: 'Cutoff',
        min: 0,
        max: 1,
        default: 0.5,
        group: 'filter',
        type: 'discrete',
        automatable: true,
      });

      expect(spec!.groups).toHaveProperty('master');
      expect(spec!.groups).toHaveProperty('filter');
      expect(spec!.groups.master.parameters).toContain(0);
      expect(spec!.groups.filter.parameters).toContain(1);
    });

    it('should skip problematic plugins in quick mode', async () => {
      const plugin = createMockPluginInfo({ name: 'ZamVerb' });
      mockArgs.quick = true;
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(true);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
      expect(mockProcessManager.runPlughost).not.toHaveBeenCalled();
    });

    it('should process problematic plugins if not in quick mode', async () => {
      const plugin = createMockPluginInfo({ name: 'ZamVerb' });
      mockArgs.quick = false;
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(true);

      const mockParameters = [createMockPluginParameter()];
      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).not.toBeNull();
      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'ZamVerb', '--json'],
        20000 // Shorter timeout for problematic plugins
      );
    });

    it('should handle plugins with no parameters', async () => {
      const plugin = createMockPluginInfo();
      const mockOutput = JSON.stringify({ parameters: [] });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should handle invalid JSON responses', async () => {
      const plugin = createMockPluginInfo();
      (mockProcessManager.runPlughost as any).mockResolvedValue('Invalid output without JSON');

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should handle process manager errors', async () => {
      const plugin = createMockPluginInfo();
      (mockProcessManager.runPlughost as any).mockRejectedValue(new Error('Process failed'));

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should use appropriate timeout based on plugin type', async () => {
      const normalPlugin = createMockPluginInfo({ name: 'Normal Plugin' });
      const problematicPlugin = createMockPluginInfo({ name: 'ZamVerb' });

      (mockProcessManager.isProblematicPlugin as any).mockImplementation((name: string) =>
        name.includes('Zam')
      );

      const mockParameters = [createMockPluginParameter()];
      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      // Normal plugin should use longer timeout
      await specGenerator.generatePluginSpec(normalPlugin);
      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'Normal Plugin', '--json'],
        45000
      );

      // Reset mock
      vi.clearAllMocks();
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      // Problematic plugin should use shorter timeout
      await specGenerator.generatePluginSpec(problematicPlugin);
      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'ZamVerb', '--json'],
        20000
      );
    });

    it('should include quick-scan argument when in quick mode', async () => {
      const plugin = createMockPluginInfo();
      mockArgs.quick = true;

      const mockParameters = [createMockPluginParameter()];
      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await specGenerator.generatePluginSpec(plugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'Test Plugin', '--json', '--quick-scan'],
        45000
      );
    });
  });

  describe('parameter processing', () => {
    it('should handle discrete vs continuous parameters', async () => {
      const plugin = createMockPluginInfo();
      const mockParameters = [
        createMockPluginParameter({ index: 0, name: 'Continuous', discrete: false }),
        createMockPluginParameter({ index: 1, name: 'Discrete', discrete: true }),
      ];

      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec!.parameters[0].type).toBe('continuous');
      expect(spec!.parameters[1].type).toBe('discrete');
    });

    it('should include label when different from name', async () => {
      const plugin = createMockPluginInfo();
      const mockParameters = [
        createMockPluginParameter({
          index: 0,
          name: 'param1',
          label: 'Parameter 1'
        }),
        createMockPluginParameter({
          index: 1,
          name: 'param2',
          label: 'param2' // Same as name
        }),
      ];

      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec!.parameters[0]).toHaveProperty('label', 'Parameter 1');
      expect(spec!.parameters[1]).not.toHaveProperty('label');
    });

    it('should group parameters correctly', async () => {
      const plugin = createMockPluginInfo();
      const mockParameters = [
        createMockPluginParameter({ index: 0, name: 'Volume' }),
        createMockPluginParameter({ index: 1, name: 'Cutoff' }),
        createMockPluginParameter({ index: 2, name: 'Attack' }),
      ];

      (mockParameterCategorizer.categorizeParameter as any)
        .mockReturnValueOnce('master')
        .mockReturnValueOnce('filter')
        .mockReturnValueOnce('envelope');

      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec!.groups).toHaveProperty('master');
      expect(spec!.groups).toHaveProperty('filter');
      expect(spec!.groups).toHaveProperty('envelope');

      expect(spec!.groups.master.name).toBe('Master');
      expect(spec!.groups.master.parameters).toEqual([0]);
      expect(spec!.groups.filter.name).toBe('Filter');
      expect(spec!.groups.filter.parameters).toEqual([1]);
      expect(spec!.groups.envelope.name).toBe('Envelope');
      expect(spec!.groups.envelope.parameters).toEqual([2]);
    });
  });

  describe('savePluginSpec', () => {
    it('should save plugin specification to file', () => {
      const plugin = createMockPluginInfo({
        manufacturer: 'Test & Co.',
        name: 'Plugin Name!'
      });
      const spec = {
        plugin: plugin,
        metadata: { version: '1.0.0' },
        parameters: [],
        groups: {},
      } as any;

      const filePath = specGenerator.savePluginSpec(plugin, spec, '/output');

      expect(filePath).toBe('/output/test---co--plugin-name-.json');
      expect(writeFileSync).toHaveBeenCalledWith(
        '/output/test---co--plugin-name-.json',
        JSON.stringify(spec, null, 2)
      );
    });

    it('should create safe identifiers from manufacturer and plugin names', () => {
      const plugin = createMockPluginInfo({
        manufacturer: 'Native Instruments',
        name: 'Massive X (VST3)'
      });
      const spec = {} as any;

      const filePath = specGenerator.savePluginSpec(plugin, spec, '/output');

      expect(filePath).toBe('/output/native-instruments-massive-x--vst3-.json');
    });
  });

  describe('factory function', () => {
    it('should create a functional spec generator', () => {
      const options = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs,
      };

      const generator = createSpecGenerator(options);

      expect(generator).toBeDefined();
      expect(generator).toBeInstanceOf(SpecGenerator);
    });
  });

  describe('edge cases and error handling', () => {
    it('should handle malformed JSON in plughost output', async () => {
      const plugin = createMockPluginInfo();
      (mockProcessManager.runPlughost as any).mockResolvedValue('{"parameters": [malformed json}');

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should handle missing parameters array in JSON', async () => {
      const plugin = createMockPluginInfo();
      const mockOutput = JSON.stringify({ not_parameters: [] });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should handle empty manufacturer and plugin names', async () => {
      const plugin = createMockPluginInfo({ manufacturer: '', name: '' });
      const mockParameters = [createMockPluginParameter()];
      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec).not.toBeNull();
      expect(spec!.metadata.tags).toContain('');
    });

    it('should handle parameter categorization consistently', async () => {
      const plugin = createMockPluginInfo();
      const mockParameters = [
        createMockPluginParameter({ index: 0, name: 'Parameter 1' }),
        createMockPluginParameter({ index: 1, name: 'Parameter 2' }),
      ];

      // Mock categorizer to return same category for both
      (mockParameterCategorizer.categorizeParameter as any).mockReturnValue('test-category');

      const mockOutput = JSON.stringify({ parameters: mockParameters });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await specGenerator.generatePluginSpec(plugin);

      expect(spec!.groups['test-category'].parameters).toEqual([0, 1]);
    });
  });
});