#!/usr/bin/env tsx

/**
 * Plugin Health Check Tool
 * Verifies plugin extraction status and data integrity
 */

import { parseArgs } from 'node:util';
import { existsSync, statSync, readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface HealthOptions {
  verbose?: boolean;
  fix?: boolean;
}

interface HealthResult {
  status: 'healthy' | 'warning' | 'error';
  message: string;
  details?: string[];
}

function parseCliArgs(): HealthOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      verbose: { type: 'boolean', short: 'v', default: false },
      fix: { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm plugins:health [options]

Options:
  -v, --verbose     Show detailed health information
  --fix             Attempt to fix common issues
  -h, --help        Show this help

Examples:
  pnpm plugins:health
  pnpm plugins:health --verbose
  pnpm plugins:health --fix
`);
    process.exit(0);
  }

  return values as HealthOptions;
}

function checkJuceHost(): HealthResult {
  const possiblePaths = [
    '../../cmake-build/modules/juce/host/plughost_artefacts/plughost',
    '../../../cmake-build/modules/juce/host/plughost_artefacts/plughost',
    resolve(__dirname, '../juce-plugin-host/plughost')
  ];

  for (const path of possiblePaths) {
    const fullPath = resolve(__dirname, path);
    if (existsSync(fullPath)) {
      const stats = statSync(fullPath);
      return {
        status: 'healthy',
        message: `JUCE Host found: ${fullPath}`,
        details: [
          `Size: ${Math.round(stats.size / 1024)}KB`,
          `Modified: ${stats.mtime.toISOString()}`
        ]
      };
    }
  }

  return {
    status: 'error',
    message: 'JUCE plugin host not found',
    details: [
      'Build the project: cd ../../ && make',
      'Expected locations:',
      ...possiblePaths.map(p => `  ${p}`)
    ]
  };
}

function checkExtractedData(): HealthResult {
  const dataPath = resolve(__dirname, '../../data/plugins-extracted.json');

  if (!existsSync(dataPath)) {
    return {
      status: 'warning',
      message: 'No extracted plugin data found',
      details: [
        'Run extraction: pnpm plugins:extract',
        `Expected location: ${dataPath}`
      ]
    };
  }

  try {
    const content = readFileSync(dataPath, 'utf-8');
    const data = JSON.parse(content);
    const stats = statSync(dataPath);

    return {
      status: 'healthy',
      message: `Extracted data found: ${Object.keys(data).length} plugins`,
      details: [
        `File size: ${Math.round(stats.size / 1024)}KB`,
        `Last updated: ${stats.mtime.toISOString()}`,
        `Data structure: ${typeof data}`
      ]
    };
  } catch (error: any) {
    return {
      status: 'error',
      message: 'Extracted data file is corrupted',
      details: [
        `Error: ${error.message}`,
        'Re-run extraction: pnpm plugins:extract --force'
      ]
    };
  }
}

function checkDataDirectory(): HealthResult {
  const dataDir = resolve(__dirname, '../../data');

  if (!existsSync(dataDir)) {
    return {
      status: 'warning',
      message: 'Data directory does not exist',
      details: [
        `Expected: ${dataDir}`,
        'Will be created during extraction'
      ]
    };
  }

  const stats = statSync(dataDir);
  return {
    status: 'healthy',
    message: `Data directory exists: ${dataDir}`,
    details: [
      `Created: ${stats.birthtime.toISOString()}`,
      `Modified: ${stats.mtime.toISOString()}`
    ]
  };
}

async function runHealthCheck(options: HealthOptions): Promise<void> {
  console.log('🏥 Running plugin system health check...\n');

  const checks = [
    { name: 'JUCE Host', check: checkJuceHost },
    { name: 'Data Directory', check: checkDataDirectory },
    { name: 'Extracted Data', check: checkExtractedData }
  ];

  let hasErrors = false;
  let hasWarnings = false;

  for (const { name, check } of checks) {
    const result = check();

    const icon = result.status === 'healthy' ? '✅' :
                 result.status === 'warning' ? '⚠️' : '❌';

    console.log(`${icon} ${name}: ${result.message}`);

    if (options.verbose && result.details) {
      for (const detail of result.details) {
        console.log(`   ${detail}`);
      }
    }

    if (result.status === 'error') hasErrors = true;
    if (result.status === 'warning') hasWarnings = true;

    console.log();
  }

  if (hasErrors) {
    console.log('❌ Health check failed with errors');
    console.log('💡 Use --fix to attempt automatic fixes');
    process.exit(1);
  } else if (hasWarnings) {
    console.log('⚠️  Health check completed with warnings');
    console.log('💡 Consider running extraction: pnpm plugins:extract');
  } else {
    console.log('✅ Plugin system is healthy');
  }

  if (options.fix) {
    console.log('\n🔧 Fix mode not yet implemented');
    throw new Error(
      'Automatic fixes not yet implemented.\n' +
      'This feature needs to:\n' +
      '1. Create missing directories\n' +
      '2. Fix file permissions\n' +
      '3. Re-run extraction if data is corrupted\n' +
      '4. Rebuild JUCE host if missing\n' +
      '\n' +
      'Implementation required in tools/plugins/health.ts'
    );
  }
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await runHealthCheck(options);
  } catch (error: any) {
    console.error('❌ Health check failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}