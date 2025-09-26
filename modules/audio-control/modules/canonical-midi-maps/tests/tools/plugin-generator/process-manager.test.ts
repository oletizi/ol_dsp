/**
 * Tests for process management functionality
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { ProcessManager, PlughostProcessManager, createProcessManager, createPlughostProcessManager } from '@/tools/plugin-generator/process-manager.js';
import type { IProcessManager } from '@/tools/plugin-generator/process-manager.js';
import { spawn } from 'child_process';
import { EventEmitter } from 'events';

// Mock child_process
vi.mock('child_process', () => ({
  spawn: vi.fn(),
}));

describe('ProcessManager', () => {
  let processManager: IProcessManager;
  let mockChildProcess: any;
  let consoleSpy: any;

  beforeEach(() => {
    processManager = createProcessManager();

    // Create a mock child process that extends EventEmitter
    mockChildProcess = new EventEmitter();
    mockChildProcess.stdout = new EventEmitter();
    mockChildProcess.stderr = new EventEmitter();
    mockChildProcess.kill = vi.fn();
    mockChildProcess.killed = false;

    vi.mocked(spawn).mockReturnValue(mockChildProcess);
    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
  });

  afterEach(() => {
    vi.clearAllMocks();
    consoleSpy.mockRestore();
  });

  describe('runProcess', () => {
    it('should run a process successfully', async () => {
      const command = 'test-command';
      const args = ['--flag', 'value'];

      // Start the promise
      const resultPromise = processManager.runProcess(command, args);

      // Simulate successful process
      mockChildProcess.stdout.emit('data', 'output data');
      mockChildProcess.stderr.emit('data', '');
      mockChildProcess.emit('close', 0);

      const result = await resultPromise;

      expect(result).toEqual({
        stdout: 'output data',
        stderr: '',
        exitCode: 0,
      });

      expect(spawn).toHaveBeenCalledWith(command, args, {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          PLUGHOST_SAMPLE_RATE: '48000',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
        }),
      });
    });

    it('should handle process errors', async () => {
      const command = 'failing-command';
      const args = ['--fail'];

      const resultPromise = processManager.runProcess(command, args);

      // Simulate process error
      mockChildProcess.emit('error', new Error('Command not found'));

      await expect(resultPromise).rejects.toThrow('Process error: Command not found');
    });

    it('should handle non-zero exit codes', async () => {
      const command = 'test-command';
      const args = ['--exit', '1'];

      const resultPromise = processManager.runProcess(command, args);

      // Simulate process failure
      mockChildProcess.stderr.emit('data', 'Error message');
      mockChildProcess.emit('close', 1);

      await expect(resultPromise).rejects.toThrow(/Process exited with code 1/);
    });

    it('should handle timeout with no progress', async () => {
      const command = 'slow-command';
      const args = ['--slow'];
      const config = { timeoutMs: 100 };

      const resultPromise = processManager.runProcess(command, args, config);

      // Don't emit any data to trigger timeout
      // Wait for timeout to occur
      await expect(resultPromise).rejects.toThrow(/timed out after 100ms with no progress/);
      expect(mockChildProcess.kill).toHaveBeenCalledWith('SIGKILL');
    });

    it('should reset progress timer on stdout data', async () => {
      const command = 'test-command';
      const args = ['--data'];
      const config = { timeoutMs: 200 };

      const resultPromise = processManager.runProcess(command, args, config);

      // Emit data to reset progress timer
      setTimeout(() => {
        mockChildProcess.stdout.emit('data', 'chunk1');
      }, 50);

      setTimeout(() => {
        mockChildProcess.stdout.emit('data', 'chunk2');
      }, 150);

      // Complete the process successfully after progress updates
      setTimeout(() => {
        mockChildProcess.emit('close', 0);
      }, 160);

      const result = await resultPromise;
      expect(result.stdout).toBe('chunk1chunk2');
    });

    it('should show progress dots for list operations', async () => {
      const stdoutWriteSpy = vi.spyOn(process.stdout, 'write').mockImplementation(() => true);
      const command = 'plughost';
      const args = ['--list'];

      const resultPromise = processManager.runProcess(command, args);

      mockChildProcess.stdout.emit('data', 'plugin list data');
      mockChildProcess.emit('close', 0);

      await resultPromise;

      expect(stdoutWriteSpy).toHaveBeenCalledWith('.');
      stdoutWriteSpy.mockRestore();
    });

    it('should merge custom environment variables', async () => {
      const command = 'test-command';
      const args = ['--env-test'];
      const config = {
        env: {
          CUSTOM_VAR: 'custom-value',
          PLUGHOST_SAMPLE_RATE: '44100', // Override default
        },
      };

      const resultPromise = processManager.runProcess(command, args, config);
      mockChildProcess.emit('close', 0);
      await resultPromise;

      expect(spawn).toHaveBeenCalledWith(command, args, {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          CUSTOM_VAR: 'custom-value',
          PLUGHOST_SAMPLE_RATE: '44100', // Should be overridden
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
        }),
      });
    });
  });

  describe('createProcessManager factory', () => {
    it('should create a valid process manager instance', () => {
      const pm = createProcessManager();

      expect(pm).toBeDefined();
      expect(typeof pm.runProcess).toBe('function');
    });

    it('should create independent instances', () => {
      const pm1 = createProcessManager();
      const pm2 = createProcessManager();

      expect(pm1).not.toBe(pm2);
      expect(pm1).toBeInstanceOf(ProcessManager);
      expect(pm2).toBeInstanceOf(ProcessManager);
    });
  });
});

describe('PlughostProcessManager', () => {
  let plughostManager: PlughostProcessManager;
  let mockChildProcess: any;
  const testPlughostPath = '/test/path/plughost';

  beforeEach(() => {
    plughostManager = createPlughostProcessManager(testPlughostPath);

    mockChildProcess = new EventEmitter();
    mockChildProcess.stdout = new EventEmitter();
    mockChildProcess.stderr = new EventEmitter();
    mockChildProcess.kill = vi.fn();
    mockChildProcess.killed = false;

    vi.mocked(spawn).mockReturnValue(mockChildProcess);
  });

  afterEach(() => {
    vi.clearAllMocks();
  });

  describe('runPlughost', () => {
    it('should run plughost with correct path and arguments', async () => {
      const args = ['--list'];

      const resultPromise = plughostManager.runPlughost(args);

      mockChildProcess.stdout.emit('data', 'plugin list output');
      mockChildProcess.emit('close', 0);

      const result = await resultPromise;

      expect(result).toBe('plugin list output');
      expect(spawn).toHaveBeenCalledWith(testPlughostPath, args, expect.any(Object));
    });

    it('should handle custom timeout', async () => {
      const args = ['--interrogate', 'plugin'];
      const customTimeout = 30000;

      const resultPromise = plughostManager.runPlughost(args, customTimeout);

      mockChildProcess.stdout.emit('data', 'interrogation result');
      mockChildProcess.emit('close', 0);

      await resultPromise;

      // Verify that the custom timeout was passed through
      expect(spawn).toHaveBeenCalledWith(testPlughostPath, args,
        expect.objectContaining({ timeoutMs: customTimeout })
      );
    });
  });

  describe('isProblematicPlugin', () => {
    it('should identify known problematic plugins', () => {
      const problematicPlugins = [
        'ZamVerb',
        'ZamTube',
        'ZamAutoSat',
        'ZamNoise',
        'ZaMaximX2',
        'ZamComp',
      ];

      problematicPlugins.forEach(pluginName => {
        expect(plughostManager.isProblematicPlugin(pluginName)).toBe(true);
      });
    });

    it('should handle plugins with problematic names as substrings', () => {
      expect(plughostManager.isProblematicPlugin('SuperZamVerbPro')).toBe(true);
      expect(plughostManager.isProblematicPlugin('MyZamTube')).toBe(true);
      expect(plughostManager.isProblematicPlugin('ZamComp v2')).toBe(true);
    });

    it('should not flag non-problematic plugins', () => {
      const goodPlugins = [
        'Serum',
        'Massive',
        'FabFilter Pro-Q',
        'Waves SSL',
        'Native Instruments Kontakt',
        'Random Plugin Name',
      ];

      goodPlugins.forEach(pluginName => {
        expect(plughostManager.isProblematicPlugin(pluginName)).toBe(false);
      });
    });

    it('should handle empty and null plugin names', () => {
      expect(plughostManager.isProblematicPlugin('')).toBe(false);
      expect(plughostManager.isProblematicPlugin('   ')).toBe(false);
    });

    it('should be case sensitive for plugin names', () => {
      // The implementation appears to be case-sensitive
      expect(plughostManager.isProblematicPlugin('zamverb')).toBe(false);
      expect(plughostManager.isProblematicPlugin('ZAMVERB')).toBe(false);
      expect(plughostManager.isProblematicPlugin('ZamVerb')).toBe(true);
    });
  });

  describe('createPlughostProcessManager factory', () => {
    it('should create a valid plughost process manager', () => {
      const pm = createPlughostProcessManager('/test/plughost');

      expect(pm).toBeDefined();
      expect(typeof pm.runPlughost).toBe('function');
      expect(typeof pm.isProblematicPlugin).toBe('function');
    });

    it('should create instances with different paths', () => {
      const pm1 = createPlughostProcessManager('/path1/plughost');
      const pm2 = createPlughostProcessManager('/path2/plughost');

      expect(pm1).not.toBe(pm2);
      expect(pm1).toBeInstanceOf(PlughostProcessManager);
      expect(pm2).toBeInstanceOf(PlughostProcessManager);
    });
  });

  describe('inheritance from ProcessManager', () => {
    it('should inherit runProcess method', async () => {
      const command = 'other-command';
      const args = ['--test'];

      const resultPromise = plughostManager.runProcess(command, args);

      mockChildProcess.stdout.emit('data', 'test output');
      mockChildProcess.emit('close', 0);

      const result = await resultPromise;

      expect(result.stdout).toBe('test output');
      expect(spawn).toHaveBeenCalledWith(command, args, expect.any(Object));
    });
  });
});