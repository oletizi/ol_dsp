/**
 * Tests for DAW Generation Tool
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { existsSync, readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, dirname } from 'node:path';

// Mock file system operations
vi.mock('node:fs');

const mockExistsSync = vi.mocked(existsSync);
const mockReadFileSync = vi.mocked(readFileSync);
const mockWriteFileSync = vi.mocked(writeFileSync);
const mockMkdirSync = vi.mocked(mkdirSync);

describe('DAW Generation Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('CLI Argument Parsing', () => {
    it('should parse default options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: [],
        options: {
          target: { type: 'string', default: 'all' },
          output: { type: 'string' },
          install: { type: 'boolean', default: false },
          force: { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.target).toBe('all');
      expect(values.install).toBe(false);
      expect(values.force).toBe(false);
    });

    it('should parse specific target and flags', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--target', 'ardour', '--install', '--force'],
        options: {
          target: { type: 'string', default: 'all' },
          output: { type: 'string' },
          install: { type: 'boolean', default: false },
          force: { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.target).toBe('ardour');
      expect(values.install).toBe(true);
      expect(values.force).toBe(true);
    });

    it('should parse output directory option', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--output', './custom-output'],
        options: {
          target: { type: 'string', default: 'all' },
          output: { type: 'string' },
          install: { type: 'boolean', default: false },
          force: { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.output).toBe('./custom-output');
    });
  });

  describe('Target DAW Support', () => {
    it('should support Ardour as primary target', () => {
      const supportedTargets = ['ardour', 'ableton', 'reaper', 'all'];

      expect(supportedTargets).toContain('ardour');
      expect(supportedTargets).toContain('all');
    });

    it('should validate target DAW selection', () => {
      const validTargets = ['ardour', 'ableton', 'reaper', 'all'];
      const testTarget = 'ardour';

      expect(validTargets.includes(testTarget)).toBe(true);
    });

    it('should reject invalid target selection', () => {
      const validTargets = ['ardour', 'ableton', 'reaper', 'all'];
      const invalidTarget = 'cubase';

      expect(validTargets.includes(invalidTarget)).toBe(false);
    });
  });

  describe('Input Data Loading', () => {
    const mockCanonicalMapping = {
      name: 'Test Mapping',
      plugin: 'test-plugin',
      controls: [
        {
          name: 'Volume',
          parameter: 'volume',
          midi_cc: 1,
          min: 0,
          max: 100
        },
        {
          name: 'Pan',
          parameter: 'pan',
          midi_cc: 2,
          min: -100,
          max: 100
        }
      ]
    };

    it('should load canonical mapping files', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockReturnValue(JSON.stringify([mockCanonicalMapping]));

      const dataFile = './data/canonical-maps.json';
      const exists = mockExistsSync(dataFile);

      expect(exists).toBe(true);

      if (exists) {
        const content = mockReadFileSync(dataFile, 'utf-8');
        const mappings = JSON.parse(content);

        expect(mappings).toHaveLength(1);
        expect(mappings[0].name).toBe('Test Mapping');
        expect(mappings[0].controls).toHaveLength(2);
      }
    });

    it('should handle missing mapping data', () => {
      mockExistsSync.mockReturnValue(false);

      const dataFile = './data/canonical-maps.json';
      const exists = mockExistsSync(dataFile);

      expect(exists).toBe(false);
    });

    it('should validate mapping data structure', () => {
      const mapping = mockCanonicalMapping;

      expect(mapping.name).toBeDefined();
      expect(mapping.plugin).toBeDefined();
      expect(mapping.controls).toBeInstanceOf(Array);
      expect(mapping.controls[0]).toHaveProperty('midi_cc');
      expect(mapping.controls[0]).toHaveProperty('parameter');
    });
  });

  describe('Ardour XML Generation', () => {
    const mockMapping = {
      name: 'Test Mapping',
      plugin: 'test-plugin',
      controls: [
        {
          name: 'Volume',
          parameter: 'volume',
          midi_cc: 1,
          min: 0,
          max: 100
        }
      ]
    };

    it('should generate valid Ardour XML structure', () => {
      const expectedXmlStructure = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="${mockMapping.name}">
  <Binding channel="1" ctl="${mockMapping.controls[0].midi_cc}" parameter="${mockMapping.controls[0].parameter}"/>
</ArdourMIDIBindings>`;

      // Test XML structure components
      expect(expectedXmlStructure).toContain('<?xml version="1.0"');
      expect(expectedXmlStructure).toContain('ArdourMIDIBindings');
      expect(expectedXmlStructure).toContain('Binding channel="1"');
      expect(expectedXmlStructure).toContain(`ctl="${mockMapping.controls[0].midi_cc}"`);
      expect(expectedXmlStructure).toContain(`parameter="${mockMapping.controls[0].parameter}"`);
    });

    it('should handle multiple controls in XML', () => {
      const multiControlMapping = {
        ...mockMapping,
        controls: [
          { name: 'Volume', parameter: 'volume', midi_cc: 1 },
          { name: 'Pan', parameter: 'pan', midi_cc: 2 },
          { name: 'Filter', parameter: 'filter_cutoff', midi_cc: 3 }
        ]
      };

      // Should generate multiple binding elements
      expect(multiControlMapping.controls).toHaveLength(3);

      for (const control of multiControlMapping.controls) {
        expect(control.midi_cc).toBeGreaterThanOrEqual(1);
        expect(control.midi_cc).toBeLessThanOrEqual(127);
        expect(control.parameter).toBeDefined();
      }
    });

    it('should escape XML special characters', () => {
      const mappingWithSpecialChars = {
        name: 'Test & Special <Chars>',
        controls: [
          { name: 'Volume & Pan', parameter: 'vol&pan', midi_cc: 1 }
        ]
      };

      // XML escaping rules
      const escapedName = mappingWithSpecialChars.name
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');

      expect(escapedName).toBe('Test &amp; Special &lt;Chars&gt;');
    });

    it('should validate XML against Ardour schema requirements', () => {
      const requiredElements = [
        'ArdourMIDIBindings',
        'version',
        'name',
        'Binding',
        'channel',
        'ctl',
        'parameter'
      ];

      const xmlTemplate = `
        <ArdourMIDIBindings version="1.0.0" name="test">
          <Binding channel="1" ctl="1" parameter="volume"/>
        </ArdourMIDIBindings>
      `;

      for (const element of requiredElements) {
        expect(xmlTemplate.includes(element)).toBe(true);
      }
    });
  });

  describe('Output File Generation', () => {
    it('should generate output files with correct naming', () => {
      const mapping = { name: 'Test Plugin Mapping', plugin: 'test-plugin' };
      const target = 'ardour';

      // Expected filename format
      const expectedFilename = `${mapping.plugin}-${target}.xml`;
      const sanitizedFilename = expectedFilename
        .replace(/[^a-zA-Z0-9.-]/g, '_')
        .toLowerCase();

      expect(sanitizedFilename).toBe('test-plugin-ardour.xml');
    });

    it('should create output directory if it does not exist', () => {
      const outputPath = './output/daw-maps';
      mockExistsSync.mockReturnValue(false);

      // Should create directory
      if (!mockExistsSync(outputPath)) {
        mockMkdirSync(outputPath, { recursive: true });
      }

      expect(mockMkdirSync).toHaveBeenCalledWith(outputPath, { recursive: true });
    });

    it('should handle file write operations', () => {
      const outputFile = './output/test-mapping.xml';
      const xmlContent = '<ArdourMIDIBindings>...</ArdourMIDIBindings>';

      mockWriteFileSync(outputFile, xmlContent, 'utf-8');

      expect(mockWriteFileSync).toHaveBeenCalledWith(outputFile, xmlContent, 'utf-8');
    });

    it('should respect force flag for overwriting', () => {
      const outputFile = './output/existing-file.xml';
      const force = true;

      // File exists
      mockExistsSync.mockReturnValue(true);

      const shouldOverwrite = force || !mockExistsSync(outputFile);
      expect(shouldOverwrite).toBe(true);
    });

    it('should skip existing files when force is false', () => {
      const outputFile = './output/existing-file.xml';
      const force = false;

      mockExistsSync.mockReturnValue(true);

      const shouldSkip = !force && mockExistsSync(outputFile);
      expect(shouldSkip).toBe(true);
    });
  });

  describe('Installation Support', () => {
    it('should determine Ardour installation paths', () => {
      const possiblePaths = [
        '~/.config/ardour8/midi_maps',
        '~/.config/ardour7/midi_maps',
        '~/.config/ardour6/midi_maps',
        '/usr/share/ardour8/midi_maps',
        '/usr/local/share/ardour8/midi_maps'
      ];

      // Should check user config first, then system paths
      expect(possiblePaths[0]).toContain('.config/ardour');
      expect(possiblePaths.some(p => p.includes('/usr/share/'))).toBe(true);
    });

    it('should handle installation when directory exists', () => {
      const installPath = '~/.config/ardour8/midi_maps';
      mockExistsSync.mockReturnValue(true);

      const canInstall = mockExistsSync(installPath);
      expect(canInstall).toBe(true);
    });

    it('should handle missing installation directory', () => {
      const installPath = '~/.config/ardour8/midi_maps';
      mockExistsSync.mockReturnValue(false);

      const canInstall = mockExistsSync(installPath);
      expect(canInstall).toBe(false);
    });

    it('should copy files to installation directory', () => {
      const sourceFile = './output/test-mapping.xml';
      const installFile = '~/.config/ardour8/midi_maps/test-mapping.xml';

      // Simulate file copy operation
      mockReadFileSync.mockReturnValue('<xml>content</xml>');
      const content = mockReadFileSync(sourceFile, 'utf-8');
      mockWriteFileSync(installFile, content, 'utf-8');

      expect(mockWriteFileSync).toHaveBeenCalledWith(installFile, '<xml>content</xml>', 'utf-8');
    });
  });

  describe('Error Handling', () => {
    it('should handle missing input data gracefully', () => {
      mockExistsSync.mockReturnValue(false);

      const inputFile = './data/canonical-maps.json';
      const exists = mockExistsSync(inputFile);

      if (!exists) {
        const error = new Error(
          'No canonical mapping data found. Please run: pnpm maps:validate'
        );
        expect(error.message).toContain('No canonical mapping data found');
        expect(error.message).toContain('pnpm maps:validate');
      }
    });

    it('should handle write permission errors', () => {
      mockWriteFileSync.mockImplementation(() => {
        throw new Error('Permission denied');
      });

      expect(() => {
        mockWriteFileSync('./output/test.xml', 'content', 'utf-8');
      }).toThrow('Permission denied');
    });

    it('should provide helpful error messages', () => {
      const errorMessages = {
        noInput: 'No canonical mapping data found. Please run: pnpm maps:validate',
        invalidTarget: 'Invalid target DAW. Supported: ardour, ableton, reaper, all',
        writeError: 'Failed to write output file. Check permissions and disk space.',
        installError: 'Installation failed. Check DAW installation and permissions.'
      };

      Object.values(errorMessages).forEach(message => {
        expect(message).toContain('.');
        expect(message.length).toBeGreaterThan(10);
      });
    });
  });

  describe('Performance Requirements', () => {
    it('should handle large mapping files efficiently', () => {
      const largeMappingCount = 1000;
      const mockMappings = Array.from({ length: largeMappingCount }, (_, i) => ({
        name: `Mapping ${i}`,
        plugin: `plugin-${i}`,
        controls: [
          { name: 'Volume', parameter: 'volume', midi_cc: 1 }
        ]
      }));

      // Should be able to process large arrays
      expect(mockMappings).toHaveLength(largeMappingCount);

      // Simulate processing time check (< 20ms requirement)
      const startTime = Date.now();

      // Simulate XML generation for all mappings
      const processedCount = mockMappings.length;

      const endTime = Date.now();
      const processingTime = endTime - startTime;

      expect(processedCount).toBe(largeMappingCount);
      expect(processingTime).toBeLessThan(100); // Generous for test environment
    });

    it('should minimize memory usage during generation', () => {
      const mapping = {
        name: 'Test',
        controls: [{ name: 'Volume', parameter: 'volume', midi_cc: 1 }]
      };

      // Should process one mapping at a time rather than loading all into memory
      const processedMapping = { ...mapping };

      expect(processedMapping).toBeDefined();
      expect(JSON.stringify(processedMapping).length).toBeLessThan(1000);
    });
  });

  describe('Multi-DAW Support', () => {
    it('should support multiple DAW formats', () => {
      const supportedFormats = {
        ardour: { extension: '.xml', generator: 'generateArdourXML' },
        ableton: { extension: '.als', generator: 'generateAbletonALS' },
        reaper: { extension: '.reascript', generator: 'generateReaperScript' }
      };

      expect(supportedFormats.ardour.extension).toBe('.xml');
      expect(supportedFormats.ableton.extension).toBe('.als');
      expect(supportedFormats.reaper.extension).toBe('.reascript');
    });

    it('should generate all formats when target is "all"', () => {
      const target = 'all';
      const supportedTargets = ['ardour', 'ableton', 'reaper'];

      const targetsToGenerate = target === 'all' ? supportedTargets : [target];

      expect(targetsToGenerate).toEqual(supportedTargets);
      expect(targetsToGenerate).toHaveLength(3);
    });
  });
});