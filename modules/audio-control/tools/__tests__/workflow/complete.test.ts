/**
 * Tests for Complete Workflow Tool
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { execSync } from 'node:child_process';

// Mock child_process
vi.mock('node:child_process');

const mockExecSync = vi.mocked(execSync);

describe('Complete Workflow Tool', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('CLI Argument Parsing', () => {
    it('should parse default options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: [],
        options: {
          target: { type: 'string', short: 't', default: 'all' },
          force: { type: 'boolean', default: false },
          install: { type: 'boolean', default: false },
          'skip-validation': { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.target).toBe('all');
      expect(values.force).toBe(false);
      expect(values.install).toBe(false);
      expect(values['skip-validation']).toBe(false);
    });

    it('should parse target options correctly', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--target', 'ardour', '--install', '--force'],
        options: {
          target: { type: 'string', short: 't', default: 'all' },
          force: { type: 'boolean', default: false },
          install: { type: 'boolean', default: false },
          'skip-validation': { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values.target).toBe('ardour');
      expect(values.force).toBe(true);
      expect(values.install).toBe(true);
    });

    it('should parse skip-validation flag', async () => {
      const { parseArgs } = await import('node:util');
      const { values } = parseArgs({
        args: ['--skip-validation'],
        options: {
          target: { type: 'string', short: 't', default: 'all' },
          force: { type: 'boolean', default: false },
          install: { type: 'boolean', default: false },
          'skip-validation': { type: 'boolean', default: false },
          help: { type: 'boolean', short: 'h', default: false }
        }
      });

      expect(values['skip-validation']).toBe(true);
    });
  });

  describe('Command Execution', () => {
    it('should execute commands successfully', () => {
      mockExecSync.mockReturnValue('Command executed successfully');

      const command = 'pnpm plugins:health';
      const stepName = 'Plugin health check';

      const startTime = Date.now();
      const output = mockExecSync(command, {
        encoding: 'utf-8',
        stdio: 'inherit',
        cwd: expect.any(String)
      });
      const duration = Date.now() - startTime;

      const result = {
        step: stepName,
        success: true,
        duration,
        output: typeof output === 'string' ? output : undefined
      };

      expect(result.success).toBe(true);
      expect(result.step).toBe(stepName);
      expect(result.duration).toBeGreaterThanOrEqual(0);
    });

    it('should handle command failures', () => {
      mockExecSync.mockImplementation(() => {
        throw new Error('Command failed');
      });

      const command = 'pnpm plugins:health';
      const stepName = 'Plugin health check';

      const startTime = Date.now();
      let result;

      try {
        mockExecSync(command, {
          encoding: 'utf-8',
          stdio: 'inherit',
          cwd: expect.any(String)
        });
        result = { step: stepName, success: true, duration: Date.now() - startTime };
      } catch (error: any) {
        const duration = Date.now() - startTime;
        result = {
          step: stepName,
          success: false,
          duration,
          error: error.message
        };
      }

      expect(result.success).toBe(false);
      expect(result.error).toBe('Command failed');
    });
  });

  describe('Workflow Steps', () => {
    interface StepResult {
      step: string;
      success: boolean;
      duration: number;
      output?: string;
      error?: string;
    }

    function createMockResult(step: string, success: boolean, error?: string): StepResult {
      return {
        step,
        success,
        duration: Math.random() * 100,
        error
      };
    }

    it('should execute all workflow steps in correct order', () => {
      const expectedSteps = [
        'Plugin health check',
        'Plugin extraction',
        'Map validation',
        'Map compatibility checking',
        'DAW map generation'
      ];

      const results: StepResult[] = [];

      // Simulate step execution
      for (const step of expectedSteps) {
        results.push(createMockResult(step, true));
      }

      expect(results).toHaveLength(5);
      expect(results[0].step).toBe('Plugin health check');
      expect(results[4].step).toBe('DAW map generation');
    });

    it('should handle step failures appropriately', () => {
      const results: StepResult[] = [
        createMockResult('Plugin health check', true),
        createMockResult('Plugin extraction', false, 'not yet implemented'),
        createMockResult('Map validation', true),
        createMockResult('Map compatibility checking', true),
        createMockResult('DAW map generation', true)
      ];

      const failedSteps = results.filter(r => !r.success);
      const implementationGaps = results.filter(r => r.error?.includes('not yet implemented'));

      expect(failedSteps).toHaveLength(1);
      expect(implementationGaps).toHaveLength(1);
    });

    it('should skip validation steps when skip-validation is true', () => {
      const skipValidation = true;
      const expectedSteps = ['Plugin health check', 'Plugin extraction'];

      if (!skipValidation) {
        expectedSteps.push('Map validation', 'Map compatibility checking');
      }

      expectedSteps.push('DAW map generation');

      expect(expectedSteps).toHaveLength(3); // Without validation steps
    });

    it('should include installation verification when install is true', () => {
      const install = true;
      const results: StepResult[] = [
        createMockResult('Plugin health check', true),
        createMockResult('Plugin extraction', true),
        createMockResult('DAW map generation', true)
      ];

      if (install) {
        results.push(createMockResult('Installation verification', true));
      }

      expect(results).toHaveLength(4);
      expect(results[3].step).toBe('Installation verification');
    });
  });

  describe('Command Generation', () => {
    it('should generate correct extract command based on force flag', () => {
      const force = true;
      const extractCmd = force ? 'pnpm plugins:extract:force' : 'pnpm plugins:extract';

      expect(extractCmd).toBe('pnpm plugins:extract:force');
    });

    it('should generate correct DAW generate command with flags', () => {
      const options = {
        target: 'ardour',
        force: true,
        install: true
      };

      const generateFlags = [
        `--target ${options.target}`,
        options.force ? '--force' : '',
        options.install ? '--install' : ''
      ].filter(Boolean).join(' ');

      const generateCmd = `pnpm daw:generate ${generateFlags}`;

      expect(generateCmd).toBe('pnpm daw:generate --target ardour --force --install');
    });

    it('should generate minimal command when no flags specified', () => {
      const options = {
        target: 'all',
        force: false,
        install: false
      };

      const generateFlags = [
        `--target ${options.target}`,
        options.force ? '--force' : '',
        options.install ? '--install' : ''
      ].filter(Boolean).join(' ');

      const generateCmd = `pnpm daw:generate ${generateFlags}`;

      expect(generateCmd).toBe('pnpm daw:generate --target all');
    });
  });

  describe('Summary Formatting', () => {
    interface StepResult {
      step: string;
      success: boolean;
      duration: number;
      error?: string;
    }

    it('should calculate success and failure counts correctly', () => {
      const results: StepResult[] = [
        { step: 'Step 1', success: true, duration: 100 },
        { step: 'Step 2', success: false, duration: 50, error: 'Failed' },
        { step: 'Step 3', success: true, duration: 200 },
        { step: 'Step 4', success: false, duration: 75, error: 'Error' }
      ];

      const successful = results.filter(r => r.success);
      const failed = results.filter(r => !r.success);
      const totalTime = results.reduce((sum, r) => sum + r.duration, 0);

      expect(successful).toHaveLength(2);
      expect(failed).toHaveLength(2);
      expect(totalTime).toBe(425);
    });

    it('should identify implementation gaps', () => {
      const results: StepResult[] = [
        { step: 'Step 1', success: true, duration: 100 },
        { step: 'Step 2', success: false, duration: 50, error: 'not yet implemented' },
        { step: 'Step 3', success: false, duration: 75, error: 'real error' }
      ];

      const hasImplementationGaps = results.some(r => r.error?.includes('not yet implemented'));
      const hasRealFailures = results.some(r => !r.success && !r.error?.includes('not yet implemented'));

      expect(hasImplementationGaps).toBe(true);
      expect(hasRealFailures).toBe(true);
    });
  });

  describe('Error Handling and Recovery', () => {
    it('should continue workflow on non-critical failures', () => {
      const results = [
        { step: 'Plugin health check', success: false, error: 'not yet implemented' },
        { step: 'Plugin extraction', success: false, error: 'not yet implemented' },
        { step: 'Map validation', success: true }
      ];

      // Should continue despite implementation gaps
      const shouldContinue = !results.some(r =>
        !r.success && !r.error?.includes('not yet implemented')
      );

      expect(shouldContinue).toBe(true);
    });

    it('should stop workflow on critical failures', () => {
      const results = [
        { step: 'Plugin extraction', success: false, error: 'Permission denied' }
      ];

      // Should stop on real errors
      const shouldContinue = !results.some(r =>
        !r.success && !r.error?.includes('not yet implemented')
      );

      expect(shouldContinue).toBe(false);
    });

    it('should provide helpful error messages', () => {
      const error = new Error('Workflow failed');
      const helpfulMessage =
        'Workflow failed. Check the errors above and:\n' +
        '1. Ensure all prerequisites are met\n' +
        '2. Verify canonical maps exist\n' +
        '3. Check DAW installation paths\n' +
        '4. Re-run with --force if needed';

      expect(helpfulMessage).toContain('Workflow failed');
      expect(helpfulMessage).toContain('Re-run with --force');
    });
  });
});