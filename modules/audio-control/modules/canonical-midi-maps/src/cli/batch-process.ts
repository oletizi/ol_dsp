#!/usr/bin/env node

/**
 * CLI tool for batch processing of MIDI map files.
 * Supports various operations on multiple files with parallel processing.
 *
 * Features:
 * - Batch validation with parallel processing
 * - Batch conversion between formats
 * - Batch optimization and formatting
 * - Progress reporting for long operations
 * - Summary statistics and reporting
 * - Error recovery and partial success handling
 *
 * @example
 * ```bash
 * # Batch validate all maps
 * batch-process validate maps/**/*.yaml
 *
 * # Batch convert with optimization
 * batch-process convert --to json --optimize maps/
 *
 * # Batch process with progress
 * batch-process --parallel 4 validate large-collection/
 * ```
 */

import { readFileSync, writeFileSync, statSync, readdirSync } from 'fs';
import { join, extname, basename, dirname } from 'path';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';
import type { CanonicalMidiMapOutput } from '@/validators/schema.js';

type BatchOperation = 'validate' | 'convert' | 'optimize' | 'analyze';
type OutputFormat = 'yaml' | 'json';

interface BatchOptions {
  operation: BatchOperation;
  parallel: number;
  output?: string;
  format?: OutputFormat;
  optimize: boolean;
  strict: boolean;
  verbose: boolean;
  quiet: boolean;
  help: boolean;
  dryRun: boolean;
}

interface BatchStats {
  total: number;
  processed: number;
  successful: number;
  failed: number;
  warnings: number;
  startTime: number;
  endTime?: number;
}

interface FileTask {
  filePath: string;
  operation: BatchOperation;
  options: BatchOptions;
}

interface TaskResult {
  filePath: string;
  success: boolean;
  message?: string;
  warnings?: string[];
  metadata?: Record<string, any>;
}

class BatchProcessor {
  private stats: BatchStats = {
    total: 0,
    processed: 0,
    successful: 0,
    failed: 0,
    warnings: 0,
    startTime: Date.now(),
  };

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): { paths: string[]; options: BatchOptions } {
    const options: BatchOptions = {
      operation: 'validate',
      parallel: 2,
      optimize: false,
      strict: false,
      verbose: false,
      quiet: false,
      help: false,
      dryRun: false,
    };
    const paths: string[] = [];

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];

      if (arg === '--help' || arg === '-h') {
        options.help = true;
      } else if (['validate', 'convert', 'optimize', 'analyze'].includes(arg)) {
        options.operation = arg as BatchOperation;
      } else if (arg === '--parallel' || arg === '-p') {
        if (i + 1 < args.length) {
          const parallel = parseInt(args[i + 1], 10);
          if (isNaN(parallel) || parallel < 1) {
            console.error('--parallel must be a positive integer');
            process.exit(1);
          }
          options.parallel = parallel;
          i++;
        } else {
          console.error('--parallel requires a number argument');
          process.exit(1);
        }
      } else if (arg === '--output' || arg === '-o') {
        if (i + 1 < args.length) {
          options.output = args[i + 1];
          i++;
        } else {
          console.error('--output requires a path argument');
          process.exit(1);
        }
      } else if (arg === '--format' || arg === '-f') {
        if (i + 1 < args.length) {
          const format = args[i + 1].toLowerCase() as OutputFormat;
          if (!['yaml', 'json'].includes(format)) {
            console.error(`Invalid format: ${args[i + 1]}`);
            process.exit(1);
          }
          options.format = format;
          i++;
        } else {
          console.error('--format requires yaml or json');
          process.exit(1);
        }
      } else if (arg === '--optimize') {
        options.optimize = true;
      } else if (arg === '--strict' || arg === '-s') {
        options.strict = true;
      } else if (arg === '--verbose' || arg === '-v') {
        options.verbose = true;
      } else if (arg === '--quiet' || arg === '-q') {
        options.quiet = true;
      } else if (arg === '--dry-run') {
        options.dryRun = true;
      } else if (!arg.startsWith('-')) {
        paths.push(arg);
      } else {
        console.error(`Unknown option: ${arg}`);
        process.exit(1);
      }
    }

    return { paths, options };
  }

  /**
   * Display help information
   */
  showHelp(): void {
    console.log(`Canonical MIDI Map Batch Processor

Usage:
  batch-process <operation> [options] <file|directory>...

Operations:
  validate        Validate MIDI map files
  convert         Convert between formats
  optimize        Optimize and reformat files
  analyze         Analyze maps and generate statistics

Arguments:
  <file|directory>     One or more files or directories to process

Options:
  -h, --help          Show this help message
  -p, --parallel <n>  Number of parallel workers [default: 2]
  -o, --output <dir>  Output directory for processed files
  -f, --format <fmt>  Output format (yaml, json) [default: yaml]
  --optimize          Apply optimization during processing
  -s, --strict        Treat warnings as errors
  -v, --verbose       Show detailed processing information
  -q, --quiet         Only show summary and errors
  --dry-run           Show what would be processed without making changes

Examples:
  batch-process validate maps/                         # Validate all maps
  batch-process convert --format json --output dist/ maps/  # Convert to JSON
  batch-process optimize --parallel 4 large-collection/     # Optimize in parallel
  batch-process analyze --verbose maps/**/*.yaml            # Analyze with details

Exit Codes:
  0    All operations successful
  1    Some operations failed
  2    Invalid command line arguments`);
  }

  /**
   * Collect all files to process
   */
  private collectFiles(paths: string[]): string[] {
    const files: string[] = [];

    for (const path of paths) {
      try {
        const stat = statSync(path);

        if (stat.isDirectory()) {
          const entries = readdirSync(path);
          for (const entry of entries) {
            const filePath = join(path, entry);
            const ext = extname(entry).toLowerCase();
            if (['.yaml', '.yml', '.json'].includes(ext)) {
              files.push(filePath);
            }
          }
        } else if (stat.isFile()) {
          const ext = extname(path).toLowerCase();
          if (['.yaml', '.yml', '.json'].includes(ext)) {
            files.push(path);
          }
        }
      } catch (error) {
        console.warn(`Warning: Cannot access ${path}: ${error instanceof Error ? error.message : 'Unknown error'}`);
      }
    }

    return files;
  }

  /**
   * Process a single file task
   */
  private async processFileTask(task: FileTask): Promise<TaskResult> {
    try {
      const content = readFileSync(task.filePath, 'utf8');
      const ext = extname(task.filePath).toLowerCase();

      // Parse the file
      let result;
      if (ext === '.yaml' || ext === '.yml') {
        result = CanonicalMapParser.parseFromYAML(content);
      } else if (ext === '.json') {
        result = CanonicalMapParser.parseFromJSON(content);
      } else {
        return {
          filePath: task.filePath,
          success: false,
          message: 'Unsupported file format',
        };
      }

      const hasErrors = !result.validation.valid;
      const hasWarnings = result.validation.warnings.length > 0;
      const shouldFail = hasErrors || (task.options.strict && hasWarnings);

      if (shouldFail) {
        return {
          filePath: task.filePath,
          success: false,
          message: hasErrors ? 'Validation failed' : 'Warnings in strict mode',
          warnings: result.validation.warnings.map(w => `${w.path}: ${w.message}`),
        };
      }

      const warnings = result.validation.warnings.map(w => `${w.path}: ${w.message}`);

      switch (task.operation) {
        case 'validate':
          return {
            filePath: task.filePath,
            success: true,
            message: hasWarnings ? `Valid with ${warnings.length} warnings` : 'Valid',
            warnings,
            metadata: result.map ? {
              device: `${result.map.device.manufacturer} ${result.map.device.model}`,
              controls: result.map.controls.length,
              plugin: result.map.plugin ? `${result.map.plugin.manufacturer} ${result.map.plugin.name}` : undefined,
            } : undefined,
          };

        case 'convert':
        case 'optimize':
          if (!result.map) {
            return {
              filePath: task.filePath,
              success: false,
              message: 'Failed to parse map data',
            };
          }

          if (task.options.dryRun) {
            return {
              filePath: task.filePath,
              success: true,
              message: 'Would be processed (dry run)',
              warnings,
            };
          }

          // Perform conversion/optimization
          const outputFormat = task.options.format || 'yaml';
          let outputContent: string;
          let outputExt: string;

          if (outputFormat === 'yaml') {
            outputContent = CanonicalMapParser.serializeToYAML(result.map);
            outputExt = '.yaml';
          } else {
            outputContent = CanonicalMapParser.serializeToJSON(result.map, true);
            outputExt = '.json';
          }

          // Determine output path
          const baseName = basename(task.filePath, extname(task.filePath));
          const outputPath = task.options.output ?
            join(task.options.output, `${baseName}${outputExt}`) :
            join(dirname(task.filePath), `${baseName}${outputExt}`);

          // Write the file
          writeFileSync(outputPath, outputContent, 'utf8');

          return {
            filePath: task.filePath,
            success: true,
            message: `${task.operation === 'convert' ? 'Converted' : 'Optimized'} to ${outputPath}`,
            warnings,
          };

        case 'analyze':
          return {
            filePath: task.filePath,
            success: true,
            message: 'Analyzed',
            warnings,
            metadata: result.map ? {
              version: result.map.version,
              device: `${result.map.device.manufacturer} ${result.map.device.model}`,
              controls: result.map.controls.length,
              controlTypes: this.analyzeControlTypes(result.map),
              plugin: result.map.plugin ? {
                manufacturer: result.map.plugin.manufacturer,
                name: result.map.plugin.name,
                format: result.map.plugin.format,
              } : undefined,
              midiChannels: this.analyzeMidiChannels(result.map),
              ccUsage: this.analyzeCCUsage(result.map),
            } : undefined,
          };

        default:
          return {
            filePath: task.filePath,
            success: false,
            message: `Unknown operation: ${task.operation}`,
          };
      }
    } catch (error) {
      return {
        filePath: task.filePath,
        success: false,
        message: error instanceof Error ? error.message : 'Unknown error',
      };
    }
  }

  /**
   * Analyze control types in a map
   */
  private analyzeControlTypes(map: CanonicalMidiMapOutput): Record<string, number> {
    const types: Record<string, number> = {};
    for (const control of map.controls) {
      types[control.type] = (types[control.type] || 0) + 1;
    }
    return types;
  }

  /**
   * Analyze MIDI channels used in a map
   */
  private analyzeMidiChannels(map: CanonicalMidiMapOutput): number[] {
    const channels = new Set<number>();
    if (map.midi_channel) {
      channels.add(map.midi_channel);
    }
    for (const control of map.controls) {
      if (typeof control.channel === 'number') {
        channels.add(control.channel);
      }
    }
    return Array.from(channels).sort();
  }

  /**
   * Analyze CC usage in a map
   */
  private analyzeCCUsage(map: CanonicalMidiMapOutput): { used: number[]; duplicates: number[] } {
    const ccCounts: Record<number, number> = {};

    for (const control of map.controls) {
      if (control.cc !== undefined) {
        ccCounts[control.cc] = (ccCounts[control.cc] || 0) + 1;
      }
      if (control.buttons) {
        for (const button of control.buttons) {
          ccCounts[button.cc] = (ccCounts[button.cc] || 0) + 1;
        }
      }
    }

    const used = Object.keys(ccCounts).map(Number).sort((a, b) => a - b);
    const duplicates = used.filter(cc => ccCounts[cc] > 1);

    return { used, duplicates };
  }

  /**
   * Process files in parallel batches
   */
  private async processInBatches(tasks: FileTask[], options: BatchOptions): Promise<TaskResult[]> {
    const results: TaskResult[] = [];
    const batchSize = options.parallel;

    for (let i = 0; i < tasks.length; i += batchSize) {
      const batch = tasks.slice(i, i + batchSize);
      const batchResults = await Promise.all(
        batch.map(task => this.processFileTask(task))
      );

      results.push(...batchResults);

      // Update progress
      this.stats.processed += batch.length;

      if (!options.quiet) {
        const progress = Math.round((this.stats.processed / this.stats.total) * 100);
        console.log(`📊 Progress: ${this.stats.processed}/${this.stats.total} (${progress}%)`);
      }
    }

    return results;
  }

  /**
   * Main batch processing function
   */
  async process(paths: string[], options: BatchOptions): Promise<boolean> {
    if (paths.length === 0) {
      console.error('Error: No files or directories specified');
      this.showHelp();
      return false;
    }

    // Collect all files to process
    const files = this.collectFiles(paths);
    if (files.length === 0) {
      console.error('Error: No MIDI map files found to process');
      return false;
    }

    this.stats.total = files.length;

    if (!options.quiet) {
      console.log(`🚀 Starting batch ${options.operation} of ${files.length} files...`);
      if (options.dryRun) {
        console.log('🔍 Dry run mode - no files will be modified');
      }
    }

    // Create tasks
    const tasks: FileTask[] = files.map(filePath => ({
      filePath,
      operation: options.operation,
      options,
    }));

    // Process files
    const results = await this.processInBatches(tasks, options);

    // Update final stats
    this.stats.endTime = Date.now();
    this.stats.successful = results.filter(r => r.success).length;
    this.stats.failed = results.filter(r => !r.success).length;
    this.stats.warnings = results.reduce((sum, r) => sum + (r.warnings?.length || 0), 0);

    // Report results
    if (!options.quiet) {
      console.log('\n📊 Batch Processing Summary:');
      console.log(`  Total files: ${this.stats.total}`);
      console.log(`  ✅ Successful: ${this.stats.successful}`);
      console.log(`  ❌ Failed: ${this.stats.failed}`);
      if (this.stats.warnings > 0) {
        console.log(`  ⚠️  Warnings: ${this.stats.warnings}`);
      }

      const duration = ((this.stats.endTime || Date.now()) - this.stats.startTime) / 1000;
      console.log(`  ⏱️  Duration: ${duration.toFixed(2)}s`);
    }

    // Show detailed results in verbose mode
    if (options.verbose) {
      console.log('\n📋 Detailed Results:');
      for (const result of results) {
        const status = result.success ? '✅' : '❌';
        console.log(`  ${status} ${basename(result.filePath)}: ${result.message || 'No message'}`);

        if (result.warnings && result.warnings.length > 0) {
          for (const warning of result.warnings) {
            console.log(`    ⚠️  ${warning}`);
          }
        }

        if (result.metadata && options.operation === 'analyze') {
          console.log(`    📊 ${JSON.stringify(result.metadata, null, 6).replace(/\n/g, '\n    ')}`);
        }
      }
    }

    // Show failures in non-quiet mode
    if (!options.quiet) {
      const failures = results.filter(r => !r.success);
      if (failures.length > 0) {
        console.log('\n❌ Failed Files:');
        for (const failure of failures) {
          console.log(`  • ${failure.filePath}: ${failure.message}`);
        }
      }
    }

    return this.stats.failed === 0;
  }
}

/**
 * Main CLI entry point
 */
async function main(): Promise<void> {
  const processor = new BatchProcessor();
  const { paths, options } = processor.parseArgs();

  if (options.help) {
    processor.showHelp();
    process.exit(0);
  }

  try {
    const success = await processor.process(paths, options);
    process.exit(success ? 0 : 1);
  } catch (error) {
    const message = error instanceof Error ? error.message : 'Unknown error';
    console.error(`Fatal error: ${message}`);
    process.exit(2);
  }
}

// Run CLI if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  await main();
}

export { BatchProcessor, main };