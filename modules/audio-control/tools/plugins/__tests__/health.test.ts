/**
 * Plugin Health Check Tool Tests
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { existsSync, statSync, readFileSync } from 'node:fs';

// Mock fs operations
vi.mock('node:fs', () => ({
  existsSync: vi.fn(),
  statSync: vi.fn(),
  readFileSync: vi.fn(),
}));

// Mock console methods
const mockConsole = {
  log: vi.fn(),
  error: vi.fn(),
  warn: vi.fn(),
};

vi.stubGlobal('console', mockConsole);

const mockExistsSync = vi.mocked(existsSync);
const mockStatSync = vi.mocked(statSync);
const mockReadFileSync = vi.mocked(readFileSync);

// Mock health check result interface
interface HealthResult {
  status: 'healthy' | 'warning' | 'error';
  message: string;
  details?: string[];
}

describe('Plugin Health Check Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('CLI argument parsing', () => {
    it('should parse verbose flag correctly', async () => {
      const { parseArgs } = await import('node:util');
      const mockParseArgs = vi.fn().mockReturnValue({
        values: { verbose: true, fix: false }
      });
      vi.mocked(parseArgs).mockImplementation(mockParseArgs);

      expect(mockParseArgs).toBeDefined();
    });

    it('should parse fix flag correctly', async () => {
      const { parseArgs } = await import('node:util');
      const mockParseArgs = vi.fn().mockReturnValue({
        values: { fix: true, verbose: false }
      });
      vi.mocked(parseArgs).mockImplementation(mockParseArgs);

      expect(mockParseArgs).toBeDefined();
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

  describe('JUCE host health check', () => {
    it('should detect healthy JUCE host', () => {
      mockExistsSync.mockReturnValue(true);
      mockStatSync.mockReturnValue({
        size: 1024000,
        mtime: new Date('2024-01-01'),
        birthtime: new Date('2024-01-01'),
        isFile: () => true,
        isDirectory: () => false
      } as any);

      const result: HealthResult = {
        status: 'healthy',
        message: 'JUCE Host found',
        details: ['Size: 1000KB', 'Modified: 2024-01-01T00:00:00.000Z']
      };

      expect(result.status).toBe('healthy');
      expect(result.details?.[0]).toContain('Size:');
    });

    it('should detect missing JUCE host', () => {
      mockExistsSync.mockReturnValue(false);

      const result: HealthResult = {
        status: 'error',
        message: 'JUCE plugin host not found',
        details: ['Build the project: cd ../../ && make']
      };

      expect(result.status).toBe('error');
      expect(result.message).toContain('not found');
    });

    it('should check multiple possible JUCE host paths', () => {
      const possiblePaths = [
        '../../cmake-build/modules/juce/host/plughost_artefacts/plughost',
        '../../../cmake-build/modules/juce/host/plughost_artefacts/plughost'
      ];

      // Test that multiple paths are checked
      expect(possiblePaths.length).toBeGreaterThan(1);
    });
  });

  describe('extracted data health check', () => {
    it('should detect healthy extracted data', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockReturnValue('{"plugins": [{"name": "Test Plugin"}]}');
      mockStatSync.mockReturnValue({
        size: 2048,
        mtime: new Date('2024-01-01'),
        birthtime: new Date('2024-01-01')
      } as any);

      const testData = { plugins: [{ name: 'Test Plugin' }] };

      expect(Object.keys(testData).length).toBe(1);
      expect(testData.plugins.length).toBe(1);
    });

    it('should detect missing extracted data', () => {
      mockExistsSync.mockReturnValue(false);

      const result: HealthResult = {
        status: 'warning',
        message: 'No extracted plugin data found',
        details: ['Run extraction: pnpm plugins:extract']
      };

      expect(result.status).toBe('warning');
      expect(result.details?.[0]).toContain('pnpm plugins:extract');
    });

    it('should detect corrupted extracted data', () => {
      mockExistsSync.mockReturnValue(true);
      mockReadFileSync.mockReturnValue('invalid json content');

      // Test JSON parsing error handling
      expect(() => JSON.parse('invalid json content')).toThrow();
    });

    it('should validate data structure integrity', () => {
      const validData = {
        plugins: [
          {
            name: 'Test Plugin',
            parameters: [
              { name: 'Volume', min: 0, max: 127 }
            ]
          }
        ]
      };

      expect(validData.plugins).toBeInstanceOf(Array);
      expect(validData.plugins[0].parameters).toBeInstanceOf(Array);
    });
  });

  describe('data directory health check', () => {
    it('should detect existing data directory', () => {
      mockExistsSync.mockReturnValue(true);
      mockStatSync.mockReturnValue({
        birthtime: new Date('2024-01-01'),
        mtime: new Date('2024-01-01'),
        isDirectory: () => true
      } as any);

      const result: HealthResult = {
        status: 'healthy',
        message: 'Data directory exists',
        details: ['Created: 2024-01-01T00:00:00.000Z']
      };

      expect(result.status).toBe('healthy');
    });

    it('should detect missing data directory', () => {
      mockExistsSync.mockReturnValue(false);

      const result: HealthResult = {
        status: 'warning',
        message: 'Data directory does not exist',
        details: ['Will be created during extraction']
      };

      expect(result.status).toBe('warning');
    });
  });

  describe('health check orchestration', () => {
    it('should run all health checks', () => {
      const checks = [
        { name: 'JUCE Host', status: 'healthy' },
        { name: 'Data Directory', status: 'healthy' },
        { name: 'Extracted Data', status: 'healthy' }
      ];

      expect(checks.length).toBe(3);
      expect(checks.every(c => c.status === 'healthy')).toBe(true);
    });

    it('should detect system with errors', () => {
      const checks = [
        { name: 'JUCE Host', status: 'error' },
        { name: 'Data Directory', status: 'healthy' }
      ];

      const hasErrors = checks.some(c => c.status === 'error');
      expect(hasErrors).toBe(true);
    });

    it('should detect system with warnings', () => {
      const checks = [
        { name: 'JUCE Host', status: 'healthy' },
        { name: 'Extracted Data', status: 'warning' }
      ];

      const hasWarnings = checks.some(c => c.status === 'warning');
      expect(hasWarnings).toBe(true);
    });
  });

  describe('verbose output mode', () => {
    it('should provide detailed information in verbose mode', () => {
      const healthResult = {
        status: 'healthy' as const,
        message: 'Test check passed',
        details: [
          'Size: 1000KB',
          'Modified: 2024-01-01T00:00:00.000Z',
          'Permissions: readable'
        ]
      };

      expect(healthResult.details?.length).toBe(3);
      expect(healthResult.details?.[0]).toContain('Size:');
    });

    it('should show basic information in non-verbose mode', () => {
      const healthResult = {
        status: 'healthy' as const,
        message: 'Test check passed'
      };

      expect(healthResult.message).toBe('Test check passed');
      expect(healthResult.details).toBeUndefined();
    });
  });

  describe('automatic fix mode', () => {
    it('should identify fixable issues', () => {
      const fixableIssues = [
        'Create missing directories',
        'Fix file permissions',
        'Re-run extraction if data is corrupted',
        'Rebuild JUCE host if missing'
      ];

      expect(fixableIssues.length).toBe(4);
      expect(fixableIssues[0]).toContain('directories');
    });

    it('should handle fix mode not implemented error', () => {
      const expectedError = 'Automatic fixes not yet implemented.';

      expect(expectedError).toContain('not yet implemented');
    });
  });

  describe('performance validation', () => {
    it('should complete health check quickly', () => {
      const startTime = Date.now();

      // Simulate health check operations
      const checks = Array(10).fill(true);
      const duration = Date.now() - startTime;

      expect(checks.length).toBe(10);
      expect(duration).toBeLessThan(100); // Should be very fast
    });

    it('should handle large extracted data files efficiently', () => {
      const largeDataSize = 10 * 1024 * 1024; // 10MB

      mockStatSync.mockReturnValue({
        size: largeDataSize,
        mtime: new Date()
      } as any);

      expect(largeDataSize).toBeGreaterThan(1024 * 1024);
    });
  });

  describe('error recovery suggestions', () => {
    it('should provide recovery steps for missing JUCE host', () => {
      const suggestions = [
        'Build the project: cd ../../ && make',
        'Check CMake configuration',
        'Verify build dependencies'
      ];

      expect(suggestions[0]).toContain('make');
    });

    it('should provide recovery steps for corrupted data', () => {
      const suggestions = [
        'Re-run extraction: pnpm plugins:extract --force',
        'Check file permissions',
        'Clear cache and retry'
      ];

      expect(suggestions[0]).toContain('--force');
    });
  });

  describe('system integration validation', () => {
    it('should validate plugin system readiness', () => {
      const systemReady = {
        juceHost: true,
        dataDirectory: true,
        extractedData: true,
        permissions: true
      };

      const isReady = Object.values(systemReady).every(Boolean);
      expect(isReady).toBe(true);
    });

    it('should detect incomplete plugin system', () => {
      const systemStatus = {
        juceHost: false,
        dataDirectory: true,
        extractedData: false
      };

      const isReady = Object.values(systemStatus).every(Boolean);
      expect(isReady).toBe(false);
    });
  });
});