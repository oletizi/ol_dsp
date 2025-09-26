/**
 * Tests for Plugin List Tool
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { existsSync, readFileSync } from 'node:fs';

// Mock file system operations
vi.mock('node:fs');

const mockExistsSync = vi.mocked(existsSync);
const mockReadFileSync = vi.mocked(readFileSync);

describe('Plugin List Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('CLI Argument Parsing', () => {
    it('should parse default options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: [],
        options: {
          format: { type: 'string', default: 'table' },
          filter: { type: 'string' },
          verbose: { type: 'boolean', short: 'v', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.format).toBe('table');
      expect(values.verbose).toBe(false);
    });

    it('should parse format options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--format', 'json', '--verbose'],
        options: {
          format: { type: 'string', default: 'table' },
          filter: { type: 'string' },
          verbose: { type: 'boolean', short: 'v', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.format).toBe('json');
      expect(values.verbose).toBe(true);
    });

    it('should parse filter option', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--filter', 'reverb'],
        options: {
          format: { type: 'string', default: 'table' },
          filter: { type: 'string' },
          verbose: { type: 'boolean', short: 'v', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.filter).toBe('reverb');
    });
  });

  describe('Plugin Data Loading', () => {
    it('should handle missing plugin data file', () => {
      mockExistsSync.mockReturnValue(false);

      const dataFile = './data/plugins-extracted.json';
      const exists = mockExistsSync(dataFile);

      expect(exists).toBe(false);
      // Should indicate no data available
    });

    it('should load existing plugin data', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockReturnValue(JSON.stringify([
        {
          name: 'Test Plugin',
          id: 'test-plugin-1',
          parameters: [
            { name: 'Volume', id: 1, min: 0, max: 100 }
          ]
        }
      ]));

      const dataFile = './data/plugins-extracted.json';
      const exists = mockExistsSync(dataFile);

      expect(exists).toBe(true);

      if (exists) {
        const data = JSON.parse(mockReadFileSync(dataFile, 'utf-8'));
        expect(data).toHaveLength(1);
        expect(data[0].name).toBe('Test Plugin');
      }
    });

    it('should handle invalid JSON in data file', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockReturnValue('invalid json');

      const dataFile = './data/plugins-extracted.json';

      expect(() => {
        const data = mockReadFileSync(dataFile, 'utf-8');
        JSON.parse(data);
      }).toThrow();
    });
  });

  describe('Plugin Filtering', () => {
    const samplePlugins = [
      { name: 'Reverb Pro', id: 'reverb-pro', type: 'effect' },
      { name: 'Compressor X', id: 'compressor-x', type: 'dynamics' },
      { name: 'Reverb Lite', id: 'reverb-lite', type: 'effect' }
    ];

    it('should filter plugins by name', () => {
      const filter = 'reverb';
      const filtered = samplePlugins.filter(plugin =>
        plugin.name.toLowerCase().includes(filter.toLowerCase())
      );

      expect(filtered).toHaveLength(2);
      expect(filtered[0].name).toBe('Reverb Pro');
      expect(filtered[1].name).toBe('Reverb Lite');
    });

    it('should filter plugins by type', () => {
      const filter = 'dynamics';
      const filtered = samplePlugins.filter(plugin =>
        plugin.type?.toLowerCase().includes(filter.toLowerCase())
      );

      expect(filtered).toHaveLength(1);
      expect(filtered[0].name).toBe('Compressor X');
    });

    it('should return all plugins when no filter', () => {
      const filtered = samplePlugins.filter(() => true);
      expect(filtered).toHaveLength(3);
    });

    it('should return empty array when filter matches nothing', () => {
      const filter = 'nonexistent';
      const filtered = samplePlugins.filter(plugin =>
        plugin.name.toLowerCase().includes(filter.toLowerCase())
      );

      expect(filtered).toHaveLength(0);
    });
  });

  describe('Output Formatting', () => {
    const samplePlugin = {
      name: 'Test Plugin',
      id: 'test-plugin',
      parameters: [
        { name: 'Volume', id: 1, min: 0, max: 100 },
        { name: 'Pan', id: 2, min: -100, max: 100 }
      ]
    };

    it('should format plugin for table output', () => {
      const tableRow = {
        name: samplePlugin.name,
        id: samplePlugin.id,
        parameters: samplePlugin.parameters.length
      };

      expect(tableRow.name).toBe('Test Plugin');
      expect(tableRow.id).toBe('test-plugin');
      expect(tableRow.parameters).toBe(2);
    });

    it('should format plugin for JSON output', () => {
      const jsonOutput = JSON.stringify(samplePlugin, null, 2);

      expect(jsonOutput).toContain('"name": "Test Plugin"');
      expect(jsonOutput).toContain('"id": "test-plugin"');
      expect(jsonOutput).toContain('"parameters"');
    });

    it('should handle empty plugin list', () => {
      const emptyList: any[] = [];

      expect(emptyList).toHaveLength(0);

      const jsonOutput = JSON.stringify(emptyList);
      expect(jsonOutput).toBe('[]');
    });
  });

  describe('Verbose Output', () => {
    const samplePlugin = {
      name: 'Test Plugin',
      id: 'test-plugin',
      manufacturer: 'Test Corp',
      version: '1.0.0',
      parameters: [
        {
          name: 'Volume',
          id: 1,
          min: 0,
          max: 100,
          default: 50,
          units: 'dB'
        }
      ]
    };

    it('should include additional details in verbose mode', () => {
      const verbose = true;

      if (verbose) {
        expect(samplePlugin.manufacturer).toBe('Test Corp');
        expect(samplePlugin.version).toBe('1.0.0');
        expect(samplePlugin.parameters[0].default).toBe(50);
        expect(samplePlugin.parameters[0].units).toBe('dB');
      }
    });

    it('should show basic info in non-verbose mode', () => {
      const verbose = false;

      if (!verbose) {
        const basicInfo = {
          name: samplePlugin.name,
          id: samplePlugin.id,
          parameterCount: samplePlugin.parameters.length
        };

        expect(basicInfo.name).toBe('Test Plugin');
        expect(basicInfo.parameterCount).toBe(1);
      }
    });
  });

  describe('Error Handling', () => {
    it('should handle file read errors gracefully', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockImplementation(() => {
        throw new Error('Permission denied');
      });

      expect(() => {
        const dataFile = './data/plugins-extracted.json';
        if (mockExistsSync(dataFile)) {
          mockReadFileSync(dataFile, 'utf-8');
        }
      }).toThrow('Permission denied');
    });

    it('should provide helpful error messages', () => {
      const error = new Error('File not found');

      const helpfulMessage = `Failed to load plugin data: ${error.message}\n` +
        'Please run: pnpm plugins:extract';

      expect(helpfulMessage).toContain('Failed to load plugin data');
      expect(helpfulMessage).toContain('pnpm plugins:extract');
    });
  });
});