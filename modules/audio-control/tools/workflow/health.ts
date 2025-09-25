#!/usr/bin/env tsx

/**
 * Workflow Health Check Tool
 * Comprehensive health check for the entire MIDI mapping toolchain
 */

import { parseArgs } from 'node:util';
import { existsSync, statSync, readFileSync, readdirSync } from 'node:fs';
import { resolve, dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execSync } from 'node:child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface HealthOptions {
  verbose?: boolean;
  fix?: boolean;
  format?: 'table' | 'json';
}

interface HealthCheck {
  category: string;
  name: string;
  status: 'healthy' | 'warning' | 'error' | 'info';
  message: string;
  details?: string[];
  fixable?: boolean;
}

function parseCliArgs(): HealthOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      verbose: { type: 'boolean', short: 'v', default: false },
      fix: { type: 'boolean', default: false },
      format: { type: 'string', default: 'table' },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm workflow:health [options]

Performs comprehensive health check of the MIDI mapping toolchain:
- System dependencies and tools
- Project structure and configuration
- Data availability and integrity
- DAW installations and paths
- Tool functionality

Options:
  -v, --verbose         Show detailed health information
  --fix                 Attempt to fix common issues automatically
  --format <type>       Output format: table, json (default: table)
  -h, --help            Show this help

Examples:
  pnpm workflow:health
  pnpm workflow:health --verbose
  pnpm workflow:health --fix
`);
    process.exit(0);
  }

  return values as HealthOptions;
}

function checkSystemDependencies(): HealthCheck[] {
  const checks: HealthCheck[] = [];

  // Node.js version
  try {
    const nodeVersion = process.version;
    const majorVersion = parseInt(nodeVersion.slice(1));

    if (majorVersion >= 18) {
      checks.push({
        category: 'System',
        name: 'Node.js Version',
        status: 'healthy',
        message: `Node.js ${nodeVersion} (compatible)`,
        details: ['Minimum required: Node.js 18+']
      });
    } else {
      checks.push({
        category: 'System',
        name: 'Node.js Version',
        status: 'error',
        message: `Node.js ${nodeVersion} (too old)`,
        details: ['Minimum required: Node.js 18+', 'Please upgrade Node.js'],
        fixable: false
      });
    }
  } catch (error: any) {
    checks.push({
      category: 'System',
      name: 'Node.js Version',
      status: 'error',
      message: 'Failed to check Node.js version',
      details: [error.message]
    });
  }

  // pnpm availability
  try {
    const pnpmVersion = execSync('pnpm --version', { encoding: 'utf-8' }).trim();
    checks.push({
      category: 'System',
      name: 'pnpm Package Manager',
      status: 'healthy',
      message: `pnpm ${pnpmVersion}`,
      details: ['Package manager available']
    });
  } catch (error: any) {
    checks.push({
      category: 'System',
      name: 'pnpm Package Manager',
      status: 'error',
      message: 'pnpm not found',
      details: ['Install pnpm: npm install -g pnpm'],
      fixable: false
    });
  }

  // tsx availability
  try {
    execSync('tsx --version', { encoding: 'utf-8', stdio: 'pipe' });
    checks.push({
      category: 'System',
      name: 'TypeScript Executor',
      status: 'healthy',
      message: 'tsx available',
      details: ['TypeScript execution environment ready']
    });
  } catch (error: any) {
    checks.push({
      category: 'System',
      name: 'TypeScript Executor',
      status: 'warning',
      message: 'tsx not globally available',
      details: [
        'Using project-local tsx',
        'Install globally: npm install -g tsx (optional)'
      ]
    });
  }

  return checks;
}

function checkProjectStructure(): HealthCheck[] {
  const checks: HealthCheck[] = [];
  const projectRoot = resolve(__dirname, '../..');

  // Essential directories
  const requiredDirs = [
    { path: 'tools', name: 'Tools Directory' },
    { path: 'tools/plugins', name: 'Plugin Tools' },
    { path: 'tools/maps', name: 'Map Tools' },
    { path: 'tools/daw', name: 'DAW Tools' },
    { path: 'tools/workflow', name: 'Workflow Tools' },
    { path: 'modules', name: 'Modules Directory' },
    { path: 'modules/canonical-midi-maps', name: 'Canonical Maps Module' },
    { path: 'modules/ardour-midi-maps', name: 'Ardour Maps Module' }
  ];

  for (const { path, name } of requiredDirs) {
    const fullPath = join(projectRoot, path);
    if (existsSync(fullPath)) {
      checks.push({
        category: 'Structure',
        name,
        status: 'healthy',
        message: `Directory exists: ${path}`,
        details: [`Path: ${fullPath}`]
      });
    } else {
      checks.push({
        category: 'Structure',
        name,
        status: 'error',
        message: `Missing directory: ${path}`,
        details: [`Expected: ${fullPath}`],
        fixable: true
      });
    }
  }

  // Essential files
  const requiredFiles = [
    { path: 'package.json', name: 'Root Package Config' },
    { path: 'tsconfig.json', name: 'TypeScript Config' },
    { path: 'pnpm-workspace.yaml', name: 'Workspace Config' }
  ];

  for (const { path, name } of requiredFiles) {
    const fullPath = join(projectRoot, path);
    if (existsSync(fullPath)) {
      const stats = statSync(fullPath);
      checks.push({
        category: 'Structure',
        name,
        status: 'healthy',
        message: `File exists: ${path}`,
        details: [
          `Size: ${Math.round(stats.size / 1024)}KB`,
          `Modified: ${stats.mtime.toLocaleDateString()}`
        ]
      });
    } else {
      checks.push({
        category: 'Structure',
        name,
        status: 'error',
        message: `Missing file: ${path}`,
        details: [`Expected: ${fullPath}`],
        fixable: false
      });
    }
  }

  return checks;
}

function checkDataAvailability(): HealthCheck[] {
  const checks: HealthCheck[] = [];
  const projectRoot = resolve(__dirname, '../..');

  // Canonical maps
  const mapsDir = join(projectRoot, 'modules/canonical-midi-maps/maps');
  if (existsSync(mapsDir)) {
    try {
      const mapFiles = readdirSync(mapsDir)
        .filter(file => file.match(/\.(yaml|yml|json)$/i));

      if (mapFiles.length > 0) {
        checks.push({
          category: 'Data',
          name: 'Canonical Maps',
          status: 'healthy',
          message: `${mapFiles.length} canonical map files found`,
          details: [`Directory: ${mapsDir}`, `Files: ${mapFiles.slice(0, 3).join(', ')}${mapFiles.length > 3 ? '...' : ''}`]
        });
      } else {
        checks.push({
          category: 'Data',
          name: 'Canonical Maps',
          status: 'warning',
          message: 'No canonical map files found',
          details: [`Directory exists: ${mapsDir}`, 'Create some .yaml or .json map files'],
          fixable: false
        });
      }
    } catch (error: any) {
      checks.push({
        category: 'Data',
        name: 'Canonical Maps',
        status: 'error',
        message: 'Failed to scan maps directory',
        details: [error.message]
      });
    }
  } else {
    checks.push({
      category: 'Data',
      name: 'Canonical Maps',
      status: 'warning',
      message: 'Maps directory does not exist',
      details: [`Expected: ${mapsDir}`],
      fixable: true
    });
  }

  // Extracted plugin data
  const pluginDataPath = join(projectRoot, 'data/plugins-extracted.json');
  if (existsSync(pluginDataPath)) {
    try {
      const content = readFileSync(pluginDataPath, 'utf-8');
      const data = JSON.parse(content);
      const pluginCount = Object.keys(data).length;

      checks.push({
        category: 'Data',
        name: 'Plugin Data',
        status: 'healthy',
        message: `Plugin data available (${pluginCount} plugins)`,
        details: [`File: ${pluginDataPath}`, `Plugins: ${pluginCount}`]
      });
    } catch (error: any) {
      checks.push({
        category: 'Data',
        name: 'Plugin Data',
        status: 'error',
        message: 'Plugin data file is corrupted',
        details: [error.message, 'Re-run: pnpm plugins:extract --force'],
        fixable: true
      });
    }
  } else {
    checks.push({
      category: 'Data',
      name: 'Plugin Data',
      status: 'info',
      message: 'No extracted plugin data',
      details: ['Run: pnpm plugins:extract', `Will create: ${pluginDataPath}`],
      fixable: true
    });
  }

  return checks;
}

function checkDawInstallations(): HealthCheck[] {
  const checks: HealthCheck[] = [];

  const dawConfigs = [
    {
      name: 'Ardour',
      paths: [
        '~/.config/ardour8/midi_maps',
        '~/.config/ardour7/midi_maps',
        '~/.config/ardour6/midi_maps'
      ]
    },
    {
      name: 'Ableton Live',
      paths: [
        '~/Music/Ableton/User Library/Remote Scripts',
        '~/Documents/Ableton/User Library/Remote Scripts'
      ]
    },
    {
      name: 'REAPER',
      paths: [
        '~/Library/Application Support/REAPER',
        '~/.config/REAPER'
      ]
    }
  ];

  for (const { name, paths } of dawConfigs) {
    let found = false;
    let foundPath = '';

    for (const path of paths) {
      const expandedPath = path.replace('~', process.env.HOME || '');
      if (existsSync(expandedPath)) {
        found = true;
        foundPath = expandedPath;
        break;
      }
    }

    if (found) {
      checks.push({
        category: 'DAWs',
        name: `${name} Installation`,
        status: 'healthy',
        message: `${name} found`,
        details: [`Config directory: ${foundPath}`]
      });
    } else {
      checks.push({
        category: 'DAWs',
        name: `${name} Installation`,
        status: 'info',
        message: `${name} not found`,
        details: [
          'DAW not installed or non-standard location',
          'Expected paths:',
          ...paths.map(p => `  ${p}`)
        ]
      });
    }
  }

  return checks;
}

function checkToolFunctionality(): HealthCheck[] {
  const checks: HealthCheck[] = [];
  const projectRoot = resolve(__dirname, '../..');

  // Check if tools are executable
  const tools = [
    { path: 'tools/plugins/extract.ts', name: 'Plugin Extraction' },
    { path: 'tools/plugins/list.ts', name: 'Plugin Listing' },
    { path: 'tools/plugins/health.ts', name: 'Plugin Health' },
    { path: 'tools/maps/validate.ts', name: 'Map Validation' },
    { path: 'tools/maps/list.ts', name: 'Map Listing' },
    { path: 'tools/maps/check.ts', name: 'Map Compatibility' },
    { path: 'tools/daw/generate.ts', name: 'DAW Generation' },
    { path: 'tools/daw/list.ts', name: 'DAW Listing' },
    { path: 'tools/workflow/complete.ts', name: 'Complete Workflow' }
  ];

  for (const { path, name } of tools) {
    const fullPath = join(projectRoot, path);
    if (existsSync(fullPath)) {
      const stats = statSync(fullPath);
      const isExecutable = !!(stats.mode & parseInt('111', 8));

      checks.push({
        category: 'Tools',
        name,
        status: 'healthy',
        message: 'Tool script exists',
        details: [
          `Path: ${path}`,
          `Size: ${Math.round(stats.size / 1024)}KB`,
          `Executable: ${isExecutable ? 'Yes' : 'No'}`
        ]
      });
    } else {
      checks.push({
        category: 'Tools',
        name,
        status: 'error',
        message: 'Tool script missing',
        details: [`Expected: ${fullPath}`],
        fixable: false
      });
    }
  }

  return checks;
}

function formatHealthResults(checks: HealthCheck[], format: string, verbose: boolean): void {
  if (format === 'json') {
    console.log(JSON.stringify(checks, null, 2));
    return;
  }

  // Group checks by category
  const categories = [...new Set(checks.map(c => c.category))];

  console.log('\n🏥 Workflow Health Check Results:\n');

  for (const category of categories) {
    const categoryChecks = checks.filter(c => c.category === category);
    const healthy = categoryChecks.filter(c => c.status === 'healthy').length;
    const total = categoryChecks.length;

    console.log(`📂 ${category} (${healthy}/${total} healthy):`);

    for (const check of categoryChecks) {
      const icon = {
        healthy: '✅',
        warning: '⚠️',
        error: '❌',
        info: 'ℹ️'
      }[check.status] || '❓';

      console.log(`  ${icon} ${check.name}: ${check.message}`);

      if (verbose && check.details) {
        for (const detail of check.details) {
          console.log(`     ${detail}`);
        }
      }

      if (check.fixable && check.status !== 'healthy') {
        console.log(`     🔧 Fixable automatically`);
      }
    }

    console.log();
  }

  // Summary
  const statusCounts = {
    healthy: checks.filter(c => c.status === 'healthy').length,
    warning: checks.filter(c => c.status === 'warning').length,
    error: checks.filter(c => c.status === 'error').length,
    info: checks.filter(c => c.status === 'info').length
  };

  console.log('📊 Summary:');
  console.log(`  ✅ Healthy: ${statusCounts.healthy}`);
  console.log(`  ⚠️  Warnings: ${statusCounts.warning}`);
  console.log(`  ❌ Errors: ${statusCounts.error}`);
  console.log(`  ℹ️  Info: ${statusCounts.info}`);
  console.log(`  📊 Total checks: ${checks.length}`);
}

async function runHealthCheck(options: HealthOptions): Promise<void> {
  console.log('🔍 Running comprehensive workflow health check...');

  const allChecks: HealthCheck[] = [
    ...checkSystemDependencies(),
    ...checkProjectStructure(),
    ...checkDataAvailability(),
    ...checkDawInstallations(),
    ...checkToolFunctionality()
  ];

  formatHealthResults(allChecks, options.format || 'table', options.verbose || false);

  const hasErrors = allChecks.some(c => c.status === 'error');
  const hasWarnings = allChecks.some(c => c.status === 'warning');

  if (hasErrors) {
    console.log('\n❌ Health check found critical errors');
    if (options.fix) {
      console.log('🔧 Attempting automatic fixes...');
      // TODO: Implement fix functionality
      console.log('⚠️  Automatic fixes not yet implemented');
    }
    process.exit(1);
  } else if (hasWarnings) {
    console.log('\n⚠️  Health check completed with warnings');
    console.log('💡 Consider addressing warnings for optimal functionality');
  } else {
    console.log('\n✅ Workflow is healthy and ready to use');
  }

  if (options.fix) {
    throw new Error(
      'Automatic fixes not yet implemented.\n' +
      'This feature needs to:\n' +
      '1. Create missing directories\n' +
      '2. Initialize empty data files\n' +
      '3. Fix file permissions\n' +
      '4. Download missing dependencies\n' +
      '\n' +
      'Implementation required in tools/workflow/health.ts'
    );
  }
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await runHealthCheck(options);
  } catch (error: any) {
    console.error('❌ Workflow health check failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}