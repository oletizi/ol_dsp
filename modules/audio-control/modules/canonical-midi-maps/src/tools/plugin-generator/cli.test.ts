/**
 * Test suite for CLI functionality
 */

import { describe, expect, it, vi, beforeEach, afterEach } from 'vitest';
import { Cli, createCli } from '@/tools/plugin-generator/cli.js';

describe('Cli', () => {
  let cli: Cli;
  let consoleSpy: any;

  beforeEach(() => {
    cli = new Cli();
    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});
  });

  afterEach(() => {
    consoleSpy.mockRestore();
  });

  describe('parseArgs', () => {
    it('should parse help flags', () => {
      expect(cli.parseArgs(['--help'])).toEqual({
        format: undefined,
        quick: false,
        help: true,
      });

      expect(cli.parseArgs(['-h'])).toEqual({
        format: undefined,
        quick: false,
        help: true,
      });
    });

    it('should parse quick flag', () => {
      expect(cli.parseArgs(['--quick'])).toEqual({
        format: undefined,
        quick: true,
        help: false,
      });
    });

    it('should parse format option', () => {
      expect(cli.parseArgs(['--format', 'VST3'])).toEqual({
        format: 'VST3',
        quick: false,
        help: false,
      });

      expect(cli.parseArgs(['--format', 'AudioUnit'])).toEqual({
        format: 'AudioUnit',
        quick: false,
        help: false,
      });
    });

    it('should parse multiple arguments', () => {
      expect(cli.parseArgs(['--quick', '--format', 'VST3', '--help'])).toEqual({
        format: 'VST3',
        quick: true,
        help: true,
      });
    });

    it('should handle empty arguments', () => {
      expect(cli.parseArgs([])).toEqual({
        format: undefined,
        quick: false,
        help: false,
      });
    });

    it('should use process.argv by default', () => {
      // Mock process.argv
      const originalArgv = process.argv;
      process.argv = ['node', 'script.js', '--quick', '--format', 'VST3'];

      try {
        const result = cli.parseArgs();
        expect(result).toEqual({
          format: 'VST3',
          quick: true,
          help: false,
        });
      } finally {
        process.argv = originalArgv;
      }
    });

    it('should ignore unknown arguments', () => {
      expect(cli.parseArgs(['--unknown', '--format', 'VST3', '--weird-flag'])).toEqual({
        format: 'VST3',
        quick: false,
        help: false,
      });
    });

    it('should handle format without value', () => {
      // When --format is the last argument without a value
      expect(cli.parseArgs(['--format'])).toEqual({
        format: undefined,
        quick: false,
        help: false,
      });

      // When --format is followed by another flag
      expect(cli.parseArgs(['--format', '--quick'])).toEqual({
        format: '--quick', // This is technically parsed as the format value
        quick: false,
        help: false,
      });
    });

    it('should handle multiple format flags (last one wins)', () => {
      expect(cli.parseArgs(['--format', 'VST', '--format', 'VST3'])).toEqual({
        format: 'VST3',
        quick: false,
        help: false,
      });
    });

    it('should handle repeated flags', () => {
      expect(cli.parseArgs(['--quick', '--quick', '--help', '--help'])).toEqual({
        format: undefined,
        quick: true,
        help: true,
      });
    });

    it('should handle mixed order of arguments', () => {
      expect(cli.parseArgs(['--format', 'AudioUnit', '--help', '--quick'])).toEqual({
        format: 'AudioUnit',
        quick: true,
        help: true,
      });

      expect(cli.parseArgs(['--quick', '--help', '--format', 'VST'])).toEqual({
        format: 'VST',
        quick: true,
        help: true,
      });
    });
  });

  describe('showHelp', () => {
    it('should display help message with output directory', () => {
      const outputDir = '/test/output/directory';
      cli.showHelp(outputDir);

      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Plugin Spec Generator for Canonical MIDI Maps')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Usage:')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Options:')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining('Examples:')
      );
      expect(consoleSpy).toHaveBeenCalledWith(
        expect.stringContaining(`Plugin descriptors are saved to: ${outputDir}`)
      );
    });

    it('should include all expected help sections', () => {
      cli.showHelp('/output');

      const helpText = consoleSpy.mock.calls[0][0];

      expect(helpText).toContain('--help, -h');
      expect(helpText).toContain('--format <format>');
      expect(helpText).toContain('--quick');
      expect(helpText).toContain('AudioUnit, VST3, VST');
      expect(helpText).toContain('Examples:');
      expect(helpText).toContain('generate-plugin-specs.ts');
      expect(helpText).toContain('File naming:');
    });

    it('should handle different output directories', () => {
      const testDirectories = [
        '/var/output',
        '/home/user/plugins',
        'C:\\Windows\\Plugins',
        './relative/path',
      ];

      testDirectories.forEach(dir => {
        consoleSpy.mockClear();
        cli.showHelp(dir);
        expect(consoleSpy).toHaveBeenCalledWith(
          expect.stringContaining(`Plugin descriptors are saved to: ${dir}`)
        );
      });
    });
  });

  describe('factory function', () => {
    it('should create a functional CLI instance', () => {
      const cli = createCli();

      expect(cli).toBeDefined();
      expect(cli).toBeInstanceOf(Cli);

      // Test that it actually works
      const result = cli.parseArgs(['--quick', '--format', 'VST3']);
      expect(result).toEqual({
        format: 'VST3',
        quick: true,
        help: false,
      });
    });
  });

  describe('edge cases and error scenarios', () => {
    it('should handle undefined arguments gracefully', () => {
      // This shouldn't happen in normal usage, but let's be defensive
      expect(cli.parseArgs(undefined as any)).toEqual({
        format: undefined,
        quick: false,
        help: false,
      });
    });

    it('should handle null arguments gracefully', () => {
      // This shouldn't happen in normal usage, but let's be defensive
      expect(cli.parseArgs(null as any)).toEqual({
        format: undefined,
        quick: false,
        help: false,
      });
    });

    it('should handle arguments with special characters', () => {
      expect(cli.parseArgs(['--format', 'VST-3.0'])).toEqual({
        format: 'VST-3.0',
        quick: false,
        help: false,
      });

      expect(cli.parseArgs(['--format', 'Audio Unit'])).toEqual({
        format: 'Audio Unit',
        quick: false,
        help: false,
      });
    });

    it('should handle empty string arguments', () => {
      expect(cli.parseArgs(['--format', ''])).toEqual({
        format: '',
        quick: false,
        help: false,
      });
    });

    it('should handle numeric format values', () => {
      expect(cli.parseArgs(['--format', '123'])).toEqual({
        format: '123',
        quick: false,
        help: false,
      });
    });
  });

  describe('real-world argument combinations', () => {
    it('should handle typical usage patterns', () => {
      // Quick VST3 generation
      expect(cli.parseArgs(['--quick', '--format', 'VST3'])).toEqual({
        format: 'VST3',
        quick: true,
        help: false,
      });

      // Help with other flags (help should still be true)
      expect(cli.parseArgs(['--help', '--format', 'AU', '--quick'])).toEqual({
        format: 'AU',
        quick: true,
        help: true,
      });

      // AudioUnit only
      expect(cli.parseArgs(['--format', 'AudioUnit'])).toEqual({
        format: 'AudioUnit',
        quick: false,
        help: false,
      });
    });

    it('should handle case sensitivity', () => {
      // CLI should preserve case of format values
      expect(cli.parseArgs(['--format', 'vst3'])).toEqual({
        format: 'vst3',
        quick: false,
        help: false,
      });

      expect(cli.parseArgs(['--format', 'AUDIOUNIT'])).toEqual({
        format: 'AUDIOUNIT',
        quick: false,
        help: false,
      });
    });
  });
});