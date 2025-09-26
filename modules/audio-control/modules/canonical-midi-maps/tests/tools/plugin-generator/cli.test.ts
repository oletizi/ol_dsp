/**
 * Tests for CLI argument parsing and help display
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { Cli, createCli } from '@/tools/plugin-generator/cli.js';
import type { ICli } from '@/tools/plugin-generator/cli.js';

describe('Cli', () => {
  let cli: ICli;
  let consoleSpy: any;

  beforeEach(() => {
    cli = createCli();
    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
  });

  afterEach(() => {
    consoleSpy.mockRestore();
  });

  describe('parseArgs', () => {
    it('should parse empty arguments to default values', () => {
      const result = cli.parseArgs([]);

      expect(result).toEqual({
        format: undefined,
        quick: false,
        help: false,
      });
    });

    it('should parse help flag variants', () => {
      const helpResult = cli.parseArgs(['--help']);
      expect(helpResult.help).toBe(true);

      const hResult = cli.parseArgs(['-h']);
      expect(hResult.help).toBe(true);
    });

    it('should parse quick flag', () => {
      const result = cli.parseArgs(['--quick']);

      expect(result.quick).toBe(true);
      expect(result.help).toBe(false);
      expect(result.format).toBeUndefined();
    });

    it('should parse format flag with value', () => {
      const result = cli.parseArgs(['--format', 'AudioUnit']);

      expect(result.format).toBe('AudioUnit');
      expect(result.quick).toBe(false);
      expect(result.help).toBe(false);
    });

    it('should parse multiple flags', () => {
      const result = cli.parseArgs(['--quick', '--format', 'VST3']);

      expect(result.quick).toBe(true);
      expect(result.format).toBe('VST3');
      expect(result.help).toBe(false);
    });

    it('should handle format flag without value gracefully', () => {
      const result = cli.parseArgs(['--format']);

      expect(result.format).toBeUndefined();
      expect(result.quick).toBe(false);
      expect(result.help).toBe(false);
    });

    it('should ignore unknown flags', () => {
      const result = cli.parseArgs(['--unknown', '--quick']);

      expect(result.quick).toBe(true);
      expect(result.help).toBe(false);
    });

    it('should use process.argv when no args provided', () => {
      // Test the default behavior when called without arguments
      const originalArgv = process.argv;
      process.argv = ['node', 'script', '--quick'];

      const result = cli.parseArgs();

      expect(result.quick).toBe(true);

      process.argv = originalArgv;
    });
  });

  describe('showHelp', () => {
    it('should display help message with output directory', () => {
      const outputDir = '/test/output';

      cli.showHelp(outputDir);

      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Plugin Spec Generator for Canonical MIDI Maps')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining(outputDir)
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('--help, -h')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('--format')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('--quick')
      );
    });

    it('should include usage examples', () => {
      cli.showHelp('/test/output');

      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Examples:')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('generate-plugin-specs.ts --format AudioUnit')
      );
    });

    it('should show output format specification', () => {
      cli.showHelp('/test/output');

      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('File naming: {manufacturer}_{plugin_name}.json')
      );
    });
  });

  describe('createCli factory', () => {
    it('should create a valid CLI instance', () => {
      const cliInstance = createCli();

      expect(cliInstance).toBeDefined();
      expect(typeof cliInstance.parseArgs).toBe('function');
      expect(typeof cliInstance.showHelp).toBe('function');
    });

    it('should create independent instances', () => {
      const cli1 = createCli();
      const cli2 = createCli();

      expect(cli1).not.toBe(cli2);
      expect(cli1).toBeInstanceOf(Cli);
      expect(cli2).toBeInstanceOf(Cli);
    });
  });

  describe('edge cases', () => {
    it('should handle format with empty string value', () => {
      const result = cli.parseArgs(['--format', '']);

      expect(result.format).toBe('');
    });

    it('should handle duplicate flags (last one wins)', () => {
      const result = cli.parseArgs(['--format', 'AudioUnit', '--format', 'VST3']);

      expect(result.format).toBe('VST3');
    });

    it('should handle mixed order flags', () => {
      const result = cli.parseArgs(['--format', 'VST3', '--help', '--quick']);

      expect(result.format).toBe('VST3');
      expect(result.help).toBe(true);
      expect(result.quick).toBe(true);
    });
  });
});