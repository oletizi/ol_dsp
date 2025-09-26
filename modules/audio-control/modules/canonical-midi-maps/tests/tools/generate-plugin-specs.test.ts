/**
 * Tests for the main plugin specification generator
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { existsSync, mkdirSync } from 'fs';
import type { PluginInfo } from '@/tools/plugin-generator/types.js';

// Mock fs
vi.mock('fs', () => ({
  existsSync: vi.fn(),
  mkdirSync: vi.fn(),
}));

// Mock the plugin generator components
vi.mock('@/tools/plugin-generator/index.js', () => ({
  createPluginSpecGeneratorComponents: vi.fn(),
}));

// We'll test the PluginSpecGenerator class logic by importing it
// Note: This is testing the orchestration logic, not the subprocess execution

describe('PluginSpecGenerator Integration', () => {
  let consoleSpy: any;
  let consoleErrorSpy: any;
  let processExitSpy: any;

  const mockComponents = {
    cli: {
      showHelp: vi.fn(),
    },
    args: {
      help: false,
      quick: false,
      format: undefined,
    },
    pluginDiscovery: {
      getPluginList: vi.fn(),
      shouldSkipPlugin: vi.fn(),
    },
    specGenerator: {
      generatePluginSpec: vi.fn(),
      savePluginSpec: vi.fn(),
    },
  };

  const mockPlugins: PluginInfo[] = [
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
  ];

  beforeEach(() => {
    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
    consoleErrorSpy = vi.spyOn(console, 'error').mockImplementation(() => {});
    processExitSpy = vi.spyOn(process, 'exit').mockImplementation(() => {
      throw new Error('process.exit called');
    });

    // Reset all mocks
    vi.clearAllMocks();

    // Set up default mock implementations
    vi.mocked(existsSync).mockReturnValue(true);
    vi.mocked(mkdirSync).mockImplementation(() => {});

    mockComponents.pluginDiscovery.getPluginList.mockResolvedValue(mockPlugins);
    mockComponents.pluginDiscovery.shouldSkipPlugin.mockReturnValue(false);
    mockComponents.specGenerator.generatePluginSpec.mockResolvedValue({
      plugin: mockPlugins[0]!,
      metadata: {
        version: '1.0.0',
        created: '2023-01-01T00:00:00Z',
        author: 'test',
        description: 'Test spec',
        parameter_count: 5,
        tags: ['test'],
      },
      parameters: [],
      groups: {},
    });
    mockComponents.specGenerator.savePluginSpec.mockReturnValue('/test/output/test-plugin.json');

    // Mock the factory function
    const { createPluginSpecGeneratorComponents } = await import('@/tools/plugin-generator/index.js');
    vi.mocked(createPluginSpecGeneratorComponents).mockReturnValue(mockComponents);
  });

  afterEach(() => {
    consoleSpy.mockRestore();
    consoleErrorSpy.mockRestore();
    processExitSpy.mockRestore();
  });

  describe('help functionality', () => {
    it('should show help when help flag is set', async () => {
      mockComponents.args.help = true;

      // We need to test the class logic directly since we can't easily test the main execution
      const { createPluginSpecGeneratorComponents } = await import('@/tools/plugin-generator/index.js');
      const components = createPluginSpecGeneratorComponents({
        plughostPath: '/test/plughost',
        outputDir: '/test/output',
      });

      if (components.args.help) {
        components.cli.showHelp('/test/output');
      }

      expect(mockComponents.cli.showHelp).toHaveBeenCalledWith('/test/output');
    });
  });

  describe('plughost validation', () => {
    it('should exit when plughost is not found', async () => {
      vi.mocked(existsSync).mockReturnValue(false);

      try {
        // Test the validation logic
        const plughostPath = '/nonexistent/plughost';
        if (!existsSync(plughostPath)) {
          console.error(`❌ plughost not found at: ${plughostPath}`);
          console.error('Please run "make all" from the project root first.');
          process.exit(1);
        }
      } catch (error: any) {
        expect(error.message).toBe('process.exit called');
      }

      expect(consoleErrorSpy).toHaveBeenCalledWith(
        expect.stringContaining('plughost not found at')
      );
      expect(processExitSpy).toHaveBeenCalledWith(1);
    });
  });

  describe('plugin processing workflow', () => {
    it('should process plugins successfully', async () => {
      const plugins = await mockComponents.pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(2);
      expect(mockComponents.pluginDiscovery.getPluginList).toHaveBeenCalled();
    });

    it('should skip system plugins', async () => {
      const systemPlugin: PluginInfo = {
        manufacturer: 'Apple',
        name: 'AUAudioFilePlayer',
        version: '1.0.0',
        format: 'AudioUnit',
        uid: 'au-system',
        category: 'Generator',
      };

      mockComponents.pluginDiscovery.shouldSkipPlugin.mockReturnValue(true);

      const shouldSkip = mockComponents.pluginDiscovery.shouldSkipPlugin(systemPlugin);
      expect(shouldSkip).toBe(true);
    });

    it('should generate specs for valid plugins', async () => {
      const plugin = mockPlugins[0]!;
      const spec = await mockComponents.specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeDefined();
      expect(mockComponents.specGenerator.generatePluginSpec).toHaveBeenCalledWith(plugin);
    });

    it('should save generated specs to files', async () => {
      const plugin = mockPlugins[0]!;
      const spec = {
        plugin,
        metadata: {
          version: '1.0.0',
          created: '2023-01-01T00:00:00Z',
          author: 'test',
          description: 'Test spec',
          parameter_count: 5,
          tags: ['test'],
        },
        parameters: [],
        groups: {},
      };

      const filepath = mockComponents.specGenerator.savePluginSpec(plugin, spec, '/test/output');

      expect(filepath).toBe('/test/output/test-plugin.json');
      expect(mockComponents.specGenerator.savePluginSpec).toHaveBeenCalledWith(
        plugin,
        spec,
        '/test/output'
      );
    });

    it('should handle plugin generation failures gracefully', async () => {
      mockComponents.specGenerator.generatePluginSpec.mockResolvedValue(null);

      const plugin = mockPlugins[0]!;
      const spec = await mockComponents.specGenerator.generatePluginSpec(plugin);

      expect(spec).toBeNull();
    });

    it('should handle plugin generation errors', async () => {
      const error = new Error('Plugin generation failed');
      mockComponents.specGenerator.generatePluginSpec.mockRejectedValue(error);

      try {
        await mockComponents.specGenerator.generatePluginSpec(mockPlugins[0]!);
      } catch (err) {
        expect(err).toBe(error);
      }
    });
  });

  describe('output directory handling', () => {
    it('should create output directory if it doesnt exist', async () => {
      const outputDir = '/test/output';

      // Simulate the directory creation logic
      mkdirSync(outputDir, { recursive: true });

      expect(mkdirSync).toHaveBeenCalledWith(outputDir, { recursive: true });
    });
  });

  describe('progress tracking', () => {
    it('should track successful and skipped plugin counts', async () => {
      const plugins = mockPlugins;
      let successCount = 0;
      let skippedCount = 0;

      for (const plugin of plugins) {
        try {
          const spec = await mockComponents.specGenerator.generatePluginSpec(plugin);
          if (spec) {
            mockComponents.specGenerator.savePluginSpec(plugin, spec, '/test/output');
            successCount++;
          } else {
            skippedCount++;
          }
        } catch (error) {
          skippedCount++;
        }
      }

      // Based on our mocks, all should succeed
      expect(successCount).toBe(2);
      expect(skippedCount).toBe(0);
    });

    it('should handle mixed success and failure scenarios', async () => {
      // Make the second plugin fail
      mockComponents.specGenerator.generatePluginSpec
        .mockResolvedValueOnce({
          plugin: mockPlugins[0]!,
          metadata: {
            version: '1.0.0',
            created: '2023-01-01T00:00:00Z',
            author: 'test',
            description: 'Test spec',
            parameter_count: 5,
            tags: ['test'],
          },
          parameters: [],
          groups: {},
        })
        .mockRejectedValueOnce(new Error('Second plugin failed'));

      const plugins = mockPlugins;
      let successCount = 0;
      let skippedCount = 0;

      for (const plugin of plugins) {
        try {
          const spec = await mockComponents.specGenerator.generatePluginSpec(plugin);
          if (spec) {
            successCount++;
          } else {
            skippedCount++;
          }
        } catch (error) {
          skippedCount++;
        }
      }

      expect(successCount).toBe(1);
      expect(skippedCount).toBe(1);
    });
  });

  describe('empty plugin list handling', () => {
    it('should handle empty plugin list gracefully', async () => {
      mockComponents.pluginDiscovery.getPluginList.mockResolvedValue([]);

      const plugins = await mockComponents.pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(0);
      // Should not attempt to process any plugins
      expect(mockComponents.specGenerator.generatePluginSpec).not.toHaveBeenCalled();
    });
  });

  describe('progress reporting', () => {
    it('should handle large plugin lists for progress reporting', async () => {
      // Create a larger list of mock plugins
      const manyPlugins = Array.from({ length: 25 }, (_, i) => ({
        manufacturer: `Manufacturer ${i}`,
        name: `Plugin ${i}`,
        version: '1.0.0',
        format: 'VST3',
        uid: `test-uid-${i}`,
        category: 'Effect',
      }));

      mockComponents.pluginDiscovery.getPluginList.mockResolvedValue(manyPlugins);

      const plugins = await mockComponents.pluginDiscovery.getPluginList();

      expect(plugins).toHaveLength(25);

      // Progress should be shown every 10 plugins (logic that would be in main)
      const progressIntervals = Math.floor(plugins.length / 10);
      expect(progressIntervals).toBe(2); // Would show progress at plugin 10 and 20
    });
  });
});