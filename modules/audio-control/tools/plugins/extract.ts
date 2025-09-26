#!/usr/bin/env tsx

/**
 * Plugin Extraction Tool
 * Extracts parameter information from audio plugins using JUCE host
 */

import { parseArgs } from 'node:util';
import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface ExtractOptions {
  force?: boolean;
  format?: 'json' | 'yaml';
  output?: string;
  plugin?: string;
}

function parseCliArgs(): ExtractOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      force: { type: 'boolean', default: false },
      format: { type: 'string', default: 'json' },
      output: { type: 'string' },
      plugin: { type: 'string' },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm plugins:extract [options]

Options:
  --force           Force re-extraction even if cache exists
  --format <type>   Output format: json, yaml (default: json)
  --output <path>   Output file path
  --plugin <name>   Extract specific plugin only
  -h, --help        Show this help

Examples:
  pnpm plugins:extract
  pnpm plugins:extract:force
  pnpm plugins:extract --plugin "Serum"
  pnpm plugins:extract --format yaml --output ./plugins.yaml
`);
    process.exit(0);
  }

  return values as ExtractOptions;
}

function findJuceHost(): string {
  const possiblePaths = [
    '../../cmake-build/modules/juce/host/plughost_artefacts/plughost',
    '../../../cmake-build/modules/juce/host/plughost_artefacts/plughost',
    resolve(__dirname, '../juce-plugin-host/plughost')
  ];

  for (const path of possiblePaths) {
    const fullPath = resolve(__dirname, path);
    if (existsSync(fullPath)) {
      return fullPath;
    }
  }

  throw new Error(
    'JUCE plugin host not found. Please build the project first:\n' +
    '  cd ../../ && make\n' +
    'Or ensure plughost is available in one of these locations:\n' +
    possiblePaths.map(p => `  ${p}`).join('\n')
  );
}

async function extractPluginData(options: ExtractOptions): Promise<void> {
  const juceHost = findJuceHost();
  const outputPath = options.output || resolve(__dirname, '../../data/plugins-extracted.json');

  console.log('🔍 Starting plugin extraction...');
  console.log(`📁 Output: ${outputPath}`);
  console.log(`🎛️  JUCE Host: ${juceHost}`);

  if (!options.force && existsSync(outputPath)) {
    console.log('⚠️  Extracted data already exists. Use --force to re-extract.');
    return;
  }

  // TODO: Implement actual extraction logic
  // For now, provide clear error message about missing implementation
  throw new Error(
    'Plugin extraction not yet implemented.\n' +
    'This tool needs to:\n' +
    '1. Execute JUCE host to scan plugins\n' +
    '2. Parse plugin parameter information\n' +
    '3. Generate structured output data\n' +
    '4. Cache results for performance\n' +
    '\n' +
    'Implementation required in tools/plugins/extract.ts'
  );
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await extractPluginData(options);
    console.log('✅ Plugin extraction completed successfully');
  } catch (error: any) {
    console.error('❌ Plugin extraction failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}