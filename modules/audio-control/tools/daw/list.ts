#!/usr/bin/env tsx

/**
 * DAW List Tool
 * Lists generated DAW-specific MIDI map configurations and installation status
 */

import { parseArgs } from 'node:util';
import { existsSync, readFileSync, readdirSync, statSync } from 'node:fs';
import { resolve, dirname, extname, join, basename } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface ListOptions {
  target?: 'ardour' | 'ableton' | 'reaper' | 'all';
  format?: 'table' | 'json' | 'simple';
  status?: boolean;
}

interface DawMapInfo {
  file: string;
  target: string;
  size: number;
  modified: string;
  installed?: boolean;
  installPath?: string;
}

function parseCliArgs(): ListOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      target: { type: 'string', short: 't', default: 'all' },
      format: { type: 'string', default: 'table' },
      status: { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm daw:list [options]

Options:
  -t, --target <daw>    Target DAW: ardour, ableton, reaper, all (default: all)
  --format <type>       Output format: table, json, simple (default: table)
  --status              Check installation status
  -h, --help            Show this help

Examples:
  pnpm daw:list
  pnpm daw:list --target ardour --status
  pnpm daw:list --format json
`);
    process.exit(0);
  }

  return values as ListOptions;
}

function getDefaultOutputDir(target: string): string {
  return resolve(__dirname, `../../generated/${target}-maps`);
}

function getInstallPath(target: string): string | null {
  switch (target) {
    case 'ardour':
      // Common Ardour MIDI map directories
      const ardourPaths = [
        '~/.config/ardour8/midi_maps',
        '~/.config/ardour7/midi_maps',
        '~/.config/ardour6/midi_maps',
        '/usr/share/ardour8/midi_maps',
        '/usr/local/share/ardour8/midi_maps'
      ];

      for (const path of ardourPaths) {
        const expandedPath = path.replace('~', process.env.HOME || '');
        if (existsSync(expandedPath)) {
          return expandedPath;
        }
      }
      return null;

    case 'ableton':
      // Ableton Live user library paths
      const abletonPaths = [
        '~/Music/Ableton/User Library/Remote Scripts',
        '~/Documents/Ableton/User Library/Remote Scripts'
      ];
      for (const path of abletonPaths) {
        const expandedPath = path.replace('~', process.env.HOME || '');
        if (existsSync(expandedPath)) {
          return expandedPath;
        }
      }
      return null;

    case 'reaper':
      // REAPER resource directory
      const reaperPaths = [
        '~/Library/Application Support/REAPER',
        '~/.config/REAPER'
      ];
      for (const path of reaperPaths) {
        const expandedPath = path.replace('~', process.env.HOME || '');
        if (existsSync(expandedPath)) {
          return expandedPath;
        }
      }
      return null;

    default:
      return null;
  }
}

function checkInstallationStatus(file: string, target: string): { installed: boolean; installPath?: string } {
  const installPath = getInstallPath(target);

  if (!installPath) {
    return { installed: false };
  }

  const installedFile = join(installPath, file);
  const installed = existsSync(installedFile);

  return {
    installed,
    installPath: installed ? installedFile : undefined
  };
}

function scanGeneratedMaps(target: string, checkStatus: boolean): DawMapInfo[] {
  const outputDir = getDefaultOutputDir(target);

  if (!existsSync(outputDir)) {
    return [];
  }

  try {
    const files = readdirSync(outputDir);
    const maps: DawMapInfo[] = [];

    for (const file of files) {
      const filePath = join(outputDir, file);
      const stats = statSync(filePath);

      const mapInfo: DawMapInfo = {
        file,
        target,
        size: stats.size,
        modified: stats.mtime.toISOString()
      };

      if (checkStatus) {
        const status = checkInstallationStatus(file, target);
        mapInfo.installed = status.installed;
        mapInfo.installPath = status.installPath;
      }

      maps.push(mapInfo);
    }

    return maps.sort((a, b) => a.file.localeCompare(b.file));

  } catch (error: any) {
    console.warn(`Warning: Could not scan ${target} maps: ${error.message}`);
    return [];
  }
}

function formatResults(maps: DawMapInfo[], format: string): void {
  if (format === 'json') {
    console.log(JSON.stringify(maps, null, 2));
    return;
  }

  if (format === 'simple') {
    for (const map of maps) {
      console.log(`${map.target}/${map.file}`);
    }
    return;
  }

  // Table format (default)
  if (maps.length === 0) {
    console.log('No generated DAW maps found');
    console.log('💡 Run generation first: pnpm daw:generate');
    return;
  }

  // Group by target
  const groupedMaps = maps.reduce((acc, map) => {
    if (!acc[map.target]) {
      acc[map.target] = [];
    }
    acc[map.target].push(map);
    return acc;
  }, {} as Record<string, DawMapInfo[]>);

  console.log('\n📋 Generated DAW Maps:\n');

  for (const [target, targetMaps] of Object.entries(groupedMaps)) {
    console.log(`🎛️  ${target.toUpperCase()} (${targetMaps.length} maps):`);

    const maxFileLen = Math.max(15, Math.max(...targetMaps.map(m => m.file.length)));

    for (const map of targetMaps) {
      const sizeStr = `${Math.round(map.size / 1024)}KB`;
      const dateStr = new Date(map.modified).toLocaleDateString();

      let statusIcon = '';
      let statusText = '';

      if (map.installed !== undefined) {
        statusIcon = map.installed ? '✅' : '❌';
        statusText = map.installed ? ' (installed)' : ' (not installed)';
      }

      console.log(
        `  ${statusIcon} ${map.file.padEnd(maxFileLen)} ${sizeStr.padEnd(6)} ${dateStr}${statusText}`
      );

      if (map.installPath) {
        console.log(`     📂 ${map.installPath}`);
      }
    }

    console.log();
  }

  const totalMaps = maps.length;
  const installedMaps = maps.filter(m => m.installed).length;

  console.log(`📊 Total: ${totalMaps} generated maps`);
  if (maps.some(m => m.installed !== undefined)) {
    console.log(`📦 Installed: ${installedMaps}/${totalMaps} maps`);
  }
}

function checkDawInstallations(): void {
  console.log('🔍 Checking DAW installations:\n');

  const targets = ['ardour', 'ableton', 'reaper'];

  for (const target of targets) {
    const installPath = getInstallPath(target);
    const icon = installPath ? '✅' : '❌';
    const status = installPath ? `Found: ${installPath}` : 'Not found';

    console.log(`${icon} ${target.toUpperCase()}: ${status}`);
  }

  console.log();
}

async function listDawMaps(options: ListOptions): Promise<void> {
  console.log('🎛️  Scanning generated DAW maps...');

  if (options.status) {
    checkDawInstallations();
  }

  const targets = options.target === 'all' ? ['ardour', 'ableton', 'reaper'] : [options.target!];
  const allMaps: DawMapInfo[] = [];

  for (const target of targets) {
    const maps = scanGeneratedMaps(target, options.status || false);
    allMaps.push(...maps);
  }

  formatResults(allMaps, options.format || 'table');

  if (allMaps.length === 0) {
    console.log('\n💡 To generate maps, run:');
    console.log('  pnpm daw:generate');
    console.log('  pnpm daw:generate:ardour --install');
  }
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await listDawMaps(options);
  } catch (error: any) {
    console.error('❌ DAW listing failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}