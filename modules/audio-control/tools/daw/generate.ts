#!/usr/bin/env tsx

/**
 * DAW Generation Tool
 * Generates DAW-specific MIDI map configurations from canonical maps
 */

import { parseArgs } from 'node:util';
import { existsSync, readFileSync, writeFileSync, readdirSync, mkdirSync, statSync } from 'node:fs';
import { resolve, dirname, extname, join, basename } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execSync } from 'node:child_process';
import { parse as parseYAML } from 'yaml';
import { MidiMapBuilder, ArdourXMLSerializer } from '../../modules/ardour-midi-maps/src/index.js';
import type { CanonicalMidiMap, MidiControl } from '../types/midi.js';
import type { ArdourFunction } from '../../modules/ardour-midi-maps/src/types/ardour.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface GenerateOptions {
  target?: 'ardour' | 'ableton' | 'reaper' | 'all';
  input?: string;
  output?: string;
  install?: boolean;
  force?: boolean;
}

function parseCliArgs(): GenerateOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      target: { type: 'string', short: 't', default: 'all' },
      input: { type: 'string', short: 'i' },
      output: { type: 'string', short: 'o' },
      install: { type: 'boolean', default: false },
      force: { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm daw:generate [options]

Options:
  -t, --target <daw>    Target DAW: ardour, ableton, reaper, all (default: all)
  -i, --input <dir>     Input directory for canonical maps
  -o, --output <dir>    Output directory for generated maps
  --install             Install generated maps to DAW directories
  --force               Overwrite existing generated maps
  -h, --help            Show this help

Examples:
  pnpm daw:generate
  pnpm daw:generate:ardour
  pnpm daw:generate:ardour --install
  pnpm daw:generate --target reaper --output ./reaper-maps
`);
    process.exit(0);
  }

  return values as GenerateOptions;
}

function findMapsDirectory(): string {
  const possiblePaths = [
    resolve(__dirname, '../../modules/canonical-midi-maps/maps'),
    resolve(__dirname, '../../maps'),
    resolve(__dirname, '../../../canonical-midi-maps/maps')
  ];

  for (const path of possiblePaths) {
    if (existsSync(path)) {
      return path;
    }
  }

  throw new Error(
    'Maps directory not found. Expected locations:\n' +
    possiblePaths.map(p => `  ${p}`).join('\n')
  );
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
      // TODO: Implement Ableton installation detection
      return null;

    case 'reaper':
      // REAPER resource directory
      const reaperPaths = [
        '~/Library/Application Support/REAPER',
        '~/.config/REAPER'
      ];
      // TODO: Implement REAPER installation detection
      return null;

    default:
      return null;
  }
}

function generateArdourMap(canonicalMap: any, outputPath: string): void {
  // Create Ardour MIDI map using the builder pattern
  const mapName = canonicalMap.plugin ?
    `${canonicalMap.device.manufacturer} ${canonicalMap.device.model} - ${canonicalMap.plugin.name}` :
    `${canonicalMap.device.manufacturer} ${canonicalMap.device.model}`;

  const ardourBuilder = new MidiMapBuilder({
    name: mapName,
    version: canonicalMap.version,
  });

  // Convert canonical controls to Ardour bindings
  let bindingCount = 0;
  const globalChannel = 0; // Default to MIDI channel 1 (0-based)

  for (const control of canonicalMap.controls) {
    try {
      // Determine MIDI channel (canonical format uses 'global' or channel numbers)
      let channel = globalChannel;
      if (control.channel && typeof control.channel === 'number') {
        channel = control.channel - 1; // Convert to 0-based
      }

      const ardourFunction = convertToArdourFunction(control, canonicalMap);

      if (control.cc !== undefined) {
        // CC-based control
        ardourBuilder.addCCBinding({
          channel,
          controller: control.cc,
          function: ardourFunction,
          encoder: control.type === 'encoder',
          momentary: control.mode === 'momentary',
        });
        bindingCount++;
      } else if (control.note !== undefined) {
        // Note-based control (buttons)
        ardourBuilder.addNoteBinding({
          channel,
          note: control.note,
          function: ardourFunction,
          momentary: control.mode === 'momentary' || control.type === 'button',
        });
        bindingCount++;
      }

      // Handle button groups in canonical format
      if (control.buttons && control.buttons.length > 0) {
        for (const button of control.buttons) {
          let buttonChannel = globalChannel;
          if (button.channel && typeof button.channel === 'number') {
            buttonChannel = button.channel - 1;
          }

          const buttonFunction = convertToArdourFunction(button, canonicalMap);

          if (button.cc !== undefined) {
            ardourBuilder.addCCBinding({
              channel: buttonChannel,
              controller: button.cc,
              function: buttonFunction,
              momentary: button.mode === 'momentary',
            });
            bindingCount++;
          } else if (button.note !== undefined) {
            ardourBuilder.addNoteBinding({
              channel: buttonChannel,
              note: button.note,
              function: buttonFunction,
              momentary: button.mode === 'momentary' || button.type === 'button',
            });
            bindingCount++;
          }
        }
      }
    } catch (error: any) {
      console.warn(`⚠️  Skipping control ${control.id}: ${error.message}`);
    }
  }

  // Build and serialize the Ardour map
  const ardourMap = ardourBuilder.build();
  const serializer = new ArdourXMLSerializer({
    indent: '  ',
    newline: '\n'
  });

  const ardourXml = serializer.serializeMidiMap(ardourMap);
  writeFileSync(outputPath, ardourXml, 'utf-8');

  console.log(`🎛️  Generated ${bindingCount} Ardour bindings`);
}

/**
 * Convert canonical MIDI control to Ardour function string
 */
function convertToArdourFunction(control: any, canonicalMap: any): string {
  // Handle plugin parameter mapping
  if (control.plugin_parameter) {
    const paramIndex = control.plugin_parameter;

    // Special handling for standard parameters
    if (paramIndex === 'bypass') {
      return 'toggle-plugin-bypass';
    }
    if (paramIndex === 'window') {
      return 'toggle-plugin-window';
    }

    // Numeric plugin parameter
    if (!isNaN(parseInt(paramIndex))) {
      return `plugin-parameter[${paramIndex}]`;
    }
  }

  // Find matching mapping for this control (legacy format)
  const mapping = canonicalMap.mappings?.find(m => m.controlId === control.id);

  if (mapping) {
    // Use parameter-specific mapping if available
    if (mapping.parameterName) {
      const paramName = mapping.parameterName.toLowerCase();

      // Map common parameter types to Ardour functions
      if (paramName.includes('gain') || paramName.includes('level') || paramName.includes('volume')) {
        return 'track-set-gain[1]';
      } else if (paramName.includes('pan')) {
        return 'track-set-pan[1]';
      } else if (paramName.includes('mute')) {
        return 'toggle-track-mute[1]';
      } else if (paramName.includes('solo')) {
        return 'toggle-track-solo[1]';
      } else if (paramName.includes('bypass')) {
        return 'toggle-plugin-bypass';
      }
    }

    // Use parameter index as plugin parameter reference
    return `plugin-parameter[${mapping.parameterIndex}]`;
  }

  // Fallback: map by control type and name
  const controlName = control.name.toLowerCase();
  const controlType = control.type;

  // Transport controls
  if (controlName.includes('play')) return 'transport-roll';
  if (controlName.includes('stop')) return 'transport-stop';
  if (controlName.includes('record')) return 'toggle-rec-enable';

  // Track controls
  if (controlName.includes('gain') || controlName.includes('volume')) {
    return 'track-set-gain[1]';
  }
  if (controlName.includes('pan')) return 'track-set-pan[1]';
  if (controlName.includes('mute')) return 'toggle-track-mute[1]';
  if (controlName.includes('solo')) return 'toggle-track-solo[1]';

  // Bank controls
  if (controlName.includes('bank') && controlName.includes('next')) return 'next-bank';
  if (controlName.includes('bank') && controlName.includes('prev')) return 'prev-bank';

  // Generic mapping by control type
  if (controlType === 'fader' || controlType === 'slider') {
    return 'track-set-gain[1]';
  }
  if (controlType === 'knob' || controlType === 'encoder') {
    return 'track-set-send-gain[1,1]';
  }
  if (controlType === 'button') {
    return 'track-select[1]';
  }

  // Default fallback
  return 'track-select[1]';
}

function findMapFilesRecursively(dir: string, files: string[] = []): string[] {
  const entries = readdirSync(dir);

  for (const entry of entries) {
    const fullPath = join(dir, entry);
    const stats = statSync(fullPath);

    if (stats.isDirectory()) {
      findMapFilesRecursively(fullPath, files);
    } else if (['.yaml', '.yml', '.json'].includes(extname(entry).toLowerCase())) {
      files.push(fullPath);
    }
  }

  return files;
}

function generateForTarget(target: string, inputDir: string, outputDir: string, force: boolean): number {
  console.log(`🎛️  Generating ${target} maps...`);

  if (!existsSync(outputDir)) {
    mkdirSync(outputDir, { recursive: true });
  }

  const mapFiles = findMapFilesRecursively(inputDir);

  if (mapFiles.length === 0) {
    console.log(`⚠️  No canonical maps found in ${inputDir}`);
    return 0;
  }

  let generated = 0;

  for (const mapFilePath of mapFiles) {
    const inputPath = mapFilePath;
    const nameWithoutExt = basename(mapFilePath, extname(mapFilePath));

    let outputFile: string;
    switch (target) {
      case 'ardour':
        outputFile = `${nameWithoutExt}.map`;
        break;
      case 'ableton':
        outputFile = `${nameWithoutExt}.py`;
        break;
      case 'reaper':
        outputFile = `${nameWithoutExt}.reapercontrolsurface`;
        break;
      default:
        throw new Error(`Unsupported target: ${target}`);
    }

    const outputPath = join(outputDir, outputFile);

    if (existsSync(outputPath) && !force) {
      console.log(`⏭️  Skipping existing: ${outputFile}`);
      continue;
    }

    try {
      const content = readFileSync(inputPath, 'utf-8');
      let canonicalMap: any;

      if (extname(mapFilePath).toLowerCase() === '.json') {
        canonicalMap = JSON.parse(content);
      } else {
        // Parse YAML content
        canonicalMap = parseYAML(content) as CanonicalMidiMap;
      }

      // Skip files that don't have controls (like registry files)
      if (!canonicalMap.controls || !Array.isArray(canonicalMap.controls)) {
        console.log(`⏭️  Skipping non-map file: ${nameWithoutExt}`);
        continue;
      }

      switch (target) {
        case 'ardour':
          generateArdourMap(canonicalMap, outputPath);
          break;
        case 'ableton':
          throw new Error('Ableton Live generation not yet implemented');
        case 'reaper':
          throw new Error('REAPER generation not yet implemented');
      }

      console.log(`✅ Generated: ${outputFile}`);
      generated++;

    } catch (error: any) {
      console.error(`❌ Failed to generate ${outputFile}: ${error.message}`);
      console.error(`   Source: ${inputPath}`);
    }
  }

  return generated;
}

function installMapsForTarget(target: string, outputDir: string): boolean {
  const installPath = getInstallPath(target);

  if (!installPath) {
    console.warn(`⚠️  Could not find ${target} installation directory`);
    console.log(`💡 Install manually from: ${outputDir}`);
    return false;
  }

  if (!existsSync(installPath)) {
    console.warn(`⚠️  ${target} directory does not exist: ${installPath}`);
    return false;
  }

  try {
    console.log(`📂 Installing to: ${installPath}`);

    const files = readdirSync(outputDir);
    let installed = 0;

    for (const file of files) {
      const sourcePath = join(outputDir, file);
      const destPath = join(installPath, file);

      try {
        if (process.platform === 'win32') {
          execSync(`copy "${sourcePath}" "${destPath}"`, { stdio: 'inherit' });
        } else {
          execSync(`cp "${sourcePath}" "${destPath}"`, { stdio: 'inherit' });
        }
        console.log(`✅ Installed: ${file}`);
        installed++;
      } catch (error: any) {
        console.error(`❌ Failed to install ${file}: ${error.message}`);
      }
    }

    console.log(`📦 Installed ${installed} maps to ${target}`);
    return installed > 0;

  } catch (error: any) {
    console.error(`❌ Installation failed: ${error.message}`);
    return false;
  }
}

async function generateMaps(options: GenerateOptions): Promise<void> {
  console.log('🎛️  Starting DAW map generation...');

  const inputDir = options.input ? resolve(options.input) : findMapsDirectory();
  console.log(`📁 Input directory: ${inputDir}`);

  if (!existsSync(inputDir)) {
    throw new Error(`Input directory does not exist: ${inputDir}`);
  }

  const targets = options.target === 'all' ? ['ardour', 'ableton', 'reaper'] : [options.target!];
  console.log(`🎯 Targets: ${targets.join(', ')}`);

  let totalGenerated = 0;

  for (const target of targets) {
    try {
      const outputDir = options.output ? resolve(options.output) : getDefaultOutputDir(target);
      console.log(`📂 Output directory (${target}): ${outputDir}`);

      const generated = generateForTarget(target, inputDir, outputDir, options.force || false);
      totalGenerated += generated;

      if (options.install && generated > 0) {
        installMapsForTarget(target, outputDir);
      }

    } catch (error: any) {
      console.error(`❌ Failed to generate for ${target}: ${error.message}`);

      if (error.message.includes('not yet implemented')) {
        console.log('💡 This DAW target requires implementation');
        continue;
      }
      throw error;
    }
  }

  if (totalGenerated === 0) {
    console.log('⚠️  No maps were generated');
    throw new Error(
      'Map generation failed or no targets implemented.\n' +
      'Current implementation status:\n' +
      '• Ardour: Basic stub (needs XML generation)\n' +
      '• Ableton Live: Not implemented\n' +
      '• REAPER: Not implemented\n' +
      '\n' +
      'Implementation required in tools/daw/generate.ts'
    );
  }

  console.log(`✅ Generated ${totalGenerated} DAW maps successfully`);
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await generateMaps(options);
  } catch (error: any) {
    console.error('❌ DAW generation failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}