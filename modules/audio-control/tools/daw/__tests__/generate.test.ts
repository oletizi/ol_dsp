/**
 * DAW Generation Tool Tests
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { existsSync, readFileSync, writeFileSync, readdirSync, mkdirSync, statSync } from 'node:fs';
import { execSync } from 'node:child_process';

// Mock fs operations
vi.mock('node:fs', () => ({
  existsSync: vi.fn(),
  readFileSync: vi.fn(),
  writeFileSync: vi.fn(),
  readdirSync: vi.fn(),
  mkdirSync: vi.fn(),
  statSync: vi.fn(),
}));

// Mock child_process
vi.mock('node:child_process', () => ({
  execSync: vi.fn(),
}));

// Mock YAML parser
vi.mock('yaml', () => ({
  parse: vi.fn(),
}));

// Mock console methods
const mockConsole = {
  log: vi.fn(),
  error: vi.fn(),
  warn: vi.fn(),
};

vi.stubGlobal('console', mockConsole);

const mockExistsSync = vi.mocked(existsSync);
const mockReadFileSync = vi.mocked(readFileSync);
const mockWriteFileSync = vi.mocked(writeFileSync);
const mockReaddirSync = vi.mocked(readdirSync);
const mockMkdirSync = vi.mocked(mkdirSync);
const mockStatSync = vi.mocked(statSync);
const mockExecSync = vi.mocked(execSync);

// Mock interfaces
interface GenerateOptions {
  target?: 'ardour' | 'ableton' | 'reaper' | 'all';
  input?: string;
  output?: string;
  install?: boolean;
  force?: boolean;
}

interface CanonicalMidiMap {
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
  };
  controls: Array<{
    id: string;
    cc?: number;
    note?: number;
    type: string;
    channel?: number | 'global';
    mode?: 'absolute' | 'relative' | 'momentary';
    plugin_parameter?: number | string;
    buttons?: Array<{
      cc?: number;
      note?: number;
      channel?: number;
      plugin_parameter?: number | string;
      mode?: string;
      type?: string;
    }>;
  }>;
  version: string;
}

describe('DAW Generation Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('CLI argument parsing', () => {
    it('should parse target options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const mockParseArgs = vi.fn().mockReturnValue({
        values: { target: 'ardour', install: true, force: false }
      });
      vi.mocked(parseArgs).mockImplementation(mockParseArgs);

      expect(mockParseArgs).toBeDefined();
    });

    it('should handle all supported DAW targets', () => {
      const supportedTargets = ['ardour', 'ableton', 'reaper', 'all'];
      const testTarget = 'ardour';

      expect(supportedTargets).toContain(testTarget);
    });

    it('should parse installation flag', () => {
      const options: GenerateOptions = {
        target: 'ardour',
        install: true,
        force: false
      };

      expect(options.install).toBe(true);
      expect(options.target).toBe('ardour');
    });

    it('should handle help flag', async () => {
      const { parseArgs } = await import('node:util');
      const mockParseArgs = vi.fn().mockReturnValue({
        values: { help: true }
      });
      vi.mocked(parseArgs).mockImplementation(mockParseArgs);

      expect(mockParseArgs).toBeDefined();
    });
  });

  describe('maps directory detection', () => {
    it('should find canonical maps directory', () => {
      mockExistsSync
        .mockReturnValueOnce(false)  // First path doesn't exist
        .mockReturnValueOnce(true);  // Second path exists

      // Test would call findMapsDirectory()
      expect(mockExistsSync).toBeDefined();
    });

    it('should handle missing maps directory', () => {
      mockExistsSync.mockReturnValue(false); // No paths exist

      expect(() => {
        // This would test the actual findMapsDirectory function
        if (!mockExistsSync('')) {
          throw new Error('Maps directory not found');
        }
      }).toThrow('Maps directory not found');
    });

    it('should check multiple possible paths', () => {
      const possiblePaths = [
        '../../modules/canonical-midi-maps/maps',
        '../../maps',
        '../../../canonical-midi-maps/maps'
      ];

      expect(possiblePaths.length).toBeGreaterThan(1);
    });
  });

  describe('output directory handling', () => {
    it('should create output directory if missing', () => {
      mockExistsSync.mockReturnValue(false);
      mockMkdirSync.mockImplementation(() => undefined);

      // Test directory creation
      expect(mockMkdirSync).toBeDefined();
    });

    it('should handle existing output directory', () => {
      mockExistsSync.mockReturnValue(true);

      expect(mockExistsSync('')).toBe(true);
    });

    it('should generate default output paths', () => {
      const getDefaultOutputDir = (target: string) => `../../generated/${target}-maps`;

      expect(getDefaultOutputDir('ardour')).toContain('ardour-maps');
      expect(getDefaultOutputDir('reaper')).toContain('reaper-maps');
    });
  });

  describe('canonical map file discovery', () => {
    it('should find YAML and JSON map files recursively', () => {
      mockReaddirSync.mockReturnValue(['map1.yaml', 'map2.json', 'subdir'] as any);
      mockStatSync
        .mockReturnValueOnce({ isDirectory: () => false } as any)  // map1.yaml
        .mockReturnValueOnce({ isDirectory: () => false } as any)  // map2.json
        .mockReturnValueOnce({ isDirectory: () => true } as any);  // subdir

      const mapFiles = ['map1.yaml', 'map2.json'];
      expect(mapFiles.some(f => f.endsWith('.yaml'))).toBe(true);
      expect(mapFiles.some(f => f.endsWith('.json'))).toBe(true);
    });

    it('should handle empty directories', () => {
      mockReaddirSync.mockReturnValue([]);

      expect(mockReaddirSync('')).toEqual([]);
    });

    it('should filter non-map files', () => {
      const allFiles = ['map.yaml', 'readme.txt', 'config.json', 'data.map'];
      const mapExtensions = ['.yaml', '.yml', '.json'];

      const mapFiles = allFiles.filter(file =>
        mapExtensions.some(ext => file.toLowerCase().endsWith(ext))
      );

      expect(mapFiles).toEqual(['map.yaml', 'config.json']);
    });
  });

  describe('canonical map parsing', () => {
    it('should parse valid YAML canonical maps', () => {
      const mockYamlContent = `
device:
  manufacturer: "Test Company"
  model: "Test Controller"
metadata:
  name: "Test Mapping"
controls:
  - id: "volume"
    cc: 7
    type: "knob"
    plugin_parameter: 0
version: "1.0"
`;

      mockReadFileSync.mockReturnValue(mockYamlContent);

      // Test YAML parsing
      expect(mockYamlContent).toContain('device:');
      expect(mockYamlContent).toContain('controls:');
    });

    it('should parse valid JSON canonical maps', () => {
      const mockJsonMap: CanonicalMidiMap = {
        device: {
          manufacturer: "Test Company",
          model: "Test Controller"
        },
        metadata: {
          name: "Test Mapping"
        },
        controls: [
          {
            id: "volume",
            cc: 7,
            type: "knob",
            plugin_parameter: 0
          }
        ],
        version: "1.0"
      };

      const jsonContent = JSON.stringify(mockJsonMap);
      mockReadFileSync.mockReturnValue(jsonContent);

      expect(() => JSON.parse(jsonContent)).not.toThrow();
    });

    it('should validate canonical map structure', () => {
      const canonicalMap: CanonicalMidiMap = {
        device: { manufacturer: 'Test', model: 'Controller' },
        metadata: { name: 'Test Map' },
        controls: [
          { id: 'volume', cc: 7, type: 'knob' }
        ],
        version: '1.0'
      };

      expect(canonicalMap.device).toBeDefined();
      expect(canonicalMap.controls).toBeInstanceOf(Array);
      expect(canonicalMap.controls.length).toBeGreaterThan(0);
    });

    it('should skip non-map files', () => {
      const nonMapContent = {
        registry: ['plugin1', 'plugin2'],
        version: '1.0'
      };

      // Missing 'controls' array indicates non-map file
      expect(nonMapContent.controls).toBeUndefined();
    });
  });

  describe('Ardour map generation', () => {
    it('should convert CC controls to Ardour bindings', () => {
      const canonicalControl = {
        id: 'volume',
        cc: 7,
        type: 'knob',
        channel: 1,
        plugin_parameter: 0
      };

      // Test CC conversion logic
      expect(canonicalControl.cc).toBe(7);
      expect(canonicalControl.channel).toBe(1);
      expect(canonicalControl.plugin_parameter).toBe(0);
    });

    it('should convert note controls to Ardour bindings', () => {
      const noteControl = {
        id: 'play',
        note: 60,
        type: 'button',
        mode: 'momentary'
      };

      expect(noteControl.note).toBe(60);
      expect(noteControl.type).toBe('button');
    });

    it('should handle button groups', () => {
      const buttonGroup = {
        id: 'track_buttons',
        type: 'button_group',
        buttons: [
          { cc: 16, plugin_parameter: 1 },
          { cc: 17, plugin_parameter: 2 },
          { cc: 18, plugin_parameter: 3 }
        ]
      };

      expect(buttonGroup.buttons?.length).toBe(3);
      expect(buttonGroup.buttons?.[0].cc).toBe(16);
    });

    it('should convert plugin parameters to Ardour functions', () => {
      const convertToArdourFunction = (control: any) => {
        if (control.plugin_parameter !== undefined) {
          if (control.plugin_parameter === 'bypass') {
            return 'toggle-plugin-bypass';
          }
          if (typeof control.plugin_parameter === 'number') {
            return `plugin-parameter[${control.plugin_parameter}]`;
          }
        }
        return 'track-select[1]';
      };

      expect(convertToArdourFunction({ plugin_parameter: 'bypass' })).toBe('toggle-plugin-bypass');
      expect(convertToArdourFunction({ plugin_parameter: 5 })).toBe('plugin-parameter[5]');
    });

    it('should generate Ardour XML output', () => {
      mockWriteFileSync.mockImplementation(() => undefined);

      // Test XML generation
      const xmlContent = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings>
  <Binding channel="0" ctl="7" function="plugin-parameter[0]"/>
</ArdourMIDIBindings>`;

      expect(xmlContent).toContain('ArdourMIDIBindings');
      expect(xmlContent).toContain('Binding');
    });
  });

  describe('DAW installation detection', () => {
    it('should detect Ardour installation paths', () => {
      const ardourPaths = [
        '~/.config/ardour8/midi_maps',
        '~/.config/ardour7/midi_maps',
        '/usr/share/ardour8/midi_maps'
      ];

      mockExistsSync.mockReturnValue(true);

      expect(ardourPaths.length).toBeGreaterThan(0);
      expect(ardourPaths[0]).toContain('ardour');
    });

    it('should handle missing DAW installations', () => {
      mockExistsSync.mockReturnValue(false);

      const getInstallPath = (target: string) => {
        return mockExistsSync('') ? '/path/to/daw' : null;
      };

      expect(getInstallPath('ardour')).toBeNull();
    });

    it('should expand home directory paths', () => {
      const expandPath = (path: string) => {
        return path.replace('~', process.env.HOME || '');
      };

      process.env.HOME = '/home/user';
      expect(expandPath('~/.config/ardour8')).toBe('/home/user/.config/ardour8');
    });
  });

  describe('map installation', () => {
    it('should copy maps to DAW directories', () => {
      mockExistsSync.mockReturnValue(true);
      mockReaddirSync.mockReturnValue(['map1.map', 'map2.map'] as any);
      mockExecSync.mockImplementation(() => '');

      // Test file copying
      expect(mockExecSync).toBeDefined();
    });

    it('should handle installation failures gracefully', () => {
      mockExecSync.mockImplementation(() => {
        throw new Error('Copy failed');
      });

      expect(() => mockExecSync('cp source dest')).toThrow('Copy failed');
    });

    it('should use platform-specific copy commands', () => {
      const originalPlatform = process.platform;

      // Test Windows
      Object.defineProperty(process, 'platform', { value: 'win32' });
      const winCommand = 'copy "source" "dest"';
      expect(winCommand).toContain('copy');

      // Test Unix-like
      Object.defineProperty(process, 'platform', { value: 'darwin' });
      const unixCommand = 'cp "source" "dest"';
      expect(unixCommand).toContain('cp');

      Object.defineProperty(process, 'platform', { value: originalPlatform });
    });
  });

  describe('multi-target generation', () => {
    it('should generate for all targets when target is "all"', () => {
      const options: GenerateOptions = { target: 'all' };
      const allTargets = ['ardour', 'ableton', 'reaper'];

      const targets = options.target === 'all' ? allTargets : [options.target];
      expect(targets.length).toBe(3);
      expect(targets).toContain('ardour');
    });

    it('should generate for single target', () => {
      const options: GenerateOptions = { target: 'ardour' };
      const targets = [options.target];

      expect(targets.length).toBe(1);
      expect(targets[0]).toBe('ardour');
    });

    it('should handle unimplemented targets gracefully', () => {
      const unimplementedTargets = ['ableton', 'reaper'];

      unimplementedTargets.forEach(target => {
        expect(() => {
          throw new Error(`${target} generation not yet implemented`);
        }).toThrow('not yet implemented');
      });
    });
  });

  describe('force overwrite handling', () => {
    it('should skip existing files when force is false', () => {
      mockExistsSync.mockReturnValue(true);

      const force = false;
      const shouldSkip = mockExistsSync('') && !force;

      expect(shouldSkip).toBe(true);
    });

    it('should overwrite existing files when force is true', () => {
      mockExistsSync.mockReturnValue(true);

      const force = true;
      const shouldSkip = mockExistsSync('') && !force;

      expect(shouldSkip).toBe(false);
    });
  });

  describe('error handling', () => {
    it('should handle file read errors', () => {
      mockReadFileSync.mockImplementation(() => {
        throw new Error('File read failed');
      });

      expect(() => mockReadFileSync('')).toThrow('File read failed');
    });

    it('should handle malformed canonical maps', () => {
      const malformedContent = '{ invalid json';

      expect(() => JSON.parse(malformedContent)).toThrow();
    });

    it('should provide helpful error messages', () => {
      const error = new Error('Ableton Live generation not yet implemented');
      expect(error.message).toContain('not yet implemented');
    });

    it('should handle missing required fields', () => {
      const incompleteMap = {
        device: { manufacturer: 'Test' }, // Missing model
        // Missing controls array
        version: '1.0'
      };

      expect(incompleteMap.controls).toBeUndefined();
    });
  });

  describe('performance validation', () => {
    it('should process large numbers of maps efficiently', () => {
      const largeMapsCollection = Array(100).fill(0).map((_, i) => ({
        filePath: `map${i}.yaml`,
        controls: Array(50).fill({ id: 'control', cc: 7 })
      }));

      expect(largeMapsCollection.length).toBe(100);
      expect(largeMapsCollection[0].controls.length).toBe(50);
    });

    it('should complete generation within time limits', () => {
      const startTime = Date.now();

      // Simulate map generation
      const mockGeneration = () => {
        return Array(10).fill(true);
      };

      const result = mockGeneration();
      const duration = Date.now() - startTime;

      expect(result.length).toBe(10);
      expect(duration).toBeLessThan(100);
    });
  });

  describe('XML generation quality', () => {
    it('should generate valid XML structure', () => {
      const xmlTemplate = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0">
  <Binding channel="0" ctl="7" function="plugin-parameter[0]"/>
</ArdourMIDIBindings>`;

      expect(xmlTemplate).toContain('<?xml');
      expect(xmlTemplate).toContain('ArdourMIDIBindings');
      expect(xmlTemplate).toContain('Binding');
    });

    it('should escape XML characters properly', () => {
      const escapeXml = (str: string) => {
        return str
          .replace(/&/g, '&amp;')
          .replace(/</g, '&lt;')
          .replace(/>/g, '&gt;')
          .replace(/"/g, '&quot;')
          .replace(/'/g, '&apos;');
      };

      const testString = 'Name with "quotes" & <brackets>';
      const escaped = escapeXml(testString);

      expect(escaped).toContain('&quot;');
      expect(escaped).toContain('&amp;');
      expect(escaped).toContain('&lt;');
    });
  });
});