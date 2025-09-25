/**
 * Tests for plugin discovery functionality
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { PluginDiscovery, createPluginDiscovery } from '@/tools/plugin-generator/plugin-discovery.js';
import type { IPluginDiscovery, PluginDiscoveryOptions } from '@/tools/plugin-generator/plugin-discovery.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';
import type { GeneratorArgs, PluginInfo } from '@/tools/plugin-generator/types.js';

describe('PluginDiscovery', () => {
  let pluginDiscovery: IPluginDiscovery;
  let mockProcessManager: jest.Mocked<PlughostProcessManager>;
  let mockArgs: GeneratorArgs;
  let consoleSpy: any;
  let consoleErrorSpy: any;

  beforeEach(() => {
    // Create mock process manager
    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn(),
      runProcess: vi.fn(),
    } as any;

    mockArgs = {
      format: undefined,
      quick: false,
      help: false,
    };

    const options: PluginDiscoveryOptions = {
      processManager: mockProcessManager,
      args: mockArgs,
    };

    pluginDiscovery = createPluginDiscovery(options);

    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
    consoleErrorSpy = vi.spyOn(console, 'error').mockImplementation(() => {});
  });

  afterEach(() => {
    vi.clearAllMocks();
    consoleSpy.mockRestore();
    consoleErrorSpy.mockRestore();
  });

  describe('getPluginList - batch JSON approach', () => {
    const mockBatchOutput = JSON.stringify({
      plugins: [
        {
          manufacturer: 'Test Manufacturer',
          name: 'Test Plugin 1',
          version: '1.0.0',
          format: 'VST3',
          uid: 'test-uid-1',
          category: 'Instrument',
        },
        {
          manufacturer: 'Another Manufacturer',
          name: 'Test Plugin 2',
          version: '2.0.0',
          format: 'AudioUnit',
          uid: 'test-uid-2',
          category: 'Effect',
        },
      ],
    });

    it('should successfully get plugins using batch JSON approach', async () => {
      mockProcessManager.runPlughost.mockResolvedValue(`Some output\n${mockBatchOutput}\nMore output`);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2);
      expect(plugins[0]).toEqual({
        manufacturer: 'Test Manufacturer',
        name: 'Test Plugin 1',
        version: '1.0.0',
        format: 'VST3',
        uid: 'test-uid-1',
        category: 'Instrument',
      });

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json'],
        180000
      );
    });

    it('should apply format filter when specified', async () => {
      mockArgs.format = 'AudioUnit';
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };
      pluginDiscovery = createPluginDiscovery(options);

      mockProcessManager.runPlughost.mockResolvedValue(mockBatchOutput);

      await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json', '--format-filter', 'AudioUnit'],
        180000
      );
    });

    it('should enable quick scan when quick mode is specified', async () => {
      mockArgs.quick = true;
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };
      pluginDiscovery = createPluginDiscovery(options);

      mockProcessManager.runPlughost.mockResolvedValue(mockBatchOutput);

      await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json', '--quick-scan'],
        180000
      );
    });

    it('should filter problematic plugins in quick mode', async () => {
      mockArgs.quick = true;
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };
      pluginDiscovery = createPluginDiscovery(options);

      const outputWithProblematic = JSON.stringify({
        plugins: [
          { name: 'Good Plugin', format: 'VST3', manufacturer: 'Test' },
          { name: 'ZamVerb', format: 'VST3', manufacturer: 'Test' },
        ],
      });

      mockProcessManager.runPlughost.mockResolvedValue(outputWithProblematic);
      mockProcessManager.isProblematicPlugin.mockImplementation((name) => name === 'ZamVerb');

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(1);
      expect(plugins[0]?.name).toBe('Good Plugin');
    });

    it('should handle missing optional fields with defaults', async () => {
      const minimalOutput = JSON.stringify({
        plugins: [
          {
            name: 'Minimal Plugin',
            format: 'VST3',
          },
        ],
      });

      mockProcessManager.runPlughost.mockResolvedValue(minimalOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins[0]).toEqual({
        manufacturer: 'Unknown',
        name: 'Minimal Plugin',
        version: '1.0.0',
        format: 'VST3',
        uid: '',
        category: 'Effect',
      });
    });
  });

  describe('getPluginList - fallback approach', () => {
    const mockFallbackOutput = `
Next Plugin: <Format:VST3>, <Name: Fallback Plugin 1>
Some other line
Next Plugin: <Format:AudioUnit>, <Name: Fallback Plugin 2>
Invalid line
Next Plugin: <Format:VST3>, <Name: ZamVerb>
`;

    it('should fallback to basic listing when batch fails', async () => {
      mockProcessManager.runPlughost
        .mockRejectedValueOnce(new Error('Batch JSON failed'))
        .mockResolvedValueOnce(mockFallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(3);
      expect(plugins[0]).toEqual({
        manufacturer: 'Unknown',
        name: 'Fallback Plugin 1',
        version: '1.0.0',
        format: 'VST3',
        uid: '',
        category: 'Effect',
      });

      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Batch JSON listing failed, trying fallback approach')
      );
    });

    it('should apply format filter in fallback mode', async () => {
      mockArgs.format = 'AudioUnit';
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };
      pluginDiscovery = createPluginDiscovery(options);

      mockProcessManager.runPlughost
        .mockRejectedValueOnce(new Error('Batch JSON failed'))
        .mockResolvedValueOnce(mockFallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      // Should only get the AudioUnit plugin
      expect(plugins).toHaveLength(1);
      expect(plugins[0]?.format).toBe('AudioUnit');
    });

    it('should skip problematic plugins in quick mode during fallback', async () => {
      mockArgs.quick = true;
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };
      pluginDiscovery = createPluginDiscovery(options);

      mockProcessManager.runPlughost
        .mockRejectedValueOnce(new Error('Batch JSON failed'))
        .mockResolvedValueOnce(mockFallbackOutput);

      mockProcessManager.isProblematicPlugin.mockImplementation((name) => name === 'ZamVerb');

      const plugins = await pluginDiscovery.getPluginList();

      // Should skip ZamVerb
      expect(plugins).toHaveLength(2);
      expect(plugins.find(p => p.name === 'ZamVerb')).toBeUndefined();
    });

    it('should return empty array when fallback also fails', async () => {
      mockProcessManager.runPlughost
        .mockRejectedValueOnce(new Error('Batch JSON failed'))
        .mockRejectedValueOnce(new Error('Fallback also failed'));

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toEqual([]);
      expect(consoleErrorSpy).toHaveBeenCalledWith(
        expect.stringContaining('Fallback listing also failed')
      );
    });
  });

  describe('JSON parsing edge cases', () => {
    it('should handle invalid JSON in batch output', async () => {
      mockProcessManager.runPlughost
        .mockResolvedValueOnce('Invalid JSON output')
        .mockResolvedValueOnce('Next Plugin: <Format:VST3>, <Name: Fallback Plugin>');

      const plugins = await pluginDiscovery.getPluginList();

      // Should fallback and get the plugin
      expect(plugins).toHaveLength(1);
      expect(plugins[0]?.name).toBe('Fallback Plugin');
    });

    it('should handle JSON with invalid structure', async () => {
      const invalidStructure = JSON.stringify({
        invalid: 'structure',
      });

      mockProcessManager.runPlughost
        .mockResolvedValueOnce(invalidStructure)
        .mockResolvedValueOnce('Next Plugin: <Format:VST3>, <Name: Fallback Plugin>');

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(1);
      expect(plugins[0]?.name).toBe('Fallback Plugin');
    });

    it('should handle JSON with non-array plugins field', async () => {
      const invalidPluginsField = JSON.stringify({
        plugins: 'not an array',
      });

      mockProcessManager.runPlughost
        .mockResolvedValueOnce(invalidPluginsField)
        .mockResolvedValueOnce('Next Plugin: <Format:VST3>, <Name: Fallback Plugin>');

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(1);
      expect(plugins[0]?.name).toBe('Fallback Plugin');
    });
  });

  describe('shouldSkipPlugin', () => {
    it('should skip system Audio Unit plugins', () => {
      const systemPlugins: PluginInfo[] = [
        { name: 'AUAudioFilePlayer', format: 'AudioUnit', manufacturer: 'Apple', version: '1.0.0', uid: '', category: 'Generator' },
        { name: 'AUNetSend', format: 'AudioUnit', manufacturer: 'Apple', version: '1.0.0', uid: '', category: 'Effect' },
        { name: 'AUSampler', format: 'AudioUnit', manufacturer: 'Apple', version: '1.0.0', uid: '', category: 'Instrument' },
        { name: 'AUMatrix', format: 'AudioUnit', manufacturer: 'Apple', version: '1.0.0', uid: '', category: 'Effect' },
      ];

      systemPlugins.forEach(plugin => {
        expect(pluginDiscovery.shouldSkipPlugin(plugin)).toBe(true);
      });
    });

    it('should not skip regular plugins', () => {
      const regularPlugins: PluginInfo[] = [
        { name: 'Serum', format: 'VST3', manufacturer: 'Xfer', version: '1.0.0', uid: '', category: 'Instrument' },
        { name: 'FabFilter Pro-Q', format: 'VST3', manufacturer: 'FabFilter', version: '3.0.0', uid: '', category: 'Effect' },
        { name: 'Massive', format: 'VST3', manufacturer: 'Native Instruments', version: '1.0.0', uid: '', category: 'Instrument' },
      ];

      regularPlugins.forEach(plugin => {
        expect(pluginDiscovery.shouldSkipPlugin(plugin)).toBe(false);
      });
    });

    it('should not skip plugins that contain AU but dont start with system prefixes', () => {
      const nonSystemPlugins: PluginInfo[] = [
        { name: 'AudioDamage Rough Rider', format: 'VST3', manufacturer: 'AudioDamage', version: '1.0.0', uid: '', category: 'Effect' },
        { name: 'Auto-Tune', format: 'VST3', manufacturer: 'Antares', version: '1.0.0', uid: '', category: 'Effect' },
      ];

      nonSystemPlugins.forEach(plugin => {
        expect(pluginDiscovery.shouldSkipPlugin(plugin)).toBe(false);
      });
    });
  });

  describe('createPluginDiscovery factory', () => {
    it('should create a valid plugin discovery instance', () => {
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };

      const pd = createPluginDiscovery(options);

      expect(pd).toBeDefined();
      expect(typeof pd.getPluginList).toBe('function');
      expect(typeof pd.shouldSkipPlugin).toBe('function');
    });

    it('should create independent instances', () => {
      const options: PluginDiscoveryOptions = {
        processManager: mockProcessManager,
        args: mockArgs,
      };

      const pd1 = createPluginDiscovery(options);
      const pd2 = createPluginDiscovery(options);

      expect(pd1).not.toBe(pd2);
      expect(pd1).toBeInstanceOf(PluginDiscovery);
      expect(pd2).toBeInstanceOf(PluginDiscovery);
    });
  });
});