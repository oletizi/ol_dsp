/**
 * Test suite for plugin discovery functionality
 */

import { describe, expect, it, beforeEach, vi } from 'vitest';
import {
  PluginDiscovery,
  createPluginDiscovery,
} from '@/tools/plugin-generator/plugin-discovery.js';
import type { PluginInfo, GeneratorArgs } from '@/tools/plugin-generator/types.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';

describe('PluginDiscovery', () => {
  let pluginDiscovery: PluginDiscovery;
  let mockProcessManager: PlughostProcessManager;
  let mockArgs: GeneratorArgs;

  beforeEach(() => {
    // Mock process manager
    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn().mockReturnValue(false),
    } as any;

    mockArgs = {
      format: undefined,
      quick: false,
      help: false,
    };

    pluginDiscovery = new PluginDiscovery({
      processManager: mockProcessManager,
      args: mockArgs,
    });

    vi.clearAllMocks();
  });

  describe('getPluginList', () => {
    it('should get plugin list using batch JSON approach', async () => {
      const mockPluginData = {
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
            format: 'VST3',
            uid: 'test-uid-2',
            category: 'Effect',
          },
        ],
      };

      const mockOutput = `Some header text\n${JSON.stringify(mockPluginData)}\nTrailing text`;
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json'],
        180000
      );

      expect(plugins).toHaveLength(2);
      expect(plugins[0]).toEqual({
        manufacturer: 'Test Manufacturer',
        name: 'Test Plugin 1',
        version: '1.0.0',
        format: 'VST3',
        uid: 'test-uid-1',
        category: 'Instrument',
      });
    });

    it('should include quick-scan argument when quick mode is enabled', async () => {
      mockArgs.quick = true;
      const mockPluginData = { plugins: [] };
      const mockOutput = JSON.stringify(mockPluginData);
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json', '--quick-scan'],
        180000
      );
    });

    it('should include format filter when format is specified', async () => {
      mockArgs.format = 'VST3';
      const mockPluginData = { plugins: [] };
      const mockOutput = JSON.stringify(mockPluginData);
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
        ['--list', '--skip-instantiation', '--batch-json', '--format-filter', 'VST3'],
        180000
      );
    });

    it('should filter problematic plugins in quick mode', async () => {
      mockArgs.quick = true;
      const mockPluginData = {
        plugins: [
          { name: 'Good Plugin', format: 'VST3' },
          { name: 'ZamVerb', format: 'VST3' }, // Problematic
          { name: 'Another Good Plugin', format: 'VST3' },
        ],
      };

      (mockProcessManager.isProblematicPlugin as any).mockImplementation(
        (name: string) => name.includes('Zam')
      );

      const mockOutput = JSON.stringify(mockPluginData);
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2);
      expect(plugins.find((p) => p.name === 'ZamVerb')).toBeUndefined();
    });

    it('should fallback to basic listing when batch JSON fails', async () => {
      // First call (batch JSON) fails
      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch JSON not supported')
      );

      // Second call (fallback) succeeds
      const fallbackOutput = `
        Next Plugin: <Format:VST3>, <Name: Plugin 1>
        Next Plugin: <Format:VST>, <Name: Plugin 2>
        Some other text
      `;
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledTimes(2);
      expect(mockProcessManager.runPlughost).toHaveBeenNthCalledWith(
        2,
        ['--list'],
        120000
      );

      expect(plugins).toHaveLength(2);
      expect(plugins[0]).toEqual({
        manufacturer: 'Unknown',
        name: 'Plugin 1',
        version: '1.0.0',
        format: 'VST3',
        uid: '',
        category: 'Effect',
      });
    });

    it('should apply format filter in fallback mode', async () => {
      mockArgs.format = 'VST3';

      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch JSON not supported')
      );

      const fallbackOutput = `
        Next Plugin: <Format:VST3>, <Name: Plugin 1>
        Next Plugin: <Format:VST>, <Name: Plugin 2>
        Next Plugin: <Format:VST3>, <Name: Plugin 3>
      `;
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2); // Only VST3 plugins
      expect(plugins.every((p) => p.format === 'VST3')).toBe(true);
    });

    it('should skip problematic plugins in fallback quick mode', async () => {
      mockArgs.quick = true;

      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch JSON not supported')
      );

      const fallbackOutput = `
        Next Plugin: <Format:VST3>, <Name: Good Plugin>
        Next Plugin: <Format:VST3>, <Name: ZamVerb>
        Next Plugin: <Format:VST3>, <Name: Another Good Plugin>
      `;
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);
      (mockProcessManager.isProblematicPlugin as any).mockImplementation(
        (name: string) => name.includes('Zam')
      );

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2);
      expect(plugins.find((p) => p.name === 'ZamVerb')).toBeUndefined();
    });

    it('should return empty array when both batch and fallback fail', async () => {
      (mockProcessManager.runPlughost as any).mockRejectedValue(
        new Error('Complete failure')
      );

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toEqual([]);
    });

    it('should handle malformed JSON in batch mode', async () => {
      const malformedOutput = 'Not JSON at all';
      (mockProcessManager.runPlughost as any).mockResolvedValueOnce(malformedOutput);

      // Should fall back to basic listing
      const fallbackOutput = 'Next Plugin: <Format:VST3>, <Name: Plugin 1>';
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledTimes(2);
      expect(plugins).toHaveLength(1);
    });

    it('should handle missing JSON structure in batch mode', async () => {
      const invalidJsonOutput = JSON.stringify({ not_plugins: [] });
      (mockProcessManager.runPlughost as any).mockResolvedValueOnce(
        `Header\n${invalidJsonOutput}\nFooter`
      );

      // Should fall back to basic listing
      const fallbackOutput = 'Next Plugin: <Format:VST3>, <Name: Plugin 1>';
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(mockProcessManager.runPlughost).toHaveBeenCalledTimes(2);
      expect(plugins).toHaveLength(1);
    });

    it('should handle plugins with missing optional fields', async () => {
      const mockPluginData = {
        plugins: [
          {
            name: 'Minimal Plugin',
            format: 'VST3',
            // Missing manufacturer, version, uid, category
          },
          {
            manufacturer: 'Full Manufacturer',
            name: 'Full Plugin',
            version: '1.0.0',
            format: 'VST3',
            uid: 'full-uid',
            category: 'Instrument',
          },
        ],
      };

      const mockOutput = JSON.stringify(mockPluginData);
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2);
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

  describe('shouldSkipPlugin', () => {
    it('should skip system AU plugins', () => {
      const systemPlugins = [
        { name: 'AUAudioFilePlayer', format: 'AU' } as PluginInfo,
        { name: 'AUNetSend', format: 'AU' } as PluginInfo,
        { name: 'AUMatrixReverb', format: 'AU' } as PluginInfo,
        { name: 'AUSampler', format: 'AU' } as PluginInfo,
      ];

      systemPlugins.forEach((plugin) => {
        expect(pluginDiscovery.shouldSkipPlugin(plugin)).toBe(true);
      });
    });

    it('should not skip regular plugins', () => {
      const regularPlugins = [
        { name: 'Massive X', format: 'VST3' } as PluginInfo,
        { name: 'FabFilter Pro-Q 3', format: 'VST3' } as PluginInfo,
        { name: 'Audio Units Plugin', format: 'AU' } as PluginInfo, // Doesn't start with AU
        { name: 'Regular AU Plugin', format: 'AU' } as PluginInfo,
      ];

      regularPlugins.forEach((plugin) => {
        expect(pluginDiscovery.shouldSkipPlugin(plugin)).toBe(false);
      });
    });

    it('should handle edge cases for system plugin detection', () => {
      const edgeCases = [
        { name: 'AU', format: 'AU' } as PluginInfo, // Just "AU"
        { name: 'AUPlugin', format: 'AU' } as PluginInfo, // Starts with AU but not system
        { name: '', format: 'AU' } as PluginInfo, // Empty name
        { name: 'AudioUnit Plugin', format: 'AU' } as PluginInfo, // Contains "AU" but doesn't start with system prefix
      ];

      expect(pluginDiscovery.shouldSkipPlugin(edgeCases[0])).toBe(true); // "AU" matches pattern
      expect(pluginDiscovery.shouldSkipPlugin(edgeCases[1])).toBe(false); // "AUPlugin" doesn't match system patterns
      expect(pluginDiscovery.shouldSkipPlugin(edgeCases[2])).toBe(false); // Empty name
      expect(pluginDiscovery.shouldSkipPlugin(edgeCases[3])).toBe(false); // Doesn't start with system prefix
    });
  });

  describe('factory function', () => {
    it('should create a functional plugin discovery service', () => {
      const options = {
        processManager: mockProcessManager,
        args: mockArgs,
      };

      const discovery = createPluginDiscovery(options);

      expect(discovery).toBeDefined();
      expect(discovery).toBeInstanceOf(PluginDiscovery);
    });
  });

  describe('error handling and edge cases', () => {
    it('should handle console output during fallback mode', async () => {
      const consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});

      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch failed')
      );
      (mockProcessManager.runPlughost as any).mockResolvedValue('');

      await pluginDiscovery.getPluginList();

      expect(consoleSpy).toHaveBeenCalledWith(
        '⚠️  Batch JSON listing failed, trying fallback approach...'
      );
      expect(consoleSpy).toHaveBeenCalledWith('Error: Batch failed');

      consoleSpy.mockRestore();
    });

    it('should handle malformed plugin lines in fallback mode', async () => {
      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch failed')
      );

      const fallbackOutput = `
        Next Plugin: <Format:VST3>, <Name: Good Plugin>
        Malformed line without proper format
        Next Plugin: <Format:>, <Name: No Format>
        Next Plugin: <Format:VST3>, <Name:>
        Next Plugin: <Format:VST3>, <Name: Another Good Plugin>
      `;
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      // Should only get valid plugins
      expect(plugins).toHaveLength(2);
      expect(plugins[0].name).toBe('Good Plugin');
      expect(plugins[1].name).toBe('Another Good Plugin');
    });

    it('should handle empty or whitespace-only output in fallback mode', async () => {
      (mockProcessManager.runPlughost as any).mockRejectedValueOnce(
        new Error('Batch failed')
      );

      const fallbackOutput = '   \n\n  \t  \n';
      (mockProcessManager.runPlughost as any).mockResolvedValue(fallbackOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins).toEqual([]);
    });

    it('should preserve manufacturer when available in batch mode', async () => {
      const mockPluginData = {
        plugins: [
          {
            manufacturer: 'Native Instruments',
            name: 'Massive X',
            format: 'VST3',
          },
          {
            // No manufacturer field
            name: 'Unknown Manufacturer Plugin',
            format: 'VST3',
          },
        ],
      };

      const mockOutput = JSON.stringify(mockPluginData);
      (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

      const plugins = await pluginDiscovery.getPluginList();

      expect(plugins[0].manufacturer).toBe('Native Instruments');
      expect(plugins[1].manufacturer).toBe('Unknown');
    });
  });
});