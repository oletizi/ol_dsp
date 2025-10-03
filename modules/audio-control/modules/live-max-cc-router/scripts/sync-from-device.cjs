#!/usr/bin/env node

const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

/**
 * One-click device sync workflow
 *
 * Complete workflow to sync mappings from Launch Control XL 3:
 * 1. Import from device (update canonical mapping)
 * 2. Convert to internal formats
 * 3. Deploy to Ableton Live
 * 4. Generate Ardour mappings
 *
 * Usage: node sync-from-device.cjs <slot> <descriptor.json> <canonical-mapping.yaml>
 *
 * Requirements:
 *   - JUCE MIDI server running (cd ../launch-control-xl3 && pnpm env:juce-server)
 *   - Launch Control XL 3 connected via USB
 *
 * Arguments:
 *   slot               - Custom mode slot number (1-15, physical slot number)
 *   descriptor.json    - Plugin descriptor file path
 *   canonical-mapping.yaml - Canonical mapping file to update
 */

function runCommand(description, command, options = {}) {
  console.log(`\n${'═'.repeat(85)}`);
  console.log(`  ${description}`);
  console.log(`${'═'.repeat(85)}\n`);

  try {
    execSync(command, {
      stdio: 'inherit',
      ...options
    });
    console.log(`\n✅ ${description} - Complete\n`);
    return true;
  } catch (error) {
    console.error(`\n❌ ${description} - Failed`);
    console.error(`Error: ${error.message}\n`);
    return false;
  }
}

async function syncFromDevice(slot, descriptorPath, canonicalMappingPath) {
  console.log('\n╔═════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                         SYNC FROM DEVICE WORKFLOW                               ║');
  console.log('╚═════════════════════════════════════════════════════════════════════════════════╝\n');

  console.log(`📋 Configuration:`);
  console.log(`   Device Slot:       ${slot}`);
  console.log(`   Plugin Descriptor: ${descriptorPath}`);
  console.log(`   Canonical Mapping: ${canonicalMappingPath}`);
  console.log(``);

  // Step 1: Import from device
  if (!runCommand(
    'Step 1/4: Import from Device',
    `node ${__dirname}/import-from-device.cjs ${slot} ${descriptorPath} ${canonicalMappingPath}`
  )) {
    process.exit(1);
  }

  // Step 2: Convert maps to internal formats
  if (!runCommand(
    'Step 2/4: Convert Canonical Maps',
    'pnpm run convert-maps'
  )) {
    console.warn('⚠️  Map conversion failed - continuing anyway...');
  }

  // Step 3: Deploy to Ableton Live
  if (!runCommand(
    'Step 3/4: Deploy to Ableton Live',
    'pnpm run deploy'
  )) {
    console.warn('⚠️  Live deployment failed - continuing anyway...');
  }

  // Step 4: Generate Ardour mappings
  console.log(`\n${'═'.repeat(85)}`);
  console.log(`  Step 4/4: Generate Ardour Mappings`);
  console.log(`${'═'.repeat(85)}\n`);

  // Check if ardour-midi-maps module exists
  const ardourPath = path.join(__dirname, '../../ardour-midi-maps');
  if (fs.existsSync(ardourPath)) {
    if (!runCommand(
      'Generate Ardour MIDI Maps (all mappings)',
      `cd ${ardourPath} && pnpm run generate:install`
    )) {
      console.warn('⚠️  Ardour mapping generation failed - continuing anyway...');
    }
  } else {
    console.log('ℹ️  Ardour MIDI maps module not found - skipping Ardour sync');
    console.log('   Install ardour-midi-maps to enable Ardour synchronization\n');
  }

  // Summary
  console.log('\n╔═════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                             SYNC COMPLETE                                       ║');
  console.log('╚═════════════════════════════════════════════════════════════════════════════════╝\n');

  console.log('✅ Device sync workflow completed successfully!\n');
  console.log('Next steps:');
  console.log('  - Launch Ableton Live - new mappings are deployed');
  console.log('  - Check Ardour MIDI maps if applicable\n');
}

// Main
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.length < 3) {
    console.error('Usage: node sync-from-device.cjs <slot> <descriptor.json> <canonical-mapping.yaml>');
    console.error('');
    console.error('Requirements:');
    console.error('  - JUCE MIDI server running (cd ../launch-control-xl3 && pnpm env:juce-server)');
    console.error('  - Launch Control XL 3 connected via USB');
    console.error('');
    console.error('Arguments:');
    console.error('  slot               - Custom mode slot number (1-15, physical slot)');
    console.error('  descriptor.json    - Plugin descriptor file path');
    console.error('  canonical-mapping.yaml - Canonical mapping file to update');
    console.error('');
    console.error('Example:');
    console.error('  node sync-from-device.cjs 1 \\');
    console.error('    ../canonical-midi-maps/plugin-descriptors/analogobsession-channev.json \\');
    console.error('    ../canonical-midi-maps/maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml');
    process.exit(1);
  }

  const [slotStr, descriptorPath, canonicalMappingPath] = args;
  const slot = parseInt(slotStr);

  if (isNaN(slot) || slot < 1 || slot > 15) {
    console.error('Error: Slot must be a number between 1 and 15 (physical slot number)');
    process.exit(1);
  }

  syncFromDevice(slot, descriptorPath, canonicalMappingPath)
    .catch(err => {
      console.error('\nFatal error:', err.message);
      process.exit(1);
    });
}

module.exports = { syncFromDevice };
