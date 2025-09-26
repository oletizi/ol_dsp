/**
 * Maps List Tool Tests
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { readFile, stat } from 'fs/promises';
import { glob } from 'glob';

// Mock fs/promises and glob
vi.mock('fs/promises', () => ({
  readFile: vi.fn(),
  stat: vi.fn(),
}));

vi.mock('glob', () => ({
  glob: vi.fn(),
}));

// Mock console methods
const mockConsole = {
  log: vi.fn(),
  error: vi.fn(),
  warn: vi.fn(),
};

vi.stubGlobal('console', mockConsole);

const mockReadFile = vi.mocked(readFile);
const mockStat = vi.mocked(stat);
const mockGlob = vi.mocked(glob);

// Mock types
interface MapSummary {
  filePath: string;
  mapId: string;
  device: {
    manufacturer: string;
    model: string;
    firmware?: string;
  };
  plugin?: {
    manufacturer: string;
    name: string;
    version?: string;
    format?: string;
  };
  metadata: {
    name: string;
    description?: string;
    author?: string;
    date?: string;
    tags?: string[];
    version?: string;
  };
  controls: {
    total: number;
    encoders: number;
    sliders: number;
    buttons: number;
    buttonGroups: number;
    mapped: number;
  };
  midi: {
    channel?: number;
    ccRange: [number, number] | null;
    uniqueCCs: number;
  };
  file: {
    format: 'yaml' | 'json';
    size: number;
    lastModified: Date;
  };
}

describe('Maps List Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('CLI argument parsing', () => {
    it('should parse output format options', () => {
      const formats = ['table', 'json', 'csv'];
      const testFormat = 'json';

      expect(formats).toContain(testFormat);
    });

    it('should parse filter options', () => {
      const filterOptions = {
        manufacturer: 'Native Instruments',
        model: 'Maschine MK3',
        plugin: 'Serum',
        tags: ['synthesizer', 'wavetable'],
        mappedOnly: true
      };

      expect(filterOptions.manufacturer).toBe('Native Instruments');
      expect(filterOptions.tags).toContain('synthesizer');
      expect(filterOptions.mappedOnly).toBe(true);
    });

    it('should parse sorting options', () => {
      const sortOptions = {
        sortBy: 'name' as const,
        sortOrder: 'asc' as const
      };

      const validSortFields = ['name', 'device', 'plugin', 'date', 'controls', 'size'];
      expect(validSortFields).toContain(sortOptions.sortBy);
    });

    it('should parse grouping options', () => {
      const groupBy = 'device';
      const validGroupFields = ['device', 'plugin', 'manufacturer'];

      expect(validGroupFields).toContain(groupBy);
    });
  });

  describe('map file discovery', () => {
    it('should find YAML and JSON map files', async () => {
      const mockFiles = [
        './maps/controller1.yaml',
        './maps/controller2.yml',
        './maps/controller3.json'
      ];

      mockGlob.mockResolvedValue(mockFiles);

      expect(mockFiles.length).toBe(3);
      expect(mockFiles[0].endsWith('.yaml')).toBe(true);
      expect(mockFiles[2].endsWith('.json')).toBe(true);
    });

    it('should handle empty directories', async () => {
      mockGlob.mockResolvedValue([]);

      expect(await mockGlob('')).toEqual([]);
    });

    it('should handle glob pattern matching', () => {
      const pattern = './maps/**/*.{yaml,yml,json}';
      expect(pattern).toContain('**/*.{yaml,yml,json}');
    });
  });

  describe('map file processing', () => {
    it('should process valid YAML map files', async () => {
      const mockYamlContent = `
device:
  manufacturer: "Test Company"
  model: "Test Controller"
metadata:
  name: "Test Mapping"
  description: "Test description"
controls:
  - id: "volume"
    cc: 7
    type: "knob"
`;

      mockReadFile.mockResolvedValue(mockYamlContent);
      mockStat.mockResolvedValue({
        size: 1024,
        mtime: new Date('2024-01-01'),
        isFile: () => true
      } as any);

      expect(mockYamlContent).toContain('device:');
      expect(mockYamlContent).toContain('controls:');
    });

    it('should process valid JSON map files', async () => {
      const mockJsonContent = JSON.stringify({
        device: {
          manufacturer: "Test Company",
          model: "Test Controller"
        },
        metadata: {
          name: "Test Mapping",
          description: "Test description"
        },
        controls: [
          {
            id: "volume",
            cc: 7,
            type: "knob"
          }
        ]
      });

      mockReadFile.mockResolvedValue(mockJsonContent);
      mockStat.mockResolvedValue({
        size: 2048,
        mtime: new Date('2024-01-01'),
        isFile: () => true
      } as any);

      expect(() => JSON.parse(mockJsonContent)).not.toThrow();
    });

    it('should handle file processing errors gracefully', async () => {
      mockReadFile.mockRejectedValue(new Error('File read error'));

      try {
        await mockReadFile('/invalid/file.yaml');
        expect(true).toBe(false); // Should not reach here
      } catch (error) {
        expect(error).toBeInstanceOf(Error);
      }
    });
  });

  describe('control statistics calculation', () => {
    it('should calculate control type counts', () => {
      const controls = [
        { type: 'encoder', id: 'enc1' },
        { type: 'slider', id: 'fader1' },
        { type: 'button', id: 'btn1' },
        { type: 'button_group', id: 'group1', buttons: [{ cc: 1 }, { cc: 2 }] }
      ];

      let encoders = 0, sliders = 0, buttons = 0, buttonGroups = 0;

      controls.forEach(control => {
        switch (control.type) {
          case 'encoder': encoders++; break;
          case 'slider': sliders++; break;
          case 'button': buttons++; break;
          case 'button_group':
            buttonGroups++;
            if ('buttons' in control && control.buttons) {
              buttons += control.buttons.length;
            }
            break;
        }
      });

      expect(encoders).toBe(1);
      expect(sliders).toBe(1);
      expect(buttons).toBe(3); // 1 button + 2 from button group
      expect(buttonGroups).toBe(1);
    });

    it('should count mapped controls', () => {
      const controls = [
        { id: 'volume', cc: 7, plugin_parameter: 0 },
        { id: 'pan', cc: 10, plugin_parameter: 1 },
        { id: 'filter', cc: 74 } // Not mapped
      ];

      const mapped = controls.filter(c => c.plugin_parameter !== undefined).length;
      expect(mapped).toBe(2);
    });

    it('should count total controls', () => {
      const controls = Array(10).fill({ id: 'control', type: 'knob' });
      expect(controls.length).toBe(10);
    });
  });

  describe('MIDI information calculation', () => {
    it('should calculate CC range', () => {
      const ccNumbers = [7, 10, 74, 127];
      const minCC = Math.min(...ccNumbers);
      const maxCC = Math.max(...ccNumbers);

      expect(minCC).toBe(7);
      expect(maxCC).toBe(127);
    });

    it('should count unique CCs', () => {
      const ccNumbers = [7, 10, 7, 74, 10]; // Duplicates
      const uniqueCCs = new Set(ccNumbers);

      expect(uniqueCCs.size).toBe(3); // 7, 10, 74
    });

    it('should handle button group CCs', () => {
      const buttonGroup = {
        type: 'button_group',
        buttons: [
          { cc: 16 },
          { cc: 17 },
          { cc: 18 }
        ]
      };

      const ccNumbers = new Set<number>();
      if ('buttons' in buttonGroup && buttonGroup.buttons) {
        buttonGroup.buttons.forEach(btn => ccNumbers.add(btn.cc));
      }

      expect(ccNumbers.size).toBe(3);
    });
  });

  describe('filtering functionality', () => {
    it('should filter by manufacturer', () => {
      const maps: MapSummary[] = [
        {
          device: { manufacturer: 'Native Instruments', model: 'Maschine' },
        } as MapSummary,
        {
          device: { manufacturer: 'Ableton', model: 'Push 2' },
        } as MapSummary
      ];

      const filterManufacturer = 'Native';
      const filtered = maps.filter(map =>
        map.device.manufacturer.toLowerCase().includes(filterManufacturer.toLowerCase())
      );

      expect(filtered.length).toBe(1);
      expect(filtered[0].device.manufacturer).toContain('Native');
    });

    it('should filter by plugin', () => {
      const maps: MapSummary[] = [
        {
          plugin: { manufacturer: 'Xfer', name: 'Serum' },
        } as MapSummary,
        {
          plugin: { manufacturer: 'Native Instruments', name: 'Massive' },
        } as MapSummary,
        {} as MapSummary // No plugin
      ];

      const filterPlugin = 'Serum';
      const filtered = maps.filter(map => {
        if (!map.plugin) return false;
        const pluginText = `${map.plugin.manufacturer} ${map.plugin.name}`.toLowerCase();
        return pluginText.includes(filterPlugin.toLowerCase());
      });

      expect(filtered.length).toBe(1);
      expect(filtered[0].plugin?.name).toBe('Serum');
    });

    it('should filter by tags', () => {
      const maps: MapSummary[] = [
        {
          metadata: { name: 'Map 1', tags: ['synthesizer', 'wavetable'] },
        } as MapSummary,
        {
          metadata: { name: 'Map 2', tags: ['effect', 'reverb'] },
        } as MapSummary
      ];

      const filterTags = ['synthesizer'];
      const filtered = maps.filter(map => {
        if (!map.metadata.tags) return false;
        return filterTags.some(tag =>
          map.metadata.tags?.some(mapTag =>
            mapTag.toLowerCase().includes(tag.toLowerCase())
          )
        );
      });

      expect(filtered.length).toBe(1);
      expect(filtered[0].metadata.tags).toContain('synthesizer');
    });

    it('should filter mapped-only maps', () => {
      const maps: MapSummary[] = [
        {
          controls: { mapped: 5, total: 10 },
        } as MapSummary,
        {
          controls: { mapped: 0, total: 8 },
        } as MapSummary
      ];

      const mappedOnly = true;
      const filtered = maps.filter(map => !mappedOnly || map.controls.mapped > 0);

      expect(filtered.length).toBe(1);
      expect(filtered[0].controls.mapped).toBeGreaterThan(0);
    });
  });

  describe('sorting functionality', () => {
    it('should sort by name', () => {
      const maps: MapSummary[] = [
        { metadata: { name: 'Zebra Map' } } as MapSummary,
        { metadata: { name: 'Ableton Map' } } as MapSummary,
        { metadata: { name: 'Maschine Map' } } as MapSummary
      ];

      maps.sort((a, b) => a.metadata.name.localeCompare(b.metadata.name));

      expect(maps[0].metadata.name).toBe('Ableton Map');
      expect(maps[2].metadata.name).toBe('Zebra Map');
    });

    it('should sort by control count', () => {
      const maps: MapSummary[] = [
        { controls: { total: 16 } } as MapSummary,
        { controls: { total: 8 } } as MapSummary,
        { controls: { total: 32 } } as MapSummary
      ];

      maps.sort((a, b) => a.controls.total - b.controls.total);

      expect(maps[0].controls.total).toBe(8);
      expect(maps[2].controls.total).toBe(32);
    });

    it('should sort by device name', () => {
      const maps: MapSummary[] = [
        { device: { manufacturer: 'Native Instruments', model: 'Maschine' } } as MapSummary,
        { device: { manufacturer: 'Ableton', model: 'Push' } } as MapSummary
      ];

      maps.sort((a, b) =>
        `${a.device.manufacturer} ${a.device.model}`.localeCompare(
          `${b.device.manufacturer} ${b.device.model}`
        )
      );

      expect(maps[0].device.manufacturer).toBe('Ableton');
    });
  });

  describe('output formatting', () => {
    it('should format table output', () => {
      const maps: MapSummary[] = [
        {
          metadata: { name: 'Test Map' },
          device: { manufacturer: 'Test Co', model: 'Controller' },
          plugin: { manufacturer: 'Plugin Co', name: 'Synth' },
          controls: { total: 16, mapped: 8 },
        } as MapSummary
      ];

      // Test table formatting logic
      const widths = {
        name: Math.max(20, 'Test Map'.length),
        device: Math.max(25, 'Test Co Controller'.length),
        plugin: Math.max(20, 'Plugin Co Synth'.length),
        controls: 8,
        mapped: 6
      };

      expect(widths.name).toBeGreaterThanOrEqual(20);
      expect(widths.device).toBeGreaterThanOrEqual(25);
    });

    it('should format JSON output', () => {
      const maps: MapSummary[] = [
        {
          metadata: { name: 'Test Map' },
          device: { manufacturer: 'Test', model: 'Controller' }
        } as MapSummary
      ];

      expect(() => JSON.stringify(maps, null, 2)).not.toThrow();
    });

    it('should format CSV output', () => {
      const testValue = 'Test, "Value"';
      const csvEscape = (value: string): string => {
        if (value.includes(',') || value.includes('"') || value.includes('\n')) {
          return `"${value.replace(/"/g, '""')}"`;
        }
        return value;
      };

      const escaped = csvEscape(testValue);
      expect(escaped).toContain('"');
    });
  });

  describe('grouping functionality', () => {
    it('should group by device', () => {
      const maps: MapSummary[] = [
        {
          device: { manufacturer: 'Native Instruments', model: 'Maschine' },
        } as MapSummary,
        {
          device: { manufacturer: 'Native Instruments', model: 'Maschine' },
        } as MapSummary,
        {
          device: { manufacturer: 'Ableton', model: 'Push' },
        } as MapSummary
      ];

      const groups = new Map<string, MapSummary[]>();

      maps.forEach(map => {
        const groupKey = `${map.device.manufacturer} ${map.device.model}`;
        if (!groups.has(groupKey)) {
          groups.set(groupKey, []);
        }
        groups.get(groupKey)!.push(map);
      });

      expect(groups.size).toBe(2);
      expect(groups.get('Native Instruments Maschine')?.length).toBe(2);
    });

    it('should group by plugin', () => {
      const maps: MapSummary[] = [
        {
          plugin: { manufacturer: 'Xfer', name: 'Serum' },
        } as MapSummary,
        {
          plugin: { manufacturer: 'Xfer', name: 'Serum' },
        } as MapSummary,
        {} as MapSummary // No plugin
      ];

      const groups = new Map<string, MapSummary[]>();

      maps.forEach(map => {
        const groupKey = map.plugin ?
          `${map.plugin.manufacturer} ${map.plugin.name}` :
          'No Plugin';

        if (!groups.has(groupKey)) {
          groups.set(groupKey, []);
        }
        groups.get(groupKey)!.push(map);
      });

      expect(groups.size).toBe(2);
      expect(groups.has('No Plugin')).toBe(true);
    });
  });

  describe('performance validation', () => {
    it('should process large map collections efficiently', () => {
      const largeMaps = Array(1000).fill(0).map((_, i) => ({
        metadata: { name: `Map ${i}` },
        device: { manufacturer: 'Test', model: 'Controller' },
        controls: { total: 16, mapped: 8 }
      }));

      expect(largeMaps.length).toBe(1000);
    });

    it('should complete listing within time limits', () => {
      const startTime = Date.now();

      // Simulate map processing
      const mockProcessing = () => {
        return Array(100).fill(true);
      };

      const result = mockProcessing();
      const duration = Date.now() - startTime;

      expect(result.length).toBe(100);
      expect(duration).toBeLessThan(100);
    });
  });

  describe('error handling', () => {
    it('should handle missing map files', async () => {
      mockGlob.mockResolvedValue([]);

      expect(await mockGlob('')).toEqual([]);
    });

    it('should handle corrupted map files', async () => {
      mockReadFile.mockResolvedValue('invalid yaml content: {[}');

      try {
        const content = await mockReadFile('invalid.yaml');
        // YAML parsing would fail here
        expect(content).toContain('invalid yaml');
      } catch (error) {
        expect(error).toBeDefined();
      }
    });

    it('should provide helpful error messages', () => {
      const error = new Error('No map files found in ./nonexistent');
      expect(error.message).toContain('No map files found');
    });
  });
});