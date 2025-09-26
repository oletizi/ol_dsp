/**
 * Test suite for process manager functionality
 */

import { describe, expect, it, beforeEach, vi, afterEach } from 'vitest';
import { EventEmitter } from 'events';
import {
  ProcessManager,
  PlughostProcessManager,
  createProcessManager,
  createPlughostProcessManager,
} from '@/tools/plugin-generator/process-manager.js';
import type { ProcessConfig } from '@/tools/plugin-generator/types.js';

// Mock child_process
const mockSpawn = vi.fn();
vi.mock('child_process', () => ({
  spawn: mockSpawn,
}));

class MockChildProcess extends EventEmitter {
  killed = false;
  stdout = new EventEmitter();
  stderr = new EventEmitter();

  kill(signal?: string) {
    this.killed = true;
    // Simulate process being killed
    setTimeout(() => this.emit('close', -1), 10);
  }
}

describe('ProcessManager', () => {
  let processManager: ProcessManager;
  let mockChild: MockChildProcess;

  beforeEach(() => {
    processManager = new ProcessManager();
    mockChild = new MockChildProcess();
    mockSpawn.mockReturnValue(mockChild);
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
    vi.clearAllMocks();
  });

  describe('runProcess', () => {
    it('should execute a process and return result on success', async () => {
      const resultPromise = processManager.runProcess('test-command', ['arg1', 'arg2']);

      // Simulate process output
      mockChild.stdout.emit('data', 'output line 1\n');
      mockChild.stdout.emit('data', 'output line 2\n');
      mockChild.stderr.emit('data', 'error line 1\n');

      // Simulate successful completion
      mockChild.emit('close', 0);

      const result = await resultPromise;

      expect(mockSpawn).toHaveBeenCalledWith('test-command', ['arg1', 'arg2'], {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          PLUGHOST_SAMPLE_RATE: '48000',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
        }),
      });

      expect(result).toEqual({
        stdout: 'output line 1\noutput line 2\n',
        stderr: 'error line 1\n',
        exitCode: 0,
      });
    });

    it('should reject on non-zero exit code', async () => {
      const resultPromise = processManager.runProcess('test-command', ['arg1']);

      mockChild.stderr.emit('data', 'Error occurred');
      mockChild.emit('close', 1);

      await expect(resultPromise).rejects.toThrow('Process exited with code 1: Error occurred...');
    });

    it('should reject on process error', async () => {
      const resultPromise = processManager.runProcess('test-command', ['arg1']);

      mockChild.emit('error', new Error('Process startup failed'));

      await expect(resultPromise).rejects.toThrow('Process error: Process startup failed');
    });

    it('should handle timeout with progress checking', async () => {
      const config: ProcessConfig = { timeoutMs: 1000 };
      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);

      // Don't send any data (no progress)
      vi.advanceTimersByTime(1100);

      await expect(resultPromise).rejects.toThrow('Process timed out after 1000ms with no progress');
      expect(mockChild.killed).toBe(true);
    });

    it('should reset timeout on progress', async () => {
      const config: ProcessConfig = { timeoutMs: 1000 };
      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);

      // Send progress just before timeout
      vi.advanceTimersByTime(900);
      mockChild.stdout.emit('data', 'progress');

      // Advance more time - should not timeout yet
      vi.advanceTimersByTime(900);

      // Send more progress
      mockChild.stdout.emit('data', 'more progress');

      // Complete the process
      mockChild.emit('close', 0);

      const result = await resultPromise;
      expect(result.stdout).toBe('progressmore progress');
      expect(mockChild.killed).toBe(false);
    });

    it('should use smaller check intervals for short timeouts', async () => {
      const config: ProcessConfig = { timeoutMs: 100 };
      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);

      // Should use interval of 10ms (100/10) for such a short timeout
      vi.advanceTimersByTime(110);

      await expect(resultPromise).rejects.toThrow('Process timed out after 100ms with no progress');
    });

    it('should show progress dots for list commands', async () => {
      const consoleSpy = vi.spyOn(process.stdout, 'write').mockImplementation(() => true);

      const resultPromise = processManager.runProcess('test-command', ['--list']);

      mockChild.stdout.emit('data', 'some data');
      mockChild.emit('close', 0);

      await resultPromise;

      expect(consoleSpy).toHaveBeenCalledWith('.');
      consoleSpy.mockRestore();
    });

    it('should merge custom environment variables', async () => {
      const config: ProcessConfig = {
        env: {
          CUSTOM_VAR: 'custom_value',
          PLUGHOST_SAMPLE_RATE: '44100', // Override default
        },
      };

      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);
      mockChild.emit('close', 0);
      await resultPromise;

      expect(mockSpawn).toHaveBeenCalledWith('test-command', ['arg1'], {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          CUSTOM_VAR: 'custom_value',
          PLUGHOST_SAMPLE_RATE: '44100',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
        }),
      });
    });

    it('should pass through additional process config', async () => {
      const config: ProcessConfig = {
        timeoutMs: 5000,
        env: { TEST: 'value' },
        args: ['extra'] as any, // This would go to spawn options
      };

      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);
      mockChild.emit('close', 0);
      await resultPromise;

      expect(mockSpawn).toHaveBeenCalledWith('test-command', ['arg1'], {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: expect.objectContaining({
          TEST: 'value',
          PLUGHOST_SAMPLE_RATE: '48000',
          JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1',
        }),
        args: ['extra'],
      });
    });

    it('should handle final timeout as backup', async () => {
      const config: ProcessConfig = { timeoutMs: 1000 };
      const resultPromise = processManager.runProcess('test-command', ['arg1'], config);

      // Keep sending progress to avoid progress timeout
      const progressInterval = setInterval(() => {
        mockChild.stdout.emit('data', '.');
      }, 100);

      // Advance to final timeout
      vi.advanceTimersByTime(1100);

      clearInterval(progressInterval);

      await expect(resultPromise).rejects.toThrow('Process killed after 1000ms maximum timeout');
    });
  });

  describe('factory function', () => {
    it('should create a functional process manager', () => {
      const manager = createProcessManager();
      expect(manager).toBeDefined();
      expect(manager).toBeInstanceOf(ProcessManager);
    });
  });
});

describe('PlughostProcessManager', () => {
  let plughostManager: PlughostProcessManager;
  let mockChild: MockChildProcess;

  beforeEach(() => {
    plughostManager = new PlughostProcessManager('/path/to/plughost');
    mockChild = new MockChildProcess();
    mockSpawn.mockReturnValue(mockChild);
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
    vi.clearAllMocks();
  });

  describe('runPlughost', () => {
    it('should run plughost with correct path and arguments', async () => {
      const resultPromise = plughostManager.runPlughost(['--list', '--json']);

      mockChild.stdout.emit('data', '{"plugins": []}');
      mockChild.emit('close', 0);

      const result = await resultPromise;

      expect(mockSpawn).toHaveBeenCalledWith('/path/to/plughost', ['--list', '--json'], expect.any(Object));
      expect(result).toBe('{"plugins": []}');
    });

    it('should use custom timeout', async () => {
      const resultPromise = plughostManager.runPlughost(['--interrogate', 'plugin'], 5000);

      vi.advanceTimersByTime(5100);

      await expect(resultPromise).rejects.toThrow('Process timed out after 5000ms with no progress');
    });

    it('should use default timeout when not specified', async () => {
      const resultPromise = plughostManager.runPlughost(['--list']);

      vi.advanceTimersByTime(60100);

      await expect(resultPromise).rejects.toThrow('Process timed out after 60000ms with no progress');
    });
  });

  describe('isProblematicPlugin', () => {
    it('should identify problematic plugins', () => {
      const problematicPlugins = [
        'ZamVerb',
        'ZamTube',
        'ZamAutoSat',
        'ZamNoise',
        'ZaMaximX2',
        'ZamPhono',
        'ZaMultiComp',
        'ZaMultiCompX2',
        'ZamGrains',
        'ZamDynamicEQ',
        'ZamDelay',
        'ZamHeadX2',
        'ZamGateX2',
        'ZamGate',
        'ZamGEQ31',
        'ZamEQ2',
        'ZamCompX2',
        'ZamComp',
      ];

      problematicPlugins.forEach((plugin) => {
        expect(plughostManager.isProblematicPlugin(plugin)).toBe(true);
      });
    });

    it('should not identify normal plugins as problematic', () => {
      const normalPlugins = [
        'Massive X',
        'Serum',
        'FabFilter Pro-Q 3',
        'Waves C4',
        'Native Instruments Battery',
      ];

      normalPlugins.forEach((plugin) => {
        expect(plughostManager.isProblematicPlugin(plugin)).toBe(false);
      });
    });

    it('should handle partial name matches', () => {
      expect(plughostManager.isProblematicPlugin('Some ZamVerb Plugin')).toBe(true);
      expect(plughostManager.isProblematicPlugin('Plugin with ZamTube inside')).toBe(true);
      expect(plughostManager.isProblematicPlugin('Regular Plugin')).toBe(false);
    });

    it('should handle empty or undefined plugin names', () => {
      expect(plughostManager.isProblematicPlugin('')).toBe(false);
      expect(plughostManager.isProblematicPlugin(undefined as any)).toBe(false);
    });
  });

  describe('factory function', () => {
    it('should create a functional plughost process manager', () => {
      const manager = createPlughostProcessManager('/test/path');
      expect(manager).toBeDefined();
      expect(manager).toBeInstanceOf(PlughostProcessManager);
    });
  });

  describe('inheritance', () => {
    it('should inherit from ProcessManager', () => {
      expect(plughostManager).toBeInstanceOf(ProcessManager);
    });

    it('should be able to use base ProcessManager methods', async () => {
      const resultPromise = plughostManager.runProcess('other-command', ['arg']);

      mockChild.stdout.emit('data', 'output');
      mockChild.emit('close', 0);

      const result = await resultPromise;

      expect(result.stdout).toBe('output');
    });
  });

  describe('error scenarios', () => {
    it('should handle plughost errors gracefully', async () => {
      const resultPromise = plughostManager.runPlughost(['--invalid-arg']);

      mockChild.stderr.emit('data', 'Unknown argument: --invalid-arg');
      mockChild.emit('close', 1);

      await expect(resultPromise).rejects.toThrow('Process exited with code 1');
    });

    it('should handle timeout during plugin interrogation', async () => {
      const resultPromise = plughostManager.runPlughost(['--interrogate', 'problematic-plugin'], 1000);

      // Simulate stuck plugin - no output or progress
      vi.advanceTimersByTime(1100);

      await expect(resultPromise).rejects.toThrow('Process timed out after 1000ms with no progress');
      expect(mockChild.killed).toBe(true);
    });
  });
});