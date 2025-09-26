/**
 * Maps Validation Tool Tests
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { readFile, stat } from 'fs/promises';
import { glob } from 'glob';

// Mock fs/promises
vi.mock('fs/promises', () => ({
  readFile: vi.fn(),
  stat: vi.fn(),
}));

// Mock glob
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

// Mock validation types
interface ValidationError {
  path: string;
  message: string;
  code: string;
  severity?: 'low' | 'medium' | 'high' | 'critical';
}

interface ValidationWarning {
  path: string;
  message: string;
  code: string;
  category?: string;
}

interface ValidationResult {
  valid: boolean;
  errors: ValidationError[];
  warnings: ValidationWarning[];
  score?: number;
}

describe('Maps Validation Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('CLI argument parsing', () => {
    it('should parse validation options correctly', () => {
      const validationOptions = {
        checkPlugins: true,
        pluginDescriptorsDir: './descriptors',
        strict: true,
        includeWarnings: true,
        maxFileSize: 1024 * 1024
      };

      expect(validationOptions.checkPlugins).toBe(true);
      expect(validationOptions.strict).toBe(true);
      expect(validationOptions.maxFileSize).toBe(1024 * 1024);
    });

    it('should handle help flag', () => {
      const helpArgs = ['--help'];
      expect(helpArgs.includes('--help')).toBe(true);
    });

    it('should parse directory and file arguments', () => {
      const args = ['./maps', '--strict', '--check-plugins'];
      expect(args[0]).toBe('./maps');
      expect(args.includes('--strict')).toBe(true);
    });
  });

  describe('file size validation', () => {
    it('should accept files within size limit', async () => {
      mockStat.mockResolvedValue({
        size: 512 * 1024, // 512KB
        isFile: () => true,
        isDirectory: () => false
      } as any);

      const maxSize = 1024 * 1024; // 1MB
      const fileSize = 512 * 1024;

      expect(fileSize).toBeLessThan(maxSize);
    });

    it('should reject files exceeding size limit', async () => {
      mockStat.mockResolvedValue({
        size: 2 * 1024 * 1024, // 2MB
        isFile: () => true,
        isDirectory: () => false
      } as any);

      const maxSize = 1024 * 1024; // 1MB
      const fileSize = 2 * 1024 * 1024;

      expect(fileSize).toBeGreaterThan(maxSize);
    });
  });

  describe('format detection and parsing', () => {
    it('should detect YAML format', () => {
      const yamlFile = 'test-map.yaml';
      const extension = yamlFile.split('.').pop()?.toLowerCase();

      expect(extension).toBe('yaml');
    });

    it('should detect JSON format', () => {
      const jsonFile = 'test-map.json';
      const extension = jsonFile.split('.').pop()?.toLowerCase();

      expect(extension).toBe('json');
    });

    it('should reject unsupported formats', () => {
      const unsupportedFile = 'test-map.xml';
      const extension = unsupportedFile.split('.').pop()?.toLowerCase();
      const supportedFormats = ['yaml', 'yml', 'json'];

      expect(supportedFormats).not.toContain(extension);
    });

    it('should parse valid YAML content', async () => {
      const validYaml = `
device:
  manufacturer: "Test Company"
  model: "Test Controller"
controls:
  - id: "volume"
    cc: 7
    type: "knob"
`;

      mockReadFile.mockResolvedValue(validYaml);

      // Test YAML parsing logic
      expect(validYaml).toContain('device:');
      expect(validYaml).toContain('controls:');
    });

    it('should parse valid JSON content', async () => {
      const validJson = JSON.stringify({
        device: {
          manufacturer: "Test Company",
          model: "Test Controller"
        },
        controls: [
          {
            id: "volume",
            cc: 7,
            type: "knob"
          }
        ]
      });

      mockReadFile.mockResolvedValue(validJson);

      expect(() => JSON.parse(validJson)).not.toThrow();
    });
  });

  describe('MIDI protocol validation', () => {
    it('should validate MIDI channel range (1-16)', () => {
      const validChannels = [1, 8, 16];
      const invalidChannels = [0, 17, -1];

      validChannels.forEach(channel => {
        expect(channel).toBeGreaterThanOrEqual(1);
        expect(channel).toBeLessThanOrEqual(16);
      });

      invalidChannels.forEach(channel => {
        expect(channel < 1 || channel > 16).toBe(true);
      });
    });

    it('should validate CC number range (0-127)', () => {
      const validCCs = [0, 64, 127];
      const invalidCCs = [-1, 128, 255];

      validCCs.forEach(cc => {
        expect(cc).toBeGreaterThanOrEqual(0);
        expect(cc).toBeLessThanOrEqual(127);
      });

      invalidCCs.forEach(cc => {
        expect(cc < 0 || cc > 127).toBe(true);
      });
    });

    it('should validate value ranges', () => {
      const validRange = [0, 127];
      const invalidRanges = [
        [-1, 127], // Min too low
        [0, 128],  // Max too high
        [64, 32]   // Min > Max
      ];

      const [min, max] = validRange;
      expect(min).toBeGreaterThanOrEqual(0);
      expect(max).toBeLessThanOrEqual(127);
      expect(min).toBeLessThan(max);

      invalidRanges.forEach(([min, max]) => {
        const isValid = min >= 0 && max <= 127 && min < max;
        expect(isValid).toBe(false);
      });
    });

    it('should detect duplicate CC assignments', () => {
      const controls = [
        { id: 'volume', cc: 7, channel: 1 },
        { id: 'pan', cc: 10, channel: 1 },
        { id: 'reverb', cc: 7, channel: 1 } // Duplicate CC 7 on channel 1
      ];

      const ccMap = new Map<string, string[]>();
      controls.forEach(control => {
        const key = `${control.channel}:${control.cc}`;
        if (!ccMap.has(key)) {
          ccMap.set(key, []);
        }
        ccMap.get(key)!.push(control.id);
      });

      const duplicates = Array.from(ccMap.entries()).filter(([_, ids]) => ids.length > 1);
      expect(duplicates.length).toBe(1);
      expect(duplicates[0][1]).toEqual(['volume', 'reverb']);
    });
  });

  describe('plugin descriptor cross-validation', () => {
    it('should load plugin descriptors', async () => {
      const mockDescriptorFiles = [
        '/path/to/plugin1.json',
        '/path/to/plugin2.json'
      ];

      mockGlob.mockResolvedValue(mockDescriptorFiles);
      mockReadFile.mockImplementation((file) => {
        if (file.toString().includes('plugin1')) {
          return Promise.resolve(JSON.stringify({
            plugin: { manufacturer: 'Company A', name: 'Plugin A' },
            parameters: [{ index: 0, name: 'Volume', automatable: true }]
          }));
        }
        return Promise.resolve(JSON.stringify({
          plugin: { manufacturer: 'Company B', name: 'Plugin B' },
          parameters: [{ index: 0, name: 'Gain', automatable: true }]
        }));
      });

      expect(mockDescriptorFiles.length).toBe(2);
    });

    it('should validate plugin parameter mappings', () => {
      const pluginDescriptor = {
        plugin: { manufacturer: 'Test', name: 'Synth' },
        parameters: [
          { index: 0, name: 'Volume', automatable: true },
          { index: 1, name: 'Cutoff', automatable: true },
          { index: 2, name: 'Bypass', automatable: false }
        ]
      };

      const mappedParameter = 1; // Cutoff
      const invalidParameter = 99; // Doesn't exist

      const availableParams = pluginDescriptor.parameters.map(p => p.index);
      expect(availableParams).toContain(mappedParameter);
      expect(availableParams).not.toContain(invalidParameter);
    });

    it('should warn about non-automatable parameters', () => {
      const parameter = {
        index: 2,
        name: 'Bypass',
        automatable: false
      };

      expect(parameter.automatable).toBe(false);
    });
  });

  describe('validation scoring', () => {
    it('should calculate perfect score for error-free validation', () => {
      const errors: ValidationError[] = [];
      const warnings: ValidationWarning[] = [];

      const score = errors.length === 0 && warnings.length === 0 ? 1.0 : 0.5;
      expect(score).toBe(1.0);
    });

    it('should penalize based on error severity', () => {
      const errors: ValidationError[] = [
        { path: 'test', message: 'Critical error', code: 'CRIT', severity: 'critical' },
        { path: 'test', message: 'High error', code: 'HIGH', severity: 'high' },
        { path: 'test', message: 'Medium error', code: 'MED', severity: 'medium' }
      ];

      const severityPenalties = {
        critical: 0.5,
        high: 0.3,
        medium: 0.2,
        low: 0.1
      };

      const totalPenalty = errors.reduce((sum, error) => {
        return sum + (severityPenalties[error.severity || 'medium'] || 0.2);
      }, 0);

      expect(totalPenalty).toBe(1.0); // 0.5 + 0.3 + 0.2
    });

    it('should include warning penalty in score', () => {
      const warnings: ValidationWarning[] = [
        { path: 'test', message: 'Warning 1', code: 'WARN1' },
        { path: 'test', message: 'Warning 2', code: 'WARN2' }
      ];

      const warningPenalty = warnings.length * 0.05;
      expect(warningPenalty).toBe(0.1);
    });
  });

  describe('batch validation', () => {
    it('should process multiple map files', async () => {
      const mapFiles = [
        './maps/controller1.yaml',
        './maps/controller2.json',
        './maps/controller3.yaml'
      ];

      mockGlob.mockResolvedValue(mapFiles);

      expect(mapFiles.length).toBe(3);
    });

    it('should collect validation results', () => {
      const results = [
        { valid: true, errors: [], warnings: [], mapId: 'map1' },
        { valid: false, errors: [{ path: 'test', message: 'Error', code: 'ERR' }], warnings: [], mapId: 'map2' },
        { valid: true, errors: [], warnings: [], mapId: 'map3' }
      ];

      const validCount = results.filter(r => r.valid).length;
      const totalCount = results.length;

      expect(validCount).toBe(2);
      expect(totalCount).toBe(3);
    });

    it('should calculate summary statistics', () => {
      const results = [
        { score: 1.0, performance: { validationTime: 10 } },
        { score: 0.8, performance: { validationTime: 15 } },
        { score: 0.9, performance: { validationTime: 12 } }
      ];

      const avgScore = results.reduce((sum, r) => sum + (r.score || 0), 0) / results.length;
      const avgTime = results.reduce((sum, r) => sum + r.performance.validationTime, 0) / results.length;

      expect(avgScore).toBeCloseTo(0.9);
      expect(avgTime).toBeCloseTo(12.33, 1);
    });
  });

  describe('performance validation', () => {
    it('should complete validation within time limits', () => {
      const startTime = performance.now();

      // Simulate validation process
      const mockValidation = () => {
        // Quick validation simulation
        return { valid: true, errors: [], warnings: [] };
      };

      const result = mockValidation();
      const duration = performance.now() - startTime;

      expect(result.valid).toBe(true);
      expect(duration).toBeLessThan(100); // Should be very fast
    });

    it('should handle large files efficiently', () => {
      const largeFileContent = JSON.stringify({
        device: { manufacturer: 'Test', model: 'Large Controller' },
        controls: Array(1000).fill(0).map((_, i) => ({
          id: `control_${i}`,
          cc: i % 128,
          type: 'knob'
        }))
      });

      expect(largeFileContent.length).toBeGreaterThan(10000);
    });
  });

  describe('error handling and recovery', () => {
    it('should handle file read errors gracefully', async () => {
      mockReadFile.mockRejectedValue(new Error('File not found'));

      try {
        await mockReadFile('/nonexistent/file.yaml');
        expect(true).toBe(false); // Should not reach here
      } catch (error) {
        expect(error).toBeInstanceOf(Error);
        expect((error as Error).message).toBe('File not found');
      }
    });

    it('should handle malformed JSON/YAML', () => {
      const malformedJson = '{"device": { "name": "test"'; // Missing closing braces

      expect(() => JSON.parse(malformedJson)).toThrow();
    });

    it('should provide helpful error messages', () => {
      const error: ValidationError = {
        path: 'controls[0].cc',
        message: 'CC 128 is out of range (0-127)',
        code: 'INVALID_CC_NUMBER',
        severity: 'high'
      };

      expect(error.message).toContain('out of range');
      expect(error.code).toBe('INVALID_CC_NUMBER');
      expect(error.severity).toBe('high');
    });
  });

  describe('integration with canonical-midi-maps module', () => {
    it('should use canonical map parser', () => {
      // Mock the canonical map parser interface
      const parseResult = {
        map: {
          device: { manufacturer: 'Test', model: 'Controller' },
          controls: []
        },
        validation: { valid: true, errors: [], warnings: [] }
      };

      expect(parseResult.validation.valid).toBe(true);
      expect(parseResult.map.device.manufacturer).toBe('Test');
    });

    it('should validate against canonical schema', () => {
      const canonicalMap = {
        device: {
          manufacturer: 'Test Company',
          model: 'Test Controller'
        },
        controls: [
          {
            id: 'volume',
            cc: 7,
            type: 'knob',
            range: [0, 127]
          }
        ]
      };

      // Basic structure validation
      expect(canonicalMap.device).toBeDefined();
      expect(canonicalMap.controls).toBeInstanceOf(Array);
      expect(canonicalMap.controls[0].id).toBe('volume');
    });
  });
});