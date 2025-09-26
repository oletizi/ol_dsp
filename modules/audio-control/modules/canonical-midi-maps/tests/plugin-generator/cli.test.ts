/**
 * Tests for CLI argument parsing and help display
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { createCli, Cli } from '@/tools/plugin-generator/cli.js';
import type { GeneratorArgs } from '@/tools/plugin-generator/types.js';

describe('CLI', () => {
  let cli: Cli;
  let consoleLogSpy: any;

  beforeEach(() => {
    cli = createCli() as Cli;
    consoleLogSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
  });

  describe('parseArgs', () => {
    it('should return default args when no arguments provided', () => {
      const args = cli.parseArgs([]);

      expect(args).toEqual({
        format: undefined,
        quick: false,
        help: false
      });
    });

    it('should parse help flag (--help)', () => {
      const args = cli.parseArgs(['--help']);

      expect(args.help).toBe(true);
      expect(args.quick).toBe(false);
      expect(args.format).toBeUndefined();
    });

    it('should parse help flag (-h)', () => {
      const args = cli.parseArgs(['-h']);

      expect(args.help).toBe(true);
    });

    it('should parse quick flag', () => {
      const args = cli.parseArgs(['--quick']);

      expect(args.quick).toBe(true);
      expect(args.help).toBe(false);
      expect(args.format).toBeUndefined();
    });

    it('should parse format flag with value', () => {
      const args = cli.parseArgs(['--format', 'AudioUnit']);

      expect(args.format).toBe('AudioUnit');
      expect(args.help).toBe(false);
      expect(args.quick).toBe(false);
    });

    it('should parse multiple flags', () => {
      const args = cli.parseArgs(['--quick', '--format', 'VST3']);

      expect(args.quick).toBe(true);
      expect(args.format).toBe('VST3');
      expect(args.help).toBe(false);
    });

    it('should handle format flag at end without value', () => {
      const args = cli.parseArgs(['--quick', '--format']);

      expect(args.quick).toBe(true);
      expect(args.format).toBeUndefined();
    });

    it('should ignore unknown flags', () => {
      const args = cli.parseArgs(['--unknown', '--quick', '--other']);

      expect(args.quick).toBe(true);
      expect(args.help).toBe(false);
      expect(args.format).toBeUndefined();
    });
  });

  describe('showHelp', () => {
    it('should display help message with output directory', () => {
      const outputDir = '/test/output';

      cli.showHelp(outputDir);

      expect(consoleLogSpy).toHaveBeenCalledWith(
        expect.stringContaining('Plugin Spec Generator for Canonical MIDI Maps')
      );
      expect(consoleLogSpy).toHaveBeenCalledWith(
        expect.stringContaining(outputDir)
      );
    });

    it('should display usage examples', () => {
      cli.showHelp('/test');

      expect(consoleLogSpy).toHaveBeenCalledWith(
        expect.stringContaining('Usage:')
      );
      expect(consoleLogSpy).toHaveBeenCalledWith(
        expect.stringContaining('Examples:')
      );
    });

    it('should display all available options', () => {
      cli.showHelp('/test');

      const helpText = consoleLogSpy.mock.calls[0][0] as string;
      expect(helpText).toContain('--help');
      expect(helpText).toContain('--format');
      expect(helpText).toContain('--quick');
      expect(helpText).toContain('AudioUnit');
      expect(helpText).toContain('VST3');
    });
  });

  describe('createCli factory', () => {
    it('should create CLI instance', () => {
      const cliInstance = createCli();

      expect(cliInstance).toBeInstanceOf(Cli);
      expect(typeof cliInstance.parseArgs).toBe('function');
      expect(typeof cliInstance.showHelp).toBe('function');
    });

    it('should create separate instances', () => {
      const cli1 = createCli();
      const cli2 = createCli();

      expect(cli1).not.toBe(cli2);
      expect(cli1).toBeInstanceOf(Cli);
      expect(cli2).toBeInstanceOf(Cli);
    });
  });
});