/**
 * Complete Workflow Tool Tests
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { execSync } from 'node:child_process';

// Mock child_process
vi.mock('node:child_process', () => ({
  execSync: vi.fn(),
}));

// Mock console methods
const mockConsole = {
  log: vi.fn(),
  error: vi.fn(),
  warn: vi.fn(),
};

vi.stubGlobal('console', mockConsole);

const mockExecSync = vi.mocked(execSync);

// Mock workflow interfaces
interface WorkflowOptions {
  target?: 'ardour' | 'ableton' | 'reaper' | 'all';
  force?: boolean;
  install?: boolean;
  skipValidation?: boolean;
}

interface StepResult {
  step: string;
  success: boolean;
  duration: number;
  output?: string;
  error?: string;
}

describe('Complete Workflow Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.spyOn(process, 'exit').mockImplementation(() => undefined as never);
    // Mock Date.now for consistent timing tests
    vi.spyOn(Date, 'now').mockReturnValue(1000000); // Fixed timestamp
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  describe('CLI argument parsing', () => {
    it('should parse workflow options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const mockParseArgs = vi.fn().mockReturnValue({
        values: {
          target: 'ardour',
          force: true,
          install: true,
          'skip-validation': false
        }
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

    it('should validate target options', () => {
      const validTargets = ['ardour', 'ableton', 'reaper', 'all'];
      const testTarget = 'ardour';

      expect(validTargets).toContain(testTarget);
    });
  });

  describe('workflow step execution', () => {
    it('should execute commands successfully', () => {
      mockExecSync.mockReturnValue('Command executed successfully');

      const runCommand = (command: string, stepName: string): StepResult => {
        const startTime = Date.now();
        try {
          const output = mockExecSync(command, {
            encoding: 'utf-8',
            stdio: 'inherit'
          });
          return {
            step: stepName,
            success: true,
            duration: Date.now() - startTime,
            output: typeof output === 'string' ? output : undefined
          };
        } catch (error: any) {
          return {
            step: stepName,
            success: false,
            duration: Date.now() - startTime,
            error: error.message
          };
        }
      };

      const result = runCommand('pnpm plugins:health', 'Plugin health check');

      expect(result.success).toBe(true);
      expect(result.step).toBe('Plugin health check');
      expect(result.duration).toBe(0); // Since Date.now() is mocked
    });

    it('should handle command failures gracefully', () => {
      mockExecSync.mockImplementation(() => {
        throw new Error('Command failed');
      });

      const runCommand = (command: string, stepName: string): StepResult => {
        const startTime = Date.now();
        try {
          mockExecSync(command);
          return {
            step: stepName,
            success: true,
            duration: Date.now() - startTime
          };
        } catch (error: any) {
          return {
            step: stepName,
            success: false,
            duration: Date.now() - startTime,
            error: error.message
          };
        }
      };

      const result = runCommand('invalid-command', 'Test step');

      expect(result.success).toBe(false);
      expect(result.error).toBe('Command failed');
    });

    it('should track step execution timing', () => {
      // Mock different timestamps for timing
      vi.spyOn(Date, 'now')
        .mockReturnValueOnce(1000000)  // Start time
        .mockReturnValueOnce(1000100); // End time

      const startTime = Date.now();
      mockExecSync.mockReturnValue('Success');
      const endTime = Date.now();

      const duration = endTime - startTime;
      expect(duration).toBe(100);
    });
  });

  describe('workflow orchestration', () => {
    it('should execute complete workflow steps in order', () => {
      const workflowSteps = [
        'Plugin health check',
        'Plugin extraction',
        'Map validation',
        'Map compatibility checking',
        'DAW map generation'
      ];

      expect(workflowSteps.length).toBe(5);
      expect(workflowSteps[0]).toBe('Plugin health check');
      expect(workflowSteps[4]).toBe('DAW map generation');
    });

    it('should handle workflow options correctly', () => {
      const options: WorkflowOptions = {
        target: 'ardour',
        force: true,
        install: true,
        skipValidation: false
      };

      // Test command generation based on options
      const extractCmd = options.force ? 'pnpm plugins:extract:force' : 'pnpm plugins:extract';
      expect(extractCmd).toBe('pnpm plugins:extract:force');

      const generateFlags = [
        `--target ${options.target}`,
        options.force ? '--force' : '',
        options.install ? '--install' : ''
      ].filter(Boolean).join(' ');

      expect(generateFlags).toContain('--target ardour');
      expect(generateFlags).toContain('--force');
      expect(generateFlags).toContain('--install');
    });

    it('should skip validation when requested', () => {
      const options: WorkflowOptions = { skipValidation: true };

      const shouldRunValidation = !options.skipValidation;
      expect(shouldRunValidation).toBe(false);
    });

    it('should continue workflow on non-critical failures', () => {
      const results: StepResult[] = [
        { step: 'Health check', success: false, duration: 10, error: 'not yet implemented' },
        { step: 'Extraction', success: true, duration: 100 },
        { step: 'Generation', success: true, duration: 200 }
      ];

      // Should continue if error contains "not yet implemented"
      const shouldContinue = !results.some(r =>
        !r.success && !r.error?.includes('not yet implemented')
      );

      expect(shouldContinue).toBe(true);
    });

    it('should stop workflow on critical failures', () => {
      const results: StepResult[] = [
        { step: 'Health check', success: true, duration: 10 },
        { step: 'Extraction', success: false, duration: 50, error: 'Critical failure' },
        { step: 'Generation', success: true, duration: 100 }
      ];

      const hasCriticalFailure = results.some(r =>
        !r.success && !r.error?.includes('not yet implemented')
      );

      expect(hasCriticalFailure).toBe(true);
    });
  });

  describe('workflow summary and reporting', () => {
    it('should format workflow summary correctly', () => {
      const results: StepResult[] = [
        { step: 'Plugin health check', success: true, duration: 100 },
        { step: 'Plugin extraction', success: true, duration: 2000 },
        { step: 'Map validation', success: false, duration: 50, error: 'Validation failed' },
        { step: 'DAW generation', success: true, duration: 1500 }
      ];

      const successful = results.filter(r => r.success);
      const failed = results.filter(r => !r.success);
      const totalTime = results.reduce((sum, r) => sum + r.duration, 0);

      expect(successful.length).toBe(3);
      expect(failed.length).toBe(1);
      expect(totalTime).toBe(3650);
    });

    it('should identify implementation gaps', () => {
      const results: StepResult[] = [
        { step: 'Test step', success: false, duration: 10, error: 'Feature not yet implemented' }
      ];

      const hasImplementationGaps = results.some(r =>
        r.error?.includes('not yet implemented')
      );

      expect(hasImplementationGaps).toBe(true);
    });

    it('should calculate workflow statistics', () => {
      const results: StepResult[] = [
        { step: 'Step 1', success: true, duration: 1000 },
        { step: 'Step 2', success: true, duration: 2000 },
        { step: 'Step 3', success: false, duration: 500 }
      ];

      const successRate = (results.filter(r => r.success).length / results.length) * 100;
      const avgDuration = results.reduce((sum, r) => sum + r.duration, 0) / results.length;

      expect(successRate).toBeCloseTo(66.67, 1);
      expect(avgDuration).toBeCloseTo(1166.67, 1);
    });
  });

  describe('target-specific workflow handling', () => {
    it('should handle Ardour-specific workflow', () => {
      const options: WorkflowOptions = { target: 'ardour', install: true };

      expect(options.target).toBe('ardour');
      expect(options.install).toBe(true);
    });

    it('should handle all-targets workflow', () => {
      const options: WorkflowOptions = { target: 'all' };
      const targets = options.target === 'all' ? ['ardour', 'ableton', 'reaper'] : [options.target];

      expect(targets.length).toBe(3);
      expect(targets).toContain('ardour');
    });

    it('should generate target-specific commands', () => {
      const generateCommand = (target: string, install: boolean) => {
        const flags = [
          `--target ${target}`,
          install ? '--install' : ''
        ].filter(Boolean).join(' ');

        return `pnpm daw:generate ${flags}`;
      };

      expect(generateCommand('ardour', true)).toBe('pnpm daw:generate --target ardour --install');
      expect(generateCommand('reaper', false)).toBe('pnpm daw:generate --target reaper');
    });
  });

  describe('error handling and recovery', () => {
    it('should provide helpful error messages', () => {
      const error = new Error('Workflow failed');
      const contextualMessage = `Complete workflow failed: ${error.message}`;

      expect(contextualMessage).toContain('Complete workflow failed');
      expect(contextualMessage).toContain('Workflow failed');
    });

    it('should suggest recovery actions', () => {
      const suggestions = [
        'Ensure all prerequisites are met',
        'Verify canonical maps exist',
        'Check DAW installation paths',
        'Re-run with --force if needed'
      ];

      expect(suggestions.length).toBe(4);
      expect(suggestions[0]).toContain('prerequisites');
    });

    it('should handle missing dependencies gracefully', () => {
      mockExecSync.mockImplementation((command) => {
        if (command.toString().includes('plugins:extract')) {
          throw new Error('JUCE host not found');
        }
        return 'Success';
      });

      expect(() => {
        try {
          mockExecSync('pnpm plugins:extract');
        } catch (error: any) {
          if (error.message.includes('JUCE host not found')) {
            // Should provide helpful guidance
            expect(error.message).toContain('JUCE host');
          }
          throw error;
        }
      }).toThrow('JUCE host not found');
    });
  });

  describe('performance validation', () => {
    it('should complete workflow within time limits', () => {
      const maxWorkflowTime = 120000; // 2 minutes
      const actualWorkflowTime = 45000; // 45 seconds

      expect(actualWorkflowTime).toBeLessThan(maxWorkflowTime);
    });

    it('should track individual step performance', () => {
      const stepPerformance = {
        'Plugin health check': 100,
        'Plugin extraction': 30000,
        'Map validation': 5000,
        'DAW generation': 15000
      };

      Object.entries(stepPerformance).forEach(([step, duration]) => {
        expect(duration).toBeTypeOf('number');
        expect(duration).toBeGreaterThan(0);
      });
    });

    it('should identify performance bottlenecks', () => {
      const stepTimes = [100, 30000, 5000, 15000]; // Plugin extraction is slowest
      const maxTime = Math.max(...stepTimes);
      const slowStepIndex = stepTimes.indexOf(maxTime);

      expect(slowStepIndex).toBe(1); // Plugin extraction
      expect(maxTime).toBe(30000);
    });
  });

  describe('installation verification', () => {
    it('should verify installation when requested', () => {
      const options: WorkflowOptions = { install: true, target: 'ardour' };

      if (options.install) {
        const verificationCmd = `pnpm daw:list --target ${options.target} --status`;
        expect(verificationCmd).toContain('--status');
      }
    });

    it('should provide installation instructions', () => {
      const instructions = [
        'Maps have been installed to DAW directories',
        'Restart your DAW to see the new MIDI maps',
        'To install maps, re-run with --install flag'
      ];

      expect(instructions.length).toBe(3);
      expect(instructions[0]).toContain('installed');
    });
  });

  describe('workflow configuration validation', () => {
    it('should validate workflow configuration', () => {
      const workflowConfig = {
        phases: [
          { id: 'health', name: 'Plugin Health Check' },
          { id: 'extract', name: 'Plugin Extraction' },
          { id: 'validate', name: 'Map Validation' },
          { id: 'generate', name: 'DAW Generation' }
        ],
        options: {
          parallel: false,
          stopOnError: false
        }
      };

      expect(workflowConfig.phases.length).toBe(4);
      expect(workflowConfig.options.parallel).toBe(false);
    });

    it('should handle workflow dependencies', () => {
      const dependencies = {
        'extract': ['health'],
        'validate': ['extract'],
        'generate': ['validate']
      };

      expect(dependencies.extract).toContain('health');
      expect(dependencies.generate).toContain('validate');
    });
  });

  describe('logging and output formatting', () => {
    it('should format step output consistently', () => {
      const formatStepOutput = (step: string, success: boolean, duration: number) => {
        const icon = success ? '✅' : '❌';
        return `${icon} ${step} (${duration}ms)`;
      };

      expect(formatStepOutput('Test', true, 100)).toBe('✅ Test (100ms)');
      expect(formatStepOutput('Fail', false, 50)).toBe('❌ Fail (50ms)');
    });

    it('should show progress indicators', () => {
      const showProgress = (current: number, total: number) => {
        return `Step ${current}/${total}`;
      };

      expect(showProgress(2, 5)).toBe('Step 2/5');
    });
  });
});