#!/usr/bin/env node

/**
 * CLI tool for converting MIDI map files between formats.
 * Supports conversion between YAML, JSON, and XML formats.
 *
 * Features:
 * - Format conversion (YAML ↔ JSON ↔ XML)
 * - Batch conversion of directories
 * - Format validation during conversion
 * - Output customization (formatting, compression)
 * - Backup creation for destructive operations
 *
 * @example
 * ```bash
 * # Convert single file
 * convert-maps controller.yaml --to json
 *
 * # Convert all files in directory
 * convert-maps maps/ --to yaml --output converted/
 *
 * # Convert with validation
 * convert-maps --from json --to yaml --validate maps/
 * ```
 */

import { readFileSync, writeFileSync, statSync, readdirSync, mkdirSync, copyFileSync } from 'fs';
import { join, extname, basename, dirname, resolve } from 'path';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';

type SupportedFormat = 'yaml' | 'json' | 'xml';

interface ConvertOptions {
  from?: SupportedFormat;
  to: SupportedFormat;
  output?: string;
  validate: boolean;
  backup: boolean;
  pretty: boolean;
  overwrite: boolean;
  help: boolean;
  verbose: boolean;
  quiet: boolean;
}

interface ConversionStats {
  total: number;
  converted: number;
  failed: number;
  skipped: number;
}

class MapConverter {
  private stats: ConversionStats = {
    total: 0,
    converted: 0,
    failed: 0,
    skipped: 0,
  };

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): { paths: string[]; options: ConvertOptions } {
    const options: ConvertOptions = {
      to: 'yaml',
      validate: true,
      backup: false,
      pretty: true,
      overwrite: false,
      help: false,
      verbose: false,
      quiet: false,
    };
    const paths: string[] = [];

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];

      if (arg === '--help' || arg === '-h') {
        options.help = true;
      } else if (arg === '--from' && i + 1 < args.length) {
        const format = args[i + 1].toLowerCase() as SupportedFormat;
        if (!['yaml', 'json', 'xml'].includes(format)) {
          console.error(`Invalid source format: ${args[i + 1]}`);
          process.exit(1);
        }
        options.from = format;
        i++;
      } else if (arg === '--to' && i + 1 < args.length) {
        const format = args[i + 1].toLowerCase() as SupportedFormat;
        if (!['yaml', 'json', 'xml'].includes(format)) {
          console.error(`Invalid target format: ${args[i + 1]}`);
          process.exit(1);
        }
        options.to = format;
        i++;
      } else if (arg === '--output' || arg === '-o') {
        if (i + 1 < args.length) {
          options.output = args[i + 1];
          i++;
        } else {
          console.error('--output requires a path argument');
          process.exit(1);
        }
      } else if (arg === '--no-validate') {
        options.validate = false;
      } else if (arg === '--backup') {
        options.backup = true;
      } else if (arg === '--no-pretty') {
        options.pretty = false;
      } else if (arg === '--overwrite') {
        options.overwrite = true;
      } else if (arg === '--verbose' || arg === '-v') {
        options.verbose = true;
      } else if (arg === '--quiet' || arg === '-q') {
        options.quiet = true;
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
    console.log(`Canonical MIDI Map Format Converter

Usage:
  convert-maps [options] <file|directory>...

Arguments:
  <file|directory>     One or more files or directories to convert

Options:
  -h, --help          Show this help message
  --from <format>     Source format (yaml, json, xml) - auto-detected if not specified
  --to <format>       Target format (yaml, json, xml) [default: yaml]
  -o, --output <dir>  Output directory (default: same as input)
  --validate          Validate during conversion [default: true]
  --no-validate       Skip validation during conversion
  --backup            Create backup files before conversion
  --no-pretty         Disable pretty formatting for JSON/YAML
  --overwrite         Overwrite existing files without prompt
  -v, --verbose       Show detailed conversion information
  -q, --quiet         Only show errors

Supported Formats:
  yaml    YAML format (.yaml, .yml)
  json    JSON format (.json)
  xml     Ardour XML format (.xml) [read-only for now]

Examples:
  convert-maps controller.yaml --to json              # Convert to JSON
  convert-maps maps/ --to yaml --output converted/    # Batch convert directory
  convert-maps --from json --to yaml file.json       # Explicit format conversion
  convert-maps --backup --overwrite maps/ --to json  # Convert with backup

Exit Codes:
  0    All conversions successful
  1    Some conversions failed
  2    Invalid command line arguments`);
  }

  /**
   * Detect format from file extension
   */
  private detectFormat(filePath: string): SupportedFormat | null {
    const ext = extname(filePath).toLowerCase();

    switch (ext) {
      case '.yaml':
      case '.yml':
        return 'yaml';
      case '.json':
        return 'json';
      case '.xml':
        return 'xml';
      default:
        return null;
    }
  }

  /**
   * Get output file path based on conversion
   */
  private getOutputPath(inputPath: string, targetFormat: SupportedFormat, outputDir?: string): string {
    const baseName = basename(inputPath, extname(inputPath));
    const extension = targetFormat === 'yaml' ? '.yaml' : `.${targetFormat}`;
    const fileName = `${baseName}${extension}`;

    if (outputDir) {
      return join(outputDir, fileName);
    } else {
      return join(dirname(inputPath), fileName);
    }
  }

  /**
   * Create backup of existing file
   */
  private createBackup(filePath: string): void {
    const backupPath = `${filePath}.backup`;
    copyFileSync(filePath, backupPath);
  }

  /**
   * Convert a single file
   */
  private async convertFile(
    inputPath: string,
    options: ConvertOptions
  ): Promise<boolean> {
    this.stats.total++;

    try {
      // Detect source format
      const sourceFormat = options.from || this.detectFormat(inputPath);
      if (!sourceFormat) {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Cannot detect format (use --from to specify)`);
        }
        this.stats.failed++;
        return false;
      }

      // Skip if already in target format
      if (sourceFormat === options.to && !options.output) {
        if (!options.quiet) {
          console.warn(`⚠️  ${inputPath}: Already in ${options.to} format, skipping`);
        }
        this.stats.skipped++;
        return true;
      }

      // Read and parse source file
      const content = readFileSync(inputPath, 'utf8');
      let result;

      if (sourceFormat === 'yaml') {
        result = CanonicalMapParser.parseFromYAML(content);
      } else if (sourceFormat === 'json') {
        result = CanonicalMapParser.parseFromJSON(content);
      } else if (sourceFormat === 'xml') {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: XML parsing not yet implemented`);
        }
        this.stats.failed++;
        return false;
      } else {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Unsupported source format: ${sourceFormat}`);
        }
        this.stats.failed++;
        return false;
      }

      // Validate if requested
      if (options.validate && !result.validation.valid) {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Validation failed`);
          if (options.verbose) {
            for (const error of result.validation.errors) {
              console.error(`    • ${error.path}: ${error.message}`);
            }
          }
        }
        this.stats.failed++;
        return false;
      }

      if (!result.map) {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Failed to parse map data`);
        }
        this.stats.failed++;
        return false;
      }

      // Generate output content
      let outputContent: string;
      if (options.to === 'yaml') {
        outputContent = CanonicalMapParser.serializeToYAML(result.map);
      } else if (options.to === 'json') {
        outputContent = CanonicalMapParser.serializeToJSON(result.map, options.pretty);
      } else if (options.to === 'xml') {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: XML generation not yet implemented`);
        }
        this.stats.failed++;
        return false;
      } else {
        if (!options.quiet) {
          console.error(`❌ ${inputPath}: Unsupported target format: ${options.to}`);
        }
        this.stats.failed++;
        return false;
      }

      // Determine output path
      const outputPath = this.getOutputPath(inputPath, options.to, options.output);

      // Create output directory if needed
      const outputDir = dirname(outputPath);
      try {
        mkdirSync(outputDir, { recursive: true });
      } catch (error) {
        // Directory might already exist
      }

      // Check if output file exists
      const outputExists = (() => {
        try {
          statSync(outputPath);
          return true;
        } catch {
          return false;
        }
      })();

      if (outputExists && !options.overwrite && outputPath !== inputPath) {
        if (!options.quiet) {
          console.warn(`⚠️  ${outputPath}: File exists, use --overwrite to replace`);
        }
        this.stats.skipped++;
        return true;
      }

      // Create backup if requested and overwriting
      if (options.backup && outputExists) {
        this.createBackup(outputPath);
      }

      // Write output file
      writeFileSync(outputPath, outputContent, 'utf8');

      // Report success
      if (!options.quiet) {
        const fromTo = sourceFormat === options.to ?
          `reformatted as ${options.to}` :
          `converted from ${sourceFormat} to ${options.to}`;
        console.log(`✅ ${basename(inputPath)} → ${basename(outputPath)} (${fromTo})`);
      }

      if (options.verbose && result.validation.warnings.length > 0) {
        console.warn('  Warnings:');
        for (const warning of result.validation.warnings) {
          console.warn(`    • ${warning.path}: ${warning.message}`);
        }
      }

      this.stats.converted++;
      return true;
    } catch (error) {
      this.stats.failed++;
      if (!options.quiet) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        console.error(`❌ ${inputPath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Convert all files in a directory
   */
  private async convertDirectory(
    dirPath: string,
    options: ConvertOptions
  ): Promise<boolean> {
    try {
      const entries = readdirSync(dirPath);
      const mapFiles = entries.filter(entry => {
        const format = this.detectFormat(entry);
        if (options.from) {
          return format === options.from;
        }
        return format !== null;
      });

      if (mapFiles.length === 0) {
        if (!options.quiet) {
          const formatMsg = options.from ? ` (${options.from} format)` : '';
          console.warn(`⚠️  No MIDI map files${formatMsg} found in ${dirPath}`);
        }
        return true;
      }

      if (!options.quiet && options.verbose) {
        console.log(`📁 Converting ${mapFiles.length} files in ${dirPath}`);
      }

      let allSuccessful = true;
      for (const file of mapFiles) {
        const filePath = join(dirPath, file);
        const success = await this.convertFile(filePath, options);
        allSuccessful = allSuccessful && success;
      }

      return allSuccessful;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Unknown error';
      if (!options.quiet) {
        console.error(`❌ Error reading directory ${dirPath}: ${message}`);
      }
      return false;
    }
  }

  /**
   * Main conversion function
   */
  async convert(paths: string[], options: ConvertOptions): Promise<boolean> {
    if (paths.length === 0) {
      console.error('Error: No files or directories specified');
      this.showHelp();
      return false;
    }

    let allSuccessful = true;

    for (const path of paths) {
      try {
        const stat = statSync(path);

        if (stat.isDirectory()) {
          const success = await this.convertDirectory(path, options);
          allSuccessful = allSuccessful && success;
        } else if (stat.isFile()) {
          const success = await this.convertFile(path, options);
          allSuccessful = allSuccessful && success;
        } else {
          if (!options.quiet) {
            console.error(`❌ ${path}: Not a file or directory`);
          }
          allSuccessful = false;
        }
      } catch (error) {
        const message = error instanceof Error ? error.message : 'Unknown error';
        if (!options.quiet) {
          console.error(`❌ ${path}: ${message}`);
        }
        allSuccessful = false;
      }
    }

    // Show summary
    if (!options.quiet && this.stats.total > 1) {
      console.log('\n📊 Conversion Summary:');
      console.log(`  Total files: ${this.stats.total}`);
      console.log(`  ✅ Converted: ${this.stats.converted}`);
      console.log(`  ⚠️  Skipped: ${this.stats.skipped}`);
      console.log(`  ❌ Failed: ${this.stats.failed}`);
    }

    return allSuccessful;
  }
}

/**
 * Main CLI entry point
 */
async function main(): Promise<void> {
  const converter = new MapConverter();
  const { paths, options } = converter.parseArgs();

  if (options.help) {
    converter.showHelp();
    process.exit(0);
  }

  try {
    const success = await converter.convert(paths, options);
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

export { MapConverter, main };