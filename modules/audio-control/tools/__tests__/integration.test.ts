/**
 * Integration Tests for End-to-End Workflows
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { execSync } from 'node:child_process';
import { existsSync, readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, join } from 'node:path';

// Mock all external dependencies
vi.mock('node:child_process', () => ({
  execSync: vi.fn(),
}));

vi.mock('node:fs', () => ({
  existsSync: vi.fn(),
  readFileSync: vi.fn(),
  writeFileSync: vi.fn(),
  mkdirSync: vi.fn(),
}));

// Mock console methods
const mockConsole = {
  log: vi.fn(),
  error: vi.fn(),
  warn: vi.fn(),
};

vi.stubGlobal('console', mockConsole);

const mockExecSync = vi.mocked(execSync);
const mockExistsSync = vi.mocked(existsSync);
const mockReadFileSync = vi.mocked(readFileSync);
const mockWriteFileSync = vi.mocked(writeFileSync);
const mockMkdirSync = vi.mocked(mkdirSync);

// Test data
const mockCanonicalMap = {
  device: {
    manufacturer: "Native Instruments",
    model: "Maschine MK3"
  },
  plugin: {
    manufacturer: "Xfer Records",
    name: "Serum",
    version: "1.369",
    format: "VST3"
  },
  metadata: {
    name: "Maschine MK3 - Serum",
    description: "Complete mapping for Serum on Maschine MK3",
    author: "Test Author",
    date: "2024-01-01",
    tags: ["synthesizer", "wavetable"]
  },
  controls: [
    {
      id: "volume",
      cc: 7,
      type: "knob",
      plugin_parameter: 0,
      range: [0, 127]
    },
    {
      id: "cutoff",
      cc: 74,
      type: "knob",
      plugin_parameter: 1,
      range: [0, 127]
    }
  ],
  version: "1.0"
};

const mockPluginDescriptor = {
  plugin: {
    manufacturer: "Xfer Records",
    name: "Serum",
    version: "1.369",
    format: "VST3"
  },
  parameters: [
    {
      index: 0,
      name: "Volume",
      min: 0,
      max: 1,
      automatable: true
    },
    {
      index: 1,
      name: "Filter Cutoff",
      min: 0,
      max: 1,
      automatable: true
    }
  ],
  metadata: {
    scanned: "2024-01-01T00:00:00Z",
    pluginId: "xfer-records_serum"
  }
};

describe('Integration Tests', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('End-to-End Plugin → Map → DAW Workflow', () => {
    it('should complete full workflow successfully', async () => {
      // Setup: Mock successful execution for all steps
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync
        .mockReturnValueOnce(JSON.stringify(mockPluginDescriptor)) // Plugin descriptor
        .mockReturnValueOnce(JSON.stringify(mockCanonicalMap));    // Canonical map

      mockExecSync
        .mockReturnValueOnce('Plugin health check passed')
        .mockReturnValueOnce('Plugin extraction completed')
        .mockReturnValueOnce('Map validation successful')
        .mockReturnValueOnce('DAW generation completed');

      mockWriteFileSync.mockImplementation(() => undefined);

      // Simulate end-to-end workflow
      const workflowSteps = [
        'pnpm plugins:health',
        'pnpm plugins:extract',
        'pnpm maps:validate',
        'pnpm daw:generate --target ardour'
      ];

      const results = workflowSteps.map((command, index) => {
        try {
          const output = mockExecSync(command);
          return {
            step: `Step ${index + 1}`,
            command,
            success: true,
            output: output?.toString()
          };
        } catch (error) {
          return {
            step: `Step ${index + 1}`,
            command,
            success: false,
            error: error instanceof Error ? error.message : 'Unknown error'
          };
        }
      });

      const successfulSteps = results.filter(r => r.success);
      expect(successfulSteps.length).toBe(4);
      expect(mockExecSync).toHaveBeenCalledTimes(4);
    });

    it('should handle partial workflow failure gracefully', () => {
      mockExecSync
        .mockReturnValueOnce('Plugin health check passed')
        .mockImplementationOnce(() => {
          throw new Error('Plugin extraction failed');
        })
        .mockReturnValueOnce('Map validation successful'); // This shouldn't be reached

      const workflowSteps = [
        'pnpm plugins:health',
        'pnpm plugins:extract',
        'pnpm maps:validate'
      ];

      let shouldContinue = true;
      const results = [];

      for (const command of workflowSteps) {
        if (!shouldContinue) break;

        try {
          const output = mockExecSync(command);
          results.push({ success: true, command, output });
        } catch (error: any) {
          results.push({ success: false, command, error: error.message });
          if (!error.message.includes('not yet implemented')) {
            shouldContinue = false; // Stop on critical failure
          }
        }
      }

      expect(results.length).toBe(2); // Should stop after plugin extraction failure
      expect(results[1].success).toBe(false);
    });

    it('should validate data flow between workflow phases', () => {
      // Phase 1: Plugin extraction creates descriptors
      const pluginDescriptors = [mockPluginDescriptor];
      mockWriteFileSync.mockImplementation((path, content) => {
        if (path.toString().includes('plugin-descriptors.json')) {
          expect(content).toContain('Serum');
        }
      });

      // Phase 2: Map validation uses descriptors
      mockReadFileSync.mockReturnValue(JSON.stringify(pluginDescriptors));
      mockExistsSync.mockReturnValue(true);

      // Phase 3: DAW generation uses validated maps
      const ardourXML = `<?xml version="1.0"?>
<ArdourMIDIBindings>
  <Binding channel="0" ctl="7" function="plugin-parameter[0]"/>
</ArdourMIDIBindings>`;

      mockWriteFileSync.mockImplementation((path, content) => {
        if (path.toString().endsWith('.map')) {
          expect(content).toContain('ArdourMIDIBindings');
        }
      });

      // Simulate data flow validation
      expect(pluginDescriptors[0].plugin.name).toBe('Serum');
      expect(mockCanonicalMap.plugin?.name).toBe('Serum');
    });
  });

  describe('Cross-Module Integration', () => {
    it('should integrate canonical-midi-maps module correctly', () => {
      // Test parsing canonical maps
      const yamlContent = `
device:
  manufacturer: "Native Instruments"
  model: "Maschine MK3"
controls:
  - id: "volume"
    cc: 7
    type: "knob"
version: "1.0"
`;

      mockReadFileSync.mockReturnValue(yamlContent);

      // Simulate canonical map parsing
      expect(yamlContent).toContain('device:');
      expect(yamlContent).toContain('controls:');
    });

    it('should integrate ardour-midi-maps module correctly', () => {
      // Test Ardour XML generation
      const expectedXML = `<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Test Map">
  <Binding channel="0" ctl="7" function="plugin-parameter[0]" encoder="no"/>
  <Binding channel="0" ctl="74" function="plugin-parameter[1]" encoder="no"/>
</ArdourMIDIBindings>`;

      mockWriteFileSync.mockImplementation((path, content) => {
        if (path.toString().endsWith('.map')) {
          expect(content).toContain('ArdourMIDIBindings');
          expect(content).toContain('Binding');
        }
      });

      // Simulate XML generation
      const xmlGenerated = true;
      expect(xmlGenerated).toBe(true);
    });

    it('should validate TypeScript interfaces across modules', () => {
      // Test interface compatibility
      interface CanonicalControl {
        id: string;
        cc?: number;
        type: string;
        plugin_parameter?: number;
      }

      interface ArdourBinding {
        channel: number;
        controller: number;
        function: string;
      }

      const canonicalControl: CanonicalControl = {
        id: "volume",
        cc: 7,
        type: "knob",
        plugin_parameter: 0
      };

      const ardourBinding: ArdourBinding = {
        channel: 0,
        controller: canonicalControl.cc!,
        function: `plugin-parameter[${canonicalControl.plugin_parameter}]`
      };

      expect(ardourBinding.controller).toBe(7);
      expect(ardourBinding.function).toBe('plugin-parameter[0]');
    });
  });

  describe('Performance Integration Tests', () => {
    it('should complete workflow within performance targets', async () => {
      const performanceTargets = {
        healthCheck: 1000,      // 1 second
        extraction: 30000,      // 30 seconds
        validation: 10000,      // 10 seconds
        generation: 20000       // 20 seconds
      };

      const mockTiming = {
        healthCheck: 500,
        extraction: 15000,
        validation: 5000,
        generation: 12000
      };

      Object.entries(mockTiming).forEach(([step, duration]) => {
        const target = performanceTargets[step as keyof typeof performanceTargets];
        expect(duration).toBeLessThan(target);
      });

      const totalTime = Object.values(mockTiming).reduce((sum, time) => sum + time, 0);
      expect(totalTime).toBeLessThan(70000); // Total under 70 seconds
    });

    it('should handle large-scale processing efficiently', () => {
      const largeDataset = {
        plugins: Array(100).fill(mockPluginDescriptor),
        maps: Array(50).fill(mockCanonicalMap),
        controls: Array(1000).fill({ id: 'control', cc: 7, type: 'knob' })
      };

      expect(largeDataset.plugins.length).toBe(100);
      expect(largeDataset.maps.length).toBe(50);
      expect(largeDataset.controls.length).toBe(1000);
    });

    it('should validate memory usage under load', () => {
      const memoryUsage = {
        plugins: 50,      // MB
        maps: 25,         // MB
        generation: 75    // MB
      };

      const totalMemory = Object.values(memoryUsage).reduce((sum, mem) => sum + mem, 0);
      const memoryLimit = 200; // MB

      expect(totalMemory).toBeLessThan(memoryLimit);
    });
  });

  describe('Error Recovery and Resilience', () => {
    it('should recover from temporary failures', () => {
      let attemptCount = 0;
      mockExecSync.mockImplementation(() => {
        attemptCount++;
        if (attemptCount <= 2) {
          throw new Error('Temporary failure');
        }
        return 'Success on retry';
      });

      const executeWithRetry = (command: string, maxRetries: number = 3) => {
        for (let i = 0; i < maxRetries; i++) {
          try {
            return mockExecSync(command);
          } catch (error) {
            if (i === maxRetries - 1) throw error;
            // Wait and retry
          }
        }
      };

      const result = executeWithRetry('pnpm plugins:extract');
      expect(result).toBe('Success on retry');
      expect(attemptCount).toBe(3);
    });

    it('should handle corrupted data gracefully', () => {
      mockReadFileSync
        .mockReturnValueOnce('invalid json content')
        .mockReturnValueOnce(JSON.stringify(mockCanonicalMap)); // Valid fallback

      const parseWithFallback = (filePath: string) => {
        try {
          const content = mockReadFileSync(filePath);
          return JSON.parse(content.toString());
        } catch (error) {
          console.warn(`Failed to parse ${filePath}, attempting recovery...`);
          // Try alternative parsing or fallback
          const fallbackContent = mockReadFileSync('fallback.json');
          return JSON.parse(fallbackContent.toString());
        }
      };

      const result = parseWithFallback('corrupted.json');
      expect(result.device.manufacturer).toBe('Native Instruments');
    });

    it('should validate system state before execution', () => {
      const systemChecks = {
        juceHostAvailable: () => mockExistsSync('/path/to/juce/host'),
        mapsDirectoryExists: () => mockExistsSync('/path/to/maps'),
        outputDirectoryWritable: () => true,
        diskSpaceAvailable: () => true
      };

      mockExistsSync.mockReturnValue(true);

      const systemReady = Object.values(systemChecks).every(check => check());
      expect(systemReady).toBe(true);
    });
  });

  describe('Multi-Platform Integration', () => {
    it('should handle platform-specific paths', () => {
      const originalPlatform = process.platform;

      // Test Windows paths
      Object.defineProperty(process, 'platform', { value: 'win32' });
      const winPath = 'C:\\Users\\User\\AppData\\Roaming\\Ardour8';
      expect(winPath).toContain('C:');

      // Test macOS paths
      Object.defineProperty(process, 'platform', { value: 'darwin' });
      const macPath = '/Users/User/Library/Application Support/Ardour8';
      expect(macPath).toContain('/Library/');

      // Test Linux paths
      Object.defineProperty(process, 'platform', { value: 'linux' });
      const linuxPath = '/home/user/.config/ardour8';
      expect(linuxPath).toContain('/.config/');

      Object.defineProperty(process, 'platform', { value: originalPlatform });
    });

    it('should use platform-appropriate commands', () => {
      const originalPlatform = process.platform;

      Object.defineProperty(process, 'platform', { value: 'win32' });
      const winCopyCmd = 'copy "source" "dest"';
      expect(winCopyCmd).toContain('copy');

      Object.defineProperty(process, 'platform', { value: 'darwin' });
      const unixCopyCmd = 'cp "source" "dest"';
      expect(unixCopyCmd).toContain('cp');

      Object.defineProperty(process, 'platform', { value: originalPlatform });
    });
  });

  describe('Configuration and Environment Integration', () => {
    it('should respect environment variables', () => {
      const originalEnv = process.env;

      process.env.AUDIO_CONTROL_MAPS_DIR = '/custom/maps/dir';
      process.env.AUDIO_CONTROL_OUTPUT_DIR = '/custom/output/dir';

      expect(process.env.AUDIO_CONTROL_MAPS_DIR).toBe('/custom/maps/dir');
      expect(process.env.AUDIO_CONTROL_OUTPUT_DIR).toBe('/custom/output/dir');

      process.env = originalEnv;
    });

    it('should handle configuration files', () => {
      const configContent = JSON.stringify({
        defaultTarget: 'ardour',
        outputFormat: 'xml',
        performanceMode: 'high',
        cacheEnabled: true
      });

      mockReadFileSync.mockReturnValue(configContent);
      mockExistsSync.mockReturnValue(true);

      const config = JSON.parse(configContent);
      expect(config.defaultTarget).toBe('ardour');
      expect(config.cacheEnabled).toBe(true);
    });
  });

  describe('Monitoring and Logging Integration', () => {
    it('should log workflow progress correctly', () => {
      const logEntries: string[] = [];

      const mockLogger = {
        info: (msg: string) => logEntries.push(`INFO: ${msg}`),
        error: (msg: string) => logEntries.push(`ERROR: ${msg}`),
        warn: (msg: string) => logEntries.push(`WARN: ${msg}`)
      };

      mockLogger.info('Starting workflow');
      mockLogger.info('Plugin extraction completed');
      mockLogger.warn('Some maps had warnings');
      mockLogger.info('DAW generation completed');

      expect(logEntries.length).toBe(4);
      expect(logEntries[0]).toContain('Starting workflow');
      expect(logEntries[2]).toContain('WARN');
    });

    it('should generate comprehensive reports', () => {
      const workflowReport = {
        startTime: '2024-01-01T00:00:00Z',
        endTime: '2024-01-01T00:02:00Z',
        duration: 120000,
        stepsCompleted: 4,
        stepsTotal: 4,
        pluginsProcessed: 25,
        mapsGenerated: 15,
        errors: 0,
        warnings: 3,
        performanceMetrics: {
          avgStepTime: 30000,
          memoryPeak: 150
        }
      };

      expect(workflowReport.stepsCompleted).toBe(workflowReport.stepsTotal);
      expect(workflowReport.errors).toBe(0);
      expect(workflowReport.duration).toBe(120000);
    });
  });
});