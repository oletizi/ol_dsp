/**
 * Tests for plugin specification generation
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { writeFileSync } from 'fs';
import { join } from 'path';
import {
  SpecGenerator,
  createSpecGenerator
} from '@/tools/plugin-generator/spec-generator.js';
import type {
  PluginInfo,
  PluginParameter,
  GeneratorArgs,
  PluginSpec
} from '@/tools/plugin-generator/types.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';
import type { IParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

// Mock fs operations
vi.mock('fs', () => ({
  writeFileSync: vi.fn()
}));

// Mock console methods
const mockConsoleLog = vi.spyOn(console, 'log').mockImplementation(() => {});
const mockConsoleError = vi.spyOn(console, 'error').mockImplementation(() => {});

describe('SpecGenerator', () => {
  let mockProcessManager: PlughostProcessManager;
  let mockParameterCategorizer: IParameterCategorizer;
  let mockArgs: GeneratorArgs;
  let generator: SpecGenerator;

  const mockPlugin: PluginInfo = {
    manufacturer: 'Test Company',
    name: 'Test Plugin',
    version: '1.0.0',
    format: 'VST3',
    uid: 'test-uid-123',
    category: 'Instrument'
  };

  beforeEach(() => {
    vi.clearAllMocks();

    // Reset Date.now to consistent value
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2024-01-01T00:00:00.000Z'));

    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn().mockReturnValue(false)
    } as any;

    mockParameterCategorizer = {
      categorizeParameter: vi.fn().mockImplementation((name: string) => {
        if (name.includes('gain')) return 'master';
        if (name.includes('cutoff')) return 'filter';
        return 'misc';
      })
    };

    mockArgs = {
      format: undefined,
      quick: false,
      help: false
    };

    generator = new SpecGenerator({
      processManager: mockProcessManager,
      parameterCategorizer: mockParameterCategorizer,
      args: mockArgs
    });
  });

  describe('generatePluginSpec', () => {
    const mockParameterData: PluginParameter[] = [
      {
        index: 0,
        name: 'Master Gain',
        label: 'Gain',
        text: '0.5',
        default_value: 0.5,
        current_value: 0.5,
        automatable: true,
        meta_parameter: false,
        discrete: false
      },
      {
        index: 1,
        name: 'Filter Cutoff',
        label: 'Cutoff',
        text: '1000 Hz',
        default_value: 0.7,
        current_value: 0.7,
        automatable: true,
        meta_parameter: false,
        discrete: false
      },
      {
        index: 2,
        name: 'Enable',
        label: 'Enable',
        text: 'On',
        default_value: 1.0,
        current_value: 1.0,
        automatable: false,
        meta_parameter: false,
        discrete: true
      }
    ];

    it('should generate complete plugin specification', async () => {
      const mockOutput = JSON.stringify({ parameters: mockParameterData });
      (mockProcessManager.runPlughost as any).mockResolvedValue(`Some text\n${mockOutput}\nMore text`);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).not.toBeNull();
      expect(spec!.plugin).toEqual({
        manufacturer: 'Test Company',
        name: 'Test Plugin',
        version: '1.0.0',
        format: 'VST3',
        uid: 'test-uid-123'
      });

      expect(spec!.metadata).toEqual({
        version: '1.0.0',
        created: '2024-01-01T00:00:00.000Z',
        author: 'plughost-generator',
        description: 'Auto-generated parameter descriptor for Test Plugin',
        parameter_count: 3,
        tags: ['auto-generated', 'instrument', 'test-company']
      });

      expect(spec!.parameters).toHaveLength(3);
      expect(spec!.parameters[0]).toEqual({
        index: 0,
        name: 'Master Gain',
        label: 'Gain',
        min: 0.0,
        max: 1.0,
        default: 0.5,
        group: 'master',
        type: 'continuous',
        automatable: true
      });

      expect(spec!.parameters[2]).toEqual({
        index: 2,
        name: 'Enable',
        label: 'Enable',
        min: 0,
        max: 1,
        default: 1.0,
        group: 'misc',
        type: 'discrete',
        automatable: false
      });
    });

    it('should generate parameter groups correctly', async () => {
      const mockOutput = JSON.stringify({ parameters: mockParameterData });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec!.groups).toEqual({
        master: {
          name: 'Master',
          parameters: [0]
        },
        filter: {
          name: 'Filter',
          parameters: [1]
        },
        misc: {
          name: 'Misc',
          parameters: [2]
        }
      });
    });

    it('should skip plugins with no parameters', async () => {
      const mockOutput = JSON.stringify({ parameters: [] });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockConsoleLog).toHaveBeenCalledWith('⚠️  No parameters found for Test Plugin');
    });

    it('should skip problematic plugins in quick mode', async () => {
      mockArgs.quick = true;
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(true);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockProcessManager.runPlughost).not.toHaveBeenCalled();
    });

    it('should handle plugins without JSON output', async () => {
      (mockProcessManager.runPlughost as any).mockResolvedValue('No JSON here');

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockConsoleLog).toHaveBeenCalledWith('⚠️  No parameter data for Test Plugin');
    });

    it('should handle invalid JSON', async () => {
      (mockProcessManager.runPlughost as any).mockResolvedValue('{ invalid json }');

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockConsoleError).toHaveBeenCalledWith(
        '❌ Failed to generate spec for Test Plugin:',
        expect.any(String)
      );
    });

    it('should handle missing parameters array in JSON', async () => {
      const mockOutput = JSON.stringify({ no_parameters: 'here' });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockConsoleLog).toHaveBeenCalledWith('⚠️  No parameters found for Test Plugin');
    });

    it('should use shorter timeout for problematic plugins', async () => {
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(true);
      const mockOutput = JSON.stringify({ parameters: mockParameterData });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await generator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'Test Plugin', '--json'],
        20000
      );
    });

    it('should use normal timeout for safe plugins', async () => {
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(false);
      const mockOutput = JSON.stringify({ parameters: mockParameterData });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await generator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'Test Plugin', '--json'],
        45000
      );
    });

    it('should add quick-scan flag when requested', async () => {
      mockArgs.quick = true;
      (mockProcessManager.isProblematicPlugin as any).mockReturnValue(false);
      const mockOutput = JSON.stringify({ parameters: mockParameterData });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await generator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--interrogate', 'Test Plugin', '--json', '--quick-scan'],
        45000
      );
    });

    it('should handle parameters without labels', async () => {
      const mockParameterNoLabel: PluginParameter = {
        index: 0,
        name: 'Test Parameter',
        label: 'Test Parameter', // Same as name
        text: '0.5',
        default_value: 0.5,
        current_value: 0.5,
        automatable: true,
        meta_parameter: false,
        discrete: false
      };

      const mockOutput = JSON.stringify({ parameters: [mockParameterNoLabel] });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec!.parameters[0]).not.toHaveProperty('label');
    });

    it('should preserve distinct labels', async () => {
      const mockParameterWithLabel: PluginParameter = {
        index: 0,
        name: 'parameter_name',
        label: 'Parameter Label',
        text: '0.5',
        default_value: 0.5,
        current_value: 0.5,
        automatable: true,
        meta_parameter: false,
        discrete: false
      };

      const mockOutput = JSON.stringify({ parameters: [mockParameterWithLabel] });
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const spec = await generator.generatePluginSpec(mockPlugin);

      expect(spec!.parameters[0]).toHaveProperty('label', 'Parameter Label');
    });
  });

  describe('savePluginSpec', () => {
    it('should save plugin spec to file', () => {
      const mockSpec: PluginSpec = {
        plugin: mockPlugin,
        metadata: {
          version: '1.0.0',
          created: '2024-01-01T00:00:00.000Z',
          author: 'test',
          description: 'test',
          parameter_count: 0,
          tags: []
        },
        parameters: [],
        groups: {}
      };

      const filepath = generator.savePluginSpec(mockPlugin, mockSpec, '/test/output');

      expect(filepath).toBe('/test/output/test-company-test-plugin.json');
      expect(writeFileSync).toHaveBeenCalledWith(
        '/test/output/test-company-test-plugin.json',
        JSON.stringify(mockSpec, null, 2)
      );
    });

    it('should create safe filenames', () => {
      const pluginWithSpecialChars: PluginInfo = {
        ...mockPlugin,
        manufacturer: 'Special! @#$ Company',
        name: 'Plugin (Version 2.0) [Beta]'
      };

      const mockSpec = {} as PluginSpec;

      const filepath = generator.savePluginSpec(pluginWithSpecialChars, mockSpec, '/test');

      expect(filepath).toBe('/test/special------company-plugin--version-2-0---beta-.json');
    });
  });

  describe('createSpecGenerator factory', () => {
    it('should create SpecGenerator instance', () => {
      const options = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs
      };

      const generator = createSpecGenerator(options);

      expect(generator).toBeInstanceOf(SpecGenerator);
      expect(typeof generator.generatePluginSpec).toBe('function');
      expect(typeof generator.savePluginSpec).toBe('function');
    });
  });
});