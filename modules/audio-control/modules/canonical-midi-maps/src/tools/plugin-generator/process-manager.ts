/**
 * Process management utility for running plughost commands
 */

import { spawn } from 'child_process';
import type { ProcessConfig, ProcessResult } from './types.js';

/**
 * Interface for process management
 */
export interface IProcessManager {
  runProcess(command: string, args: string[], config?: ProcessConfig): Promise<ProcessResult>;
}

/**
 * Default implementation of process management for plughost
 */
export class ProcessManager implements IProcessManager {
  private readonly defaultEnv = {
    PLUGHOST_SAMPLE_RATE: '48000', // Match plugin expectations
    JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1' // Prevent UAD plugin conflicts
  };

  /**
   * Run a process with optional configuration
   */
  async runProcess(command: string, args: string[], config: ProcessConfig = {}): Promise<ProcessResult> {
    const { timeoutMs = 60000, env = {}, ...processConfig } = config;

    return new Promise((resolve, reject) => {
      console.log(`🔧 Running: ${command} ${args.join(' ')}`);

      const child = spawn(command, args, {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: {
          ...process.env,
          ...this.defaultEnv,
          ...env
        },
        ...processConfig
      });

      let stdout = '';
      let stderr = '';
      let lastProgress = Date.now();

      child.stdout?.on('data', (data) => {
        stdout += data.toString();
        lastProgress = Date.now();
        // Show some progress for long operations
        if (args.includes('--list')) {
          process.stdout.write('.');
        }
      });

      child.stderr?.on('data', (data) => {
        stderr += data.toString();
        lastProgress = Date.now();
      });

      child.on('close', (code) => {
        const result: ProcessResult = {
          stdout,
          stderr,
          exitCode: code ?? -1
        };

        if (code === 0) {
          resolve(result);
        } else {
          const errorMsg = `Process exited with code ${code}${stderr ? `: ${stderr.slice(0, 500)}...` : ''}`;
          reject(new Error(errorMsg));
        }
      });

      child.on('error', (error) => {
        reject(new Error(`Process error: ${error.message}`));
      });

      // Progressive timeout with progress checking
      const checkProgress = setInterval(() => {
        if (Date.now() - lastProgress > timeoutMs) {
          clearInterval(checkProgress);
          child.kill('SIGKILL');
          reject(new Error(`Process timed out after ${timeoutMs}ms with no progress`));
        }
      }, 5000);

      // Final timeout
      setTimeout(() => {
        clearInterval(checkProgress);
        if (!child.killed) {
          child.kill('SIGKILL');
          reject(new Error(`Process killed after ${timeoutMs}ms maximum timeout`));
        }
      }, timeoutMs);
    });
  }
}

/**
 * Specialized plughost process manager
 */
export class PlughostProcessManager extends ProcessManager {
  constructor(private readonly plughostPath: string) {
    super();
  }

  /**
   * Run plughost with specific arguments
   */
  async runPlughost(args: string[], timeoutMs = 60000): Promise<string> {
    const result = await this.runProcess(this.plughostPath, args, { timeoutMs });
    return result.stdout;
  }

  /**
   * Check if a plugin is known to be problematic
   */
  isProblematicPlugin(pluginName: string): boolean {
    const problematicPlugins = [
      'ZamVerb', 'ZamTube', 'ZamAutoSat', 'ZamNoise', 'ZaMaximX2',
      'ZamPhono', 'ZaMultiComp', 'ZaMultiCompX2', 'ZamGrains',
      'ZamDynamicEQ', 'ZamDelay', 'ZamHeadX2', 'ZamGateX2',
      'ZamGate', 'ZamGEQ31', 'ZamEQ2', 'ZamCompX2', 'ZamComp'
    ];

    return problematicPlugins.some(problem =>
      pluginName.includes(problem)
    );
  }
}

/**
 * Factory function to create process manager
 */
export function createProcessManager(): IProcessManager {
  return new ProcessManager();
}

/**
 * Factory function to create plughost process manager
 */
export function createPlughostProcessManager(plughostPath: string): PlughostProcessManager {
  return new PlughostProcessManager(plughostPath);
}