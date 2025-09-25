#!/usr/bin/env tsx

/**
 * Plugin List Tool
 * Lists available audio plugins on the system
 */

import { parseArgs } from 'node:util';
import { existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface ListOptions {
  format?: 'json' | 'table' | 'simple';
  type?: 'vst3' | 'au' | 'vst' | 'all';
  cached?: boolean;
}

function parseCliArgs(): ListOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      format: { type: 'string', default: 'table' },
      type: { type: 'string', default: 'all' },
      cached: { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm plugins:list [options]

Options:
  --format <type>   Output format: table, json, simple (default: table)
  --type <plugin>   Plugin type: vst3, au, vst, all (default: all)
  --cached         Show only cached/extracted plugins
  -h, --help        Show this help

Examples:
  pnpm plugins:list
  pnpm plugins:list --type vst3 --format json
  pnpm plugins:list --cached
`);
    process.exit(0);
  }

  return values as ListOptions;
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

async function listPlugins(options: ListOptions): Promise<void> {
  console.log('🔍 Scanning for audio plugins...');
  console.log(`📋 Format: ${options.format}`);
  console.log(`🎛️  Type: ${options.type}`);

  if (options.cached) {
    const cacheFile = resolve(__dirname, '../../data/plugins-extracted.json');
    if (!existsSync(cacheFile)) {
      console.log('⚠️  No cached plugin data found. Run extraction first:');
      console.log('  pnpm plugins:extract');
      return;
    }
    console.log(`📁 Using cached data: ${cacheFile}`);
  } else {
    const juceHost = findJuceHost();
    console.log(`🎛️  JUCE Host: ${juceHost}`);
  }

  // TODO: Implement actual listing logic
  // For now, provide clear error message about missing implementation
  throw new Error(
    'Plugin listing not yet implemented.\n' +
    'This tool needs to:\n' +
    '1. Execute JUCE host to scan plugins\n' +
    '2. Parse plugin list output\n' +
    '3. Format results (table, json, simple)\n' +
    '4. Filter by plugin type if specified\n' +
    '5. Support cached data display\n' +
    '\n' +
    'Implementation required in tools/plugins/list.ts'
  );
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await listPlugins(options);
  } catch (error: any) {
    console.error('❌ Plugin listing failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}