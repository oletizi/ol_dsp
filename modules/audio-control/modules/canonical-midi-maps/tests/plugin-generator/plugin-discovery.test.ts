/**
 * Tests for plugin discovery functionality
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import {
  PluginDiscovery,
  createPluginDiscovery
} from '@/tools/plugin-generator/plugin-discovery.js';
import type { PluginInfo, GeneratorArgs } from '@/tools/plugin-generator/types.js';
import type { PlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';

// Mock console methods
const mockConsoleLog = vi.spyOn(console, 'log').mockImplementation(() => {});
const mockConsoleError = vi.spyOn(console, 'error').mockImplementation(() => {});

describe('PluginDiscovery', () => {
  let mockProcessManager: PlughostProcessManager;
  let discovery: PluginDiscovery;
  let mockArgs: GeneratorArgs;

  beforeEach(() => {
    vi.clearAllMocks();

    mockProcessManager = {
      runPlughost: vi.fn(),
      isProblematicPlugin: vi.fn(),
      runProcess: vi.fn()
    } as any;

    mockArgs = {
      format: undefined,
      quick: false,
      help: false
    };

    discovery = new PluginDiscovery({
      processManager: mockProcessManager,
      args: mockArgs
    });
  });

  describe('getPluginList', () => {
    describe('batch JSON approach', () => {
      it('should successfully parse batch JSON output', async () => {
        const mockOutput = `Some initial text
{
  "plugins": [
    {
      "manufacturer": "Test Company",
      "name": "Test Plugin",
      "version": "1.0.0",
      "format": "VST3",
      "uid": "test-uid-123",
      "category": "Instrument"
    }
  ]
}
Some trailing text`;

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0]).toEqual({
          manufacturer: 'Test Company',
          name: 'Test Plugin',
          version: '1.0.0',
          format: 'VST3',
          uid: 'test-uid-123',
          category: 'Instrument'
        });

        expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
          ['--list', '--skip-instantiation', '--batch-json'],
          180000
        );
      });

      it('should handle plugins with missing optional fields', async () => {
        const mockOutput = `{
  "plugins": [
    {
      "name": "Minimal Plugin",
      "format": "AudioUnit"
    }
  ]
}`;

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

        const plugins = await discovery.getPluginList();

        expect(plugins[0]).toEqual({
          manufacturer: 'Unknown',
          name: 'Minimal Plugin',
          version: '1.0.0',
          format: 'AudioUnit',
          uid: '',
          category: 'Effect'
        });
      });

      it('should add format filter when specified', async () => {
        mockArgs.format = 'VST3';
        const mockOutput = '{"plugins": []}';

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

        await discovery.getPluginList();

        expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
          ['--list', '--skip-instantiation', '--batch-json', '--format-filter', 'VST3'],
          180000
        );
      });

      it('should add quick scan flag when requested', async () => {
        mockArgs.quick = true;
        const mockOutput = '{"plugins": []}';

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

        await discovery.getPluginList();

        expect(mockProcessManager.runPlughost).toHaveBeenCalledWith(
          ['--list', '--skip-instantiation', '--batch-json', '--quick-scan'],
          180000
        );
      });

      it('should filter problematic plugins in quick mode', async () => {
        mockArgs.quick = true;
        const mockOutput = `{
  "plugins": [
    {"name": "Good Plugin", "format": "VST3"},
    {"name": "ZamVerb", "format": "VST3"}
  ]
}`;

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);
        (mockProcessManager.isProblematicPlugin as any).mockImplementation((name: string) =>
          name === 'ZamVerb'
        );

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0].name).toBe('Good Plugin');
      });
    });

    describe('fallback approach', () => {
      it('should fall back to basic listing when JSON fails', async () => {
        // First call (batch) fails
        (mockProcessManager.runPlughost as any)
          .mockRejectedValueOnce(new Error('Batch failed'))
          .mockResolvedValueOnce(`Some output
Next Plugin: <Format:VST3>, <Name: Fallback Plugin>
More output`);

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0]).toEqual({
          manufacturer: 'Unknown',
          name: 'Fallback Plugin',
          version: '1.0.0',
          format: 'VST3',
          uid: '',
          category: 'Effect'
        });

        // Should have tried batch first, then fallback
        expect(mockProcessManager.runPlughost).toHaveBeenCalledTimes(2);
      });

      it('should parse multiple plugins from fallback output', async () => {
        (mockProcessManager.runPlughost as any)
          .mockRejectedValueOnce(new Error('Batch failed'))
          .mockResolvedValueOnce(`
Next Plugin: <Format:VST3>, <Name: Plugin One>
Some other text
Next Plugin: <Format:AudioUnit>, <Name: Plugin Two>
More text
Invalid line
Next Plugin: <Format:VST>, <Name: Plugin Three>`);

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(3);
        expect(plugins[0].name).toBe('Plugin One');
        expect(plugins[0].format).toBe('VST3');
        expect(plugins[1].name).toBe('Plugin Two');
        expect(plugins[1].format).toBe('AudioUnit');
        expect(plugins[2].name).toBe('Plugin Three');
        expect(plugins[2].format).toBe('VST');
      });

      it('should apply format filter in fallback mode', async () => {
        mockArgs.format = 'VST3';

        (mockProcessManager.runPlughost as any)
          .mockRejectedValueOnce(new Error('Batch failed'))
          .mockResolvedValueOnce(`
Next Plugin: <Format:VST3>, <Name: VST3 Plugin>
Next Plugin: <Format:AudioUnit>, <Name: AU Plugin>`);

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0].name).toBe('VST3 Plugin');
        expect(plugins[0].format).toBe('VST3');
      });

      it('should skip problematic plugins in quick mode fallback', async () => {
        mockArgs.quick = true;

        (mockProcessManager.runPlughost as any)
          .mockRejectedValueOnce(new Error('Batch failed'))
          .mockResolvedValueOnce(`
Next Plugin: <Format:VST3>, <Name: Good Plugin>
Next Plugin: <Format:VST3>, <Name: ZamVerb>`);

        (mockProcessManager.isProblematicPlugin as any).mockImplementation((name: string) =>
          name === 'ZamVerb'
        );

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0].name).toBe('Good Plugin');
      });

      it('should return empty array when both approaches fail', async () => {
        (mockProcessManager.runPlughost as any)
          .mockRejectedValueOnce(new Error('Batch failed'))
          .mockRejectedValueOnce(new Error('Fallback failed'));

        const plugins = await discovery.getPluginList();

        expect(plugins).toEqual([]);
        expect(mockConsoleError).toHaveBeenCalledWith(
          '❌ Fallback listing also failed:',
          'Fallback failed'
        );
      });
    });

    describe('error handling', () => {
      it('should handle invalid JSON in batch mode', async () => {
        (mockProcessManager.runPlughost as any)
          .mockResolvedValueOnce('invalid json output')
          .mockResolvedValueOnce('Next Plugin: <Format:VST3>, <Name: Fallback Plugin>');

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0].name).toBe('Fallback Plugin');
      });

      it('should handle JSON without plugins array', async () => {
        (mockProcessManager.runPlughost as any)
          .mockResolvedValueOnce('{"invalid": "structure"}')
          .mockResolvedValueOnce('Next Plugin: <Format:VST3>, <Name: Fallback Plugin>');

        const plugins = await discovery.getPluginList();

        expect(plugins).toHaveLength(1);
        expect(plugins[0].name).toBe('Fallback Plugin');
      });

      it('should handle empty JSON plugins array', async () => {
        const mockOutput = '{"plugins": []}';

        (mockProcessManager.runPlughost as any).mockResolvedValue(mockOutput);

        const plugins = await discovery.getPluginList();

        expect(plugins).toEqual([]);
      });
    });
  });

  describe('shouldSkipPlugin', () => {
    it('should skip system AU plugins', () => {
      const systemPlugins: PluginInfo[] = [
        { name: 'AU Plugin', format: 'AudioUnit' } as PluginInfo,
        { name: 'AUAudio Something', format: 'AudioUnit' } as PluginInfo,
        { name: 'AUNet Service', format: 'AudioUnit' } as PluginInfo,
        { name: 'AUMatrix Tool', format: 'AudioUnit' } as PluginInfo,
        { name: 'AUSample Plugin', format: 'AudioUnit' } as PluginInfo
      ];

      systemPlugins.forEach(plugin => {
        expect(discovery.shouldSkipPlugin(plugin)).toBe(true);
      });
    });

    it('should not skip regular plugins', () => {
      const regularPlugins: PluginInfo[] = [
        { name: 'Normal Plugin', format: 'VST3' } as PluginInfo,
        { name: 'Audio Effect', format: 'VST3' } as PluginInfo,
        { name: 'Synth Plugin', format: 'AudioUnit' } as PluginInfo
      ];

      regularPlugins.forEach(plugin => {
        expect(discovery.shouldSkipPlugin(plugin)).toBe(false);
      });
    });
  });

  describe('createPluginDiscovery factory', () => {
    it('should create PluginDiscovery instance', () => {
      const options = {
        processManager: mockProcessManager,
        args: mockArgs
      };

      const discovery = createPluginDiscovery(options);

      expect(discovery).toBeInstanceOf(PluginDiscovery);
      expect(typeof discovery.getPluginList).toBe('function');
      expect(typeof discovery.shouldSkipPlugin).toBe('function');
    });
  });
});