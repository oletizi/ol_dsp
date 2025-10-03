#!/usr/bin/env npx tsx
/**
 * Backup Custom Mode from Specified Physical Slot
 *
 * Reads a custom mode from the specified physical slot (1-15) and saves it to JSON.
 * Used by import-from-device.cjs to fetch device configurations.
 *
 * Usage: tsx backup-slot.ts <slot> <output.json>
 */

import { writeFileSync } from 'fs';
import { LaunchControlXL3 } from '../src';
import { JuceMidiBackend } from '../src/backends/JuceMidiBackend';

async function backupSlot(physicalSlot: number, outputPath: string) {
  console.log(`📖 Reading mode from device physical slot ${physicalSlot}...`);

  // Convert physical slot (1-15) to API slot (0-14)
  const apiSlot = physicalSlot - 1;

  // Create device with JUCE backend
  const backend = new JuceMidiBackend({ host: 'localhost', port: 7777 });
  const device = new LaunchControlXL3({ midiBackend: backend });

  try {
    await device.connect();

    const currentMode = await device.loadCustomMode(apiSlot);

    if (!currentMode) {
      throw new Error(`No mode found in physical slot ${physicalSlot}`);
    }

    console.log(`✓ Successfully read mode: "${currentMode.name}"`);
    console.log(`  Controls: ${Object.keys(currentMode.controls).length}`);

    // Extract labels from control.name properties
    const labels: Record<string, string> = {};
    for (const [controlId, control] of Object.entries(currentMode.controls)) {
      if (control.name) {
        labels[controlId] = control.name;
      }
    }

    // Store in JSON format
    const jsonData = {
      metadata: {
        physicalSlot,
        apiSlot,
        timestamp: new Date().toISOString(),
        deviceInfo: device.isConnected() ? await device.verifyDevice() : null
      },
      mode: {
        name: currentMode.name,
        controls: Object.fromEntries(
          Object.entries(currentMode.controls).map(([id, control]) => [
            id,
            {
              controlType: control.controlType,
              midiChannel: control.midiChannel,
              ccNumber: control.ccNumber,
              behavior: control.behavior,
              name: control.name, // Include the label
              ...control
            }
          ])
        ),
        labels,
        metadata: currentMode.metadata
      }
    };

    writeFileSync(outputPath, JSON.stringify(jsonData, null, 2));
    console.log(`💾 Mode backup saved: ${outputPath}`);

  } catch (error) {
    console.error('❌ Backup failed:', error);
    throw error;
  } finally {
    await device.disconnect();
    await backend.close();
  }
}

// Parse command line arguments
const args = process.argv.slice(2);

if (args.length < 2) {
  console.error('Usage: tsx backup-slot.ts <slot> <output.json>');
  console.error('');
  console.error('Arguments:');
  console.error('  slot        - Physical slot number (1-15)');
  console.error('  output.json - Output file path');
  console.error('');
  console.error('Example:');
  console.error('  tsx backup-slot.ts 1 /tmp/slot1.json');
  process.exit(1);
}

const physicalSlot = parseInt(args[0]);
const outputPath = args[1];

if (isNaN(physicalSlot) || physicalSlot < 1 || physicalSlot > 15) {
  console.error('Error: Slot must be a number between 1 and 15 (physical slot number)');
  process.exit(1);
}

// Run the backup
backupSlot(physicalSlot, outputPath).catch(error => {
  console.error('Fatal error:', error);
  process.exit(1);
});
