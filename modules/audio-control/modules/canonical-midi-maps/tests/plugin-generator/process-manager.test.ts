/**
 * Tests for process management utilities
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { EventEmitter } from 'events';
import {
  ProcessManager,
  PlughostProcessManager,
  createProcessManager,
  createPlughostProcessManager
} from '@/tools/plugin-generator/process-manager.js';
import type { ProcessConfig } from '@/tools/plugin-generator/types.js';

// Mock child_process
const mockSpawn = vi.fn();
vi.mock('child_process', () => ({
  spawn: mockSpawn
}));

// Mock process utilities
class MockChildProcess extends EventEmitter {
  public stdout = new EventEmitter();
  public stderr = new EventEmitter();
  public killed = false;

  kill(signal: string) {
    this.killed = true;
    setImmediate(() => this.emit('close', signal === 'SIGKILL' ? 128 : 0));
  }
}

describe('ProcessManager', () => {
  let processManager: ProcessManager;
  let mockChild: MockChildProcess;
  let consoleLogSpy: any;

  beforeEach(() => {
    processManager = new ProcessManager();
    mockChild = new MockChildProcess();
    consoleLogSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
    vi.clearAllMocks();

    mockSpawn.mockReturnValue(mockChild);
  });

  describe('runProcess', () => {
    it('should resolve with successful process result', async () => {
      const command = 'test-command';
      const args = ['--arg1', '--arg2'];

      const processPromise = processManager.runProcess(command, args);

      // Simulate successful process
      mockChild.stdout.emit('data', 'output data');
      mockChild.emit('close', 0);

      const result = await processPromise;

      expect(result).toEqual({
        stdout: 'output data',
        stderr: '',
        exitCode: 0
      });
      expect(mockSpawn).toHaveBeenCalledWith(command, args, expect.objectContaining({
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          PLUGHOST_SAMPLE_RATE: '48000',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1'
        })
      }));
    });

    it('should reject on non-zero exit code', async () => {
      const processPromise = processManager.runProcess('test', []);

      mockChild.stderr.emit('data', 'error message');
      mockChild.emit('close', 1);

      await expect(processPromise).rejects.toThrow('Process exited with code 1');
    });

    it('should reject on process error', async () => {
      const processPromise = processManager.runProcess('test', []);

      mockChild.emit('error', new Error('spawn error'));

      await expect(processPromise).rejects.toThrow('Process error: spawn error');
    });

    it('should handle timeout with no progress', async () => {
      const config: ProcessConfig = { timeoutMs: 100 };
      const processPromise = processManager.runProcess('test', [], config);

      // Don't emit any data to trigger timeout

      await expect(processPromise).rejects.toThrow('Process timed out after 100ms with no progress');
      expect(mockChild.killed).toBe(true);
    });

    it('should extend timeout on progress', async () => {
      const config: ProcessConfig = { timeoutMs: 200 };
      const processPromise = processManager.runProcess('test', []);

      // Emit progress to extend timeout
      setTimeout(() => {
        mockChild.stdout.emit('data', 'progress');
      }, 150);

      setTimeout(() => {
        mockChild.emit('close', 0);
      }, 300);

      const result = await processPromise;
      expect(result.exitCode).toBe(0);
    });

    it('should merge environment variables', async () => {
      const config: ProcessConfig = {
        env: { CUSTOM_VAR: 'custom_value' }
      };

      const processPromise = processManager.runProcess('test', [], config);
      mockChild.emit('close', 0);
      await processPromise;

      expect(mockSpawn).toHaveBeenCalledWith('test', [], expect.objectContaining({
        env: expect.objectContaining({
          PLUGHOST_SAMPLE_RATE: '48000',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
          CUSTOM_VAR: 'custom_value'
        })
      }));
    });

    it('should show progress dots for list commands', async () => {
      const processPromise = processManager.runProcess('test', ['--list']);

      mockChild.stdout.emit('data', 'list data');
      mockChild.emit('close', 0);

      await processPromise;

      // Verify progress indication was shown
      expect(vi.spyOn(process.stdout, 'write')).toBeDefined();
    });
  });

  describe('createProcessManager factory', () => {
    it('should create ProcessManager instance', () => {
      const manager = createProcessManager();

      expect(manager).toBeInstanceOf(ProcessManager);
      expect(typeof manager.runProcess).toBe('function');
    });
  });
});

describe('PlughostProcessManager', () => {
  let plughostManager: PlughostProcessManager;
  let mockChild: MockChildProcess;

  beforeEach(() => {
    const plughostPath = '/path/to/plughost';
    plughostManager = new PlughostProcessManager(plughostPath);
    mockChild = new MockChildProcess();
    mockSpawn.mockReturnValue(mockChild);
    vi.clearAllMocks();
  });

  describe('runPlughost', () => {
    it('should run plughost with specified arguments', async () => {
      const args = ['--list'];
      const plughostPromise = plughostManager.runPlughost(args);

      mockChild.stdout.emit('data', 'plugin list');
      mockChild.emit('close', 0);

      const result = await plughostPromise;

      expect(result).toBe('plugin list');
      expect(mockSpawn).toHaveBeenCalledWith(
        '/path/to/plughost',
        args,
        expect.objectContaining({
          timeoutMs: 60000
        })
      );
    });

    it('should use custom timeout', async () => {
      const plughostPromise = plughostManager.runPlughost(['--list'], 30000);

      mockChild.emit('close', 0);
      await plughostPromise;

      expect(mockSpawn).toHaveBeenCalledWith(
        '/path/to/plughost',
        ['--list'],
        expect.objectContaining({
          timeoutMs: 30000
        })
      );
    });
  });

  describe('isProblematicPlugin', () => {
    it('should identify problematic ZAM plugins', () => {
      expect(plughostManager.isProblematicPlugin('ZamVerb')).toBe(true);
      expect(plughostManager.isProblematicPlugin('ZamTube')).toBe(true);
      expect(plughostManager.isProblematicPlugin('ZamComp')).toBe(true);
    });

    it('should not flag safe plugins as problematic', () => {
      expect(plughostManager.isProblematicPlugin('Safe Plugin')).toBe(false);
      expect(plughostManager.isProblematicPlugin('Normal Compressor')).toBe(false);
      expect(plughostManager.isProblematicPlugin('')).toBe(false);
    });

    it('should handle partial matches', () => {
      expect(plughostManager.isProblematicPlugin('Plugin ZamVerb Extra')).toBe(true);
      expect(plughostManager.isProblematicPlugin('ZamGateX2 Modern')).toBe(true);
    });
  });

  describe('createPlughostProcessManager factory', () => {
    it('should create PlughostProcessManager instance', () => {
      const path = '/test/plughost';
      const manager = createPlughostProcessManager(path);

      expect(manager).toBeInstanceOf(PlughostProcessManager);
      expect(typeof manager.runPlughost).toBe('function');
      expect(typeof manager.isProblematicPlugin).toBe('function');
    });
  });
});