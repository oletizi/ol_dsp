/**
 * Tests for plugin specification generation functionality
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { SpecGenerator, createSpecGenerator } from '@/tools/plugin-generator/spec-generator.js';
import type { ISpecGenerator, SpecGeneratorOptions } from '@/tools/plugin-generator/spec-generator.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';
import type { IParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';
import type { GeneratorArgs, PluginInfo, PluginParameter } from '@/tools/plugin-generator/types.js';
import { writeFileSync } from 'fs';

// Mock fs
vi.mock('fs', () => ({
  writeFileSync: vi.fn(),
}));

describe('SpecGenerator', () => {
  let specGenerator: ISpecGenerator;
  let mockProcessManager: jest.Mocked<PlughostProcessManager>;
  let mockParameterCategorizer: jest.Mocked<IParameterCategorizer>;
  let mockArgs: GeneratorArgs;
  let consoleSpy: any;
  let consoleErrorSpy: any;

  const mockPlugin: PluginInfo = {
    manufacturer: 'Test Manufacturer',
    name: 'Test Plugin',
    version: '1.0.0',
    format: 'VST3',
    uid: 'test-uid-123',
    category: 'Instrument',
  };

  const mockParameters: PluginParameter[] = [
    {
      index: 0,
      name: 'Master Volume',
      label: 'Volume',
      text: '75%',
      default_value: 0.75,
      current_value: 0.75,
      automatable: true,
      meta_parameter: false,
      discrete: false,
    },
    {
      index: 1,
      name: 'Filter Cutoff',
      label: 'Cutoff',
      text: '1000Hz',
      default_value: 0.5,
      current_value: 0.5,
      automatable: true,
      meta_parameter: false,
      discrete: false,
    },
    {
      index: 2,
      name: 'Bypass',
      label: 'Bypass',
      text: 'Off',
      default_value: 0,
      current_value: 0,
      automatable: false,
      meta_parameter: false,
      discrete: true,
    },
  ];

  beforeEach(() => {
    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn(),
      runProcess: vi.fn(),
    } as any;

    mockParameterCategorizer = {
      categorizeParameter: vi.fn(),
    } as any;

    mockArgs = {
      format: undefined,
      quick: false,
      help: false,
    };

    const options: SpecGeneratorOptions = {
      processManager: mockProcessManager,
      parameterCategorizer: mockParameterCategorizer,
      args: mockArgs,
    };

    specGenerator = createSpecGenerator(options);

    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
    consoleErrorSpy = vi.spyOn(console, 'error').mockImplementation(() => {});
  });

  afterEach(() => {
    vi.clearAllMocks();
    consoleSpy.mockRestore();
    consoleErrorSpy.mockRestore();
  });

  describe('generatePluginSpec', () => {
    beforeEach(() => {
      // Set up default mocks
      const mockInterrogateOutput = JSON.stringify({
        parameters: mockParameters,
      });

      mockProcessManager.runPlughost.mockResolvedValue(`Some output\n${mockInterrogateOutput}\nMore output`);
      mockProcessManager.isProblematicPlugin.mockReturnValue(false);

      // Set up categorizer responses
      mockParameterCategorizer.categorizeParameter
        .mockImplementation((name: string) => {
          if (name.includes('Volume')) return 'master';
          if (name.includes('Filter')) return 'filter';
          return 'misc';
        });
    });

    it('should generate complete plugin specification', async () => {
      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).not.toBeNull();
      expect(spec?.plugin).toEqual({
        manufacturer: 'Test Manufacturer',
        name: 'Test Plugin',
        version: '1.0.0',
        format: 'VST3',
        uid: 'test-uid-123',
      });

      expect(spec?.metadata).toEqual(
        expect.objectContaining({
          version: '1.0.0',
          author: 'plughost-generator',
          parameter_count: 3,
          tags: ['auto-generated', 'instrument', 'test-manufacturer'],
        })
      );

      expect(spec?.parameters).toHaveLength(3);
      expect(spec?.groups).toBeDefined();
    });

    it('should process parameters correctly', async () => {
      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec?.parameters[0]).toEqual({
        index: 0,
        name: 'Master Volume',
        min: 0.0,
        max: 1.0,
        default: 0.75,
        group: 'master',
        type: 'continuous',
        automatable: true,
        label: 'Volume',
      });

      expect(spec?.parameters[2]).toEqual({
        index: 2,
        name: 'Bypass',
        min: 0,
        max: 1,
        default: 0,
        group: 'misc',
        type: 'discrete',
        automatable: false,
        label: 'Bypass',
      });
    });

    it('should omit label when it matches parameter name', async () => {
      const parametersWithSameLabel = mockParameters.map(p => ({
        ...p,
        label: p.name, // Make label same as name
      }));

      const outputWithSameLabels = JSON.stringify({
        parameters: parametersWithSameLabel,
      });

      mockProcessManager.runPlughost.mockResolvedValue(outputWithSameLabels);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      // Labels should be omitted when they match the parameter name
      expect(spec?.parameters[0]).not.toHaveProperty('label');
      expect(spec?.parameters[1]).not.toHaveProperty('label');
      expect(spec?.parameters[2]).not.toHaveProperty('label');
    });

    it('should group parameters correctly', async () => {
      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec?.groups).toEqual({
        master: {
          name: 'Master',
          parameters: [0],
        },
        filter: {
          name: 'Filter',
          parameters: [1],
        },
        misc: {
          name: 'Misc',
          parameters: [2],
        },
      });
    });

    it('should skip problematic plugins in quick mode', async () => {
      mockArgs.quick = true;
      const quickOptions: SpecGeneratorOptions = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs,
      };
      specGenerator = createSpecGenerator(quickOptions);

      mockProcessManager.isProblematicPlugin.mockReturnValue(true);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(mockProcessManager.runPlughost).not.toHaveBeenCalled();
    });

    it('should handle plugins with no parameters', async () => {
      const outputWithNoParams = JSON.stringify({
        parameters: [],
      });

      mockProcessManager.runPlughost.mockResolvedValue(outputWithNoParams);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
    });

    it('should handle invalid JSON response', async () => {
      mockProcessManager.runPlughost.mockResolvedValue('Invalid JSON response');

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(consoleErrorSpy).toHaveBeenCalledWith(
        expect.stringContaining('Failed to generate spec'),
        expect.any(String)
      );
    });

    it('should handle response without JSON', async () => {
      mockProcessManager.runPlughost.mockResolvedValue('No JSON in this output');

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
    });

    it('should use shorter timeout for problematic plugins', async () => {
      mockProcessManager.isProblematicPlugin.mockReturnValue(true);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        expect.any(Array),
        20000 // Shorter timeout for problematic plugins
      );
    });

    it('should use normal timeout for regular plugins', async () => {
      mockProcessManager.isProblematicPlugin.mockReturnValue(false);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        expect.any(Array),
        45000 // Normal timeout
      );
    });

    it('should include quick-scan flag in quick mode', async () => {
      mockArgs.quick = true;
      const quickOptions: SpecGeneratorOptions = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs,
      };
      specGenerator = createSpecGenerator(quickOptions);

      mockProcessManager.isProblematicPlugin.mockReturnValue(false);

      await specGenerator.generatePluginSpec(mockPlugin);

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        expect.arrayContaining(['--quick-scan']),
        expect.any(Number)
      );
    });
  });

  describe('savePluginSpec', () => {
    const mockSpec = {
      plugin: mockPlugin,
      metadata: {
        version: '1.0.0',
        created: '2023-01-01T00:00:00Z',
        author: 'test',
        description: 'Test spec',
        parameter_count: 0,
        tags: [],
      },
      parameters: [],
      groups: {},
    };

    beforeEach(() => {
      vi.mocked(writeFileSync).mockImplementation(() => {});
    });

    it('should save plugin spec to correctly named file', () => {
      const outputDir = '/test/output';

      const filepath = specGenerator.savePluginSpec(mockPlugin, mockSpec, outputDir);

      expect(filepath).toBe('/test/output/test-manufacturer-test-plugin.json');
      expect(writeFileSync).toHaveBeenCalledWith(
        '/test/output/test-manufacturer-test-plugin.json',
        JSON.stringify(mockSpec, null, 2)
      );
    });

    it('should create safe identifiers from plugin names', () => {
      const pluginWithSpecialChars: PluginInfo = {
        ...mockPlugin,
        manufacturer: 'Test & Co.',
        name: 'My Plugin v2.0 (Pro)',
      };

      const filepath = specGenerator.savePluginSpec(pluginWithSpecialChars, mockSpec, '/test');

      expect(filepath).toBe('/test/test---co--my-plugin-v2-0--pro-.json');
    });

    it('should handle empty manufacturer and name', () => {
      const pluginWithEmptyFields: PluginInfo = {
        ...mockPlugin,
        manufacturer: '',
        name: '',
      };

      const filepath = specGenerator.savePluginSpec(pluginWithEmptyFields, mockSpec, '/test');

      expect(filepath).toBe('/test/-.json');
    });
  });

  describe('createSafeIdentifier', () => {
    // This tests the private method through the public savePluginSpec method
    it('should create safe identifiers', () => {
      const testCases = [
        { input: 'Simple Name', expected: 'simple-name' },
        { input: 'With-Numbers123', expected: 'with-numbers123' },
        { input: 'Special!@#$%^&*()Chars', expected: 'special----------chars' },
        { input: 'UPPERCASE', expected: 'uppercase' },
        { input: '', expected: '' },
      ];

      testCases.forEach(({ input, expected }) => {
        const plugin: PluginInfo = {
          ...mockPlugin,
          manufacturer: input,
          name: 'test',
        };

        const filepath = specGenerator.savePluginSpec(plugin, {
          plugin: mockPlugin,
          metadata: {
            version: '1.0.0',
            created: '2023-01-01T00:00:00Z',
            author: 'test',
            description: 'Test spec',
            parameter_count: 0,
            tags: [],
          },
          parameters: [],
          groups: {},
        }, '/test');

        expect(filepath).toContain(expected);
      });
    });
  });

  describe('createSpecGenerator factory', () => {
    it('should create a valid spec generator instance', () => {
      const options: SpecGeneratorOptions = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs,
      };

      const sg = createSpecGenerator(options);

      expect(sg).toBeDefined();
      expect(typeof sg.generatePluginSpec).toBe('function');
      expect(typeof sg.savePluginSpec).toBe('function');
    });

    it('should create independent instances', () => {
      const options: SpecGeneratorOptions = {
        processManager: mockProcessManager,
        parameterCategorizer: mockParameterCategorizer,
        args: mockArgs,
      };

      const sg1 = createSpecGenerator(options);
      const sg2 = createSpecGenerator(options);

      expect(sg1).not.toBe(sg2);
      expect(sg1).toBeInstanceOf(SpecGenerator);
      expect(sg2).toBeInstanceOf(SpecGenerator);
    });
  });

  describe('error handling', () => {
    beforeEach(() => {
      mockProcessManager.isProblematicPlugin.mockReturnValue(false);
      mockParameterCategorizer.categorizeParameter.mockReturnValue('misc');
    });

    it('should handle process manager errors', async () => {
      mockProcessManager.runPlughost.mockRejectedValue(new Error('Process failed'));

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(consoleErrorSpy).toHaveBeenCalledWith(
        expect.stringContaining('Failed to generate spec'),
        'Process failed'
      );
    });

    it('should handle JSON parsing errors', async () => {
      mockProcessManager.runPlughost.mockResolvedValue('{ invalid json }');

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
      expect(consoleErrorSpy).toHaveBeenCalled();
    });

    it('should handle missing parameters field in JSON', async () => {
      const outputWithoutParams = JSON.stringify({
        someOtherField: 'value',
      });

      mockProcessManager.runPlughost.mockResolvedValue(outputWithoutParams);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
    });

    it('should handle non-array parameters field', async () => {
      const outputWithInvalidParams = JSON.stringify({
        parameters: 'not an array',
      });

      mockProcessManager.runPlughost.mockResolvedValue(outputWithInvalidParams);

      const spec = await specGenerator.generatePluginSpec(mockPlugin);

      expect(spec).toBeNull();
    });
  });
});