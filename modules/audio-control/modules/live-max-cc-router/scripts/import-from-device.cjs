#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const yaml = require('yaml');

/**
 * Import canonical mapping from Launch Control XL3 custom mode
 *
 * Uses the launch-control-xl3 backup utility to fetch a mode from device,
 * then uses AI to match controls to plugin parameters.
 *
 * Usage: node import-from-device.cjs <slot> <descriptor.json> <output.yaml>
 *
 * Requirements:
 *   - JUCE MIDI server running (cd ../launch-control-xl3 && pnpm env:juce-server)
 *   - Launch Control XL 3 connected via USB
 *
 * Arguments:
 *   slot            - Custom mode slot number (1-15, physical slot number)
 *   descriptor.json - Plugin descriptor file path
 *   output.yaml     - Output canonical mapping file path
 */

async function importFromDevice(slot, descriptorPath, outputPath) {
  console.log('\n╔══════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                    IMPORT MAPPING FROM DEVICE                                    ║');
  console.log('╚══════════════════════════════════════════════════════════════════════════════════╝\n');

  const lcxl3Path = path.join(__dirname, '../../launch-control-xl3');
  const backupScript = path.join(lcxl3Path, 'utils/backup-slot.ts');

  // Check if JUCE server is running
  console.log('🔍 Checking JUCE MIDI server...\n');
  try {
    execSync('curl -s http://localhost:7777/health', { stdio: 'pipe' });
    console.log('✓ JUCE MIDI server is running\n');
  } catch (error) {
    console.error('❌ JUCE MIDI server not running!');
    console.error('\nPlease start the server:');
    console.error('  cd ../launch-control-xl3 && pnpm env:juce-server');
    console.error('\nOr check environment:');
    console.error('  cd ../launch-control-xl3 && pnpm env:check\n');
    process.exit(1);
  }

  // Fetch mode from device using backup utility
  console.log(`📡 Fetching mode from device slot ${slot}...\n`);

  const tempBackup = `/tmp/lcxl3-import-${Date.now()}.json`;

  try {
    // Run backup utility
    execSync(`cd ${lcxl3Path} && tsx ${backupScript} ${slot} ${tempBackup}`, {
      stdio: 'inherit'
    });

    if (!fs.existsSync(tempBackup)) {
      throw new Error('Backup file was not created');
    }

    console.log(`\n✓ Mode fetched successfully\n`);

  } catch (error) {
    console.error('\n❌ Failed to fetch mode from device');
    console.error('Error:', error.message);
    console.error('\nMake sure:');
    console.error('  1. Launch Control XL 3 is connected via USB');
    console.error('  2. Device is powered on');
    console.error('  3. JUCE MIDI server is running\n');
    process.exit(1);
  }

  // Read the fetched mode
  const backup = JSON.parse(fs.readFileSync(tempBackup, 'utf8'));
  const mode = backup.mode;

  console.log(`Mode: ${mode.name || 'Unnamed'}`);
  console.log(`Controls: ${Object.keys(mode.controls || {}).length}\n`);

  // Extract control information
  const controls = extractControls(mode);
  console.log(`Extracted ${controls.length} controls with labels\n`);

  // Load plugin descriptor
  if (!fs.existsSync(descriptorPath)) {
    throw new Error(`Plugin descriptor not found: ${descriptorPath}`);
  }

  const descriptor = JSON.parse(fs.readFileSync(descriptorPath, 'utf8'));
  console.log(`Plugin: ${descriptor.pluginName || 'Unknown'}\n`);

  // Use AI to match controls to parameters
  console.log('🤖 Using Claude AI to match controls to parameters...\n');
  const mappings = await matchControlsToParameters(controls, descriptor);

  console.log(`✓ Matched ${mappings.length} controls to parameters\n`);

  // Generate canonical mapping YAML
  const canonicalMapping = generateCanonicalMapping(mode, descriptor, mappings);

  // Write output
  fs.writeFileSync(outputPath, yaml.stringify(canonicalMapping, {
    lineWidth: 0,
    defaultStringType: 'QUOTE_DOUBLE',
  }), 'utf8');

  console.log(`✅ Canonical mapping saved to: ${outputPath}\n`);

  // Clean up temp file
  fs.unlinkSync(tempBackup);
}

function extractControls(mode) {
  const controls = [];
  const modeControls = mode.controls || {};

  // Extract controls with their labels
  for (const [controlId, control] of Object.entries(modeControls)) {
    // Use control.name for label (comes from loadCustomMode)
    const label = control.name || controlId;

    controls.push({
      id: controlId,
      name: label,
      cc: control.ccNumber,
      channel: control.midiChannel,
      type: getControlType(control.controlType),
      behavior: control.behavior || 'absolute'
    });
  }

  // Sort by CC number for consistency
  controls.sort((a, b) => a.cc - b.cc);

  return controls;
}

function getControlType(typeCode) {
  // Map control type codes to canonical types
  const typeMap = {
    0x00: 'encoder',    // Rotary encoder
    0x01: 'button',     // Button
    0x02: 'fader',      // Slider/fader
    0x05: 'encoder'     // Encoder (alternate)
  };
  return typeMap[typeCode] || 'unknown';
}

async function matchControlsToParameters(controls, descriptor) {
  // Build prompt for Claude
  const controlList = controls.map((c, i) =>
    `${i + 1}. ${c.name} (CC ${c.cc})`
  ).join('\n');

  const paramList = descriptor.parameters
    .map(p => `${p.index}: ${p.name}`)
    .join('\n');

  const prompt = `Match MIDI controller names to plugin parameters.

CONTROLLER CONTROLS:
${controlList}

PLUGIN PARAMETERS:
${paramList}

Please match each control to its best corresponding plugin parameter index. Consider:
- Common abbreviations (Comp = Compressor, Limit = Limiter)
- Synonyms (Dry/Wet = Mix, Enable = In)
- Semantic equivalence

Respond with ONLY a mapping table (one line per control):
1 -> [index]
2 -> [index]
...`;

  // Find claude executable
  const possiblePaths = [
    process.env.HOME + '/.claude/local/claude',
    '/usr/local/bin/claude',
    '/opt/homebrew/bin/claude',
    '/usr/bin/claude'
  ];

  let claudePath = null;
  for (const p of possiblePaths) {
    if (fs.existsSync(p)) {
      claudePath = p;
      break;
    }
  }

  if (!claudePath) {
    throw new Error('Claude CLI not found. Install from: https://github.com/anthropics/claude-code');
  }

  // Call Claude
  const response = execSync(`"${claudePath}" "${prompt}"`, {
    encoding: 'utf8',
    maxBuffer: 10 * 1024 * 1024,
    stdio: ['pipe', 'pipe', 'pipe']
  });

  // Parse response
  const lines = response.trim().split('\n');
  const mappings = [];

  lines.forEach(line => {
    const match = line.match(/^(\d+)\s*->\s*(\d+)/);
    if (match) {
      const controlIdx = parseInt(match[1]) - 1;
      const paramIdx = parseInt(match[2]);

      if (controlIdx >= 0 && controlIdx < controls.length) {
        mappings.push({
          control: controls[controlIdx],
          paramIndex: paramIdx
        });
      }
    }
  });

  return mappings;
}

function generateCanonicalMapping(mode, descriptor, mappings) {
  const mapping = {
    version: '1.0.0',
    device: {
      manufacturer: 'Novation',
      model: 'Launch Control XL 3',
      firmware: '>=1.0'
    },
    metadata: {
      name: `${descriptor.manufacturer || 'Unknown'} ${descriptor.pluginName || 'Unknown'} Mapping`,
      description: `Imported from device custom mode: ${mode.name || 'Unnamed'}`,
      author: 'Auto-generated',
      date: new Date().toISOString().split('T')[0],
      tags: ['auto-generated', 'imported']
    },
    plugin: {
      manufacturer: descriptor.manufacturer || 'Unknown',
      name: descriptor.pluginName || 'Unknown',
      type: descriptor.pluginType || 'VST3',
      version: '>=1.0'
    },
    midi_channel_registry: '../midi-channel-registry.yaml',
    controls: []
  };

  // Convert mappings to canonical format
  mappings.forEach((m, i) => {
    const control = {
      id: `control_${i + 1}`,
      name: m.control.name,
      type: m.control.type,
      cc: m.control.cc,
      channel: 'global',
      range: [0, 127],
      description: `Control: ${m.control.name}`,
      plugin_parameter: m.paramIndex.toString()
    };

    if (m.control.type === 'encoder') {
      control.behavior = m.control.behavior;
    }

    mapping.controls.push(control);
  });

  return mapping;
}

// Main
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.length < 3) {
    console.error('Usage: node import-from-device.cjs <slot> <descriptor.json> <output.yaml>');
    console.error('\nRequirements:');
    console.error('  - JUCE MIDI server running (cd ../launch-control-xl3 && pnpm env:juce-server)');
    console.error('  - Launch Control XL 3 connected via USB');
    console.error('\nArguments:');
    console.error('  slot            - Custom mode slot number (1-15, physical slot)');
    console.error('  descriptor.json - Plugin descriptor file path');
    console.error('  output.yaml     - Output canonical mapping file path');
    console.error('\nExample:');
    console.error('  node import-from-device.cjs 1 \\');
    console.error('    ../canonical-midi-maps/plugin-descriptors/analogobsession-channev.json \\');
    console.error('    ../canonical-midi-maps/maps/novation-launch-control-xl-3/channel-strips/my-mapping.yaml');
    process.exit(1);
  }

  const [slotStr, descriptorPath, outputPath] = args;
  const slot = parseInt(slotStr);

  if (isNaN(slot) || slot < 1 || slot > 15) {
    console.error('Error: Slot must be a number between 1 and 15 (physical slot number)');
    process.exit(1);
  }

  importFromDevice(slot, descriptorPath, outputPath)
    .catch(err => {
      console.error('\nFatal error:', err.message);
      process.exit(1);
    });
}

module.exports = { importFromDevice };
