#!/usr/bin/env tsx
/**
 * Round-trip validation test with DATA CHANGES for Issue #36
 *
 * 1. Read from slot 10 (known good)
 * 2. MODIFY the data (change labels, CC values)
 * 3. Write modified data to slot 3
 * 4. Read back from slot 3
 * 5. Verify read-back matches MODIFIED data (not original)
 */

import midi from 'midi';
import { setTimeout as delay } from 'timers/promises';
import { writeFileSync } from 'fs';

const MIDI_OUT_PORT = 'LCXL3 1 MIDI Out';
const MIDI_IN_PORT = 'LCXL3 1 MIDI In';
const MANUFACTURER_ID = [0x00, 0x20, 0x29];
const DEVICE_ID = 0x02;

function toHex(bytes: number[]): string {
  return bytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
}

function buildReadMessage(page: number, slot: number): number[] {
  return [
    0xF0, ...MANUFACTURER_ID, DEVICE_ID, 0x15, 0x05, 0x00, 0x40, page, slot, 0xF7
  ];
}

function buildWriteMessage(page: number, slot: number, ...data: number[]): number[] {
  return [
    0xF0, ...MANUFACTURER_ID, DEVICE_ID, 0x15, 0x05, 0x00, 0x45, page, slot, ...data, 0xF7
  ];
}

function buildTemplateChange(slot: number): number[] {
  return [0xF0, ...MANUFACTURER_ID, DEVICE_ID, 0x77, slot, 0xF7];
}

async function findPort(midiIO: any, exactName: string): Promise<number> {
  const count = midiIO.getPortCount();
  for (let i = 0; i < count; i++) {
    if (midiIO.getPortName(i) === exactName) {
      return i;
    }
  }
  throw new Error(`Port "${exactName}" not found`);
}

function modifyModeData(page0: number[], page3: number[]): { page0: number[], page3: number[] } {
  // Clone the data
  const modifiedPage0 = [...page0];
  const modifiedPage3 = [...page3];

  console.log('\nModifications being made:');
  console.log('─────────────────────────');

  // Modify mode name (bytes 1-8 in page 0, after slot byte)
  // Original name should be at bytes 1-8
  const originalName = String.fromCharCode(...modifiedPage0.slice(1, 9).filter(b => b !== 0));
  const newName = 'TEST123'; // 7 chars, will be padded with space
  const nameBytes = newName.split('').map(c => c.charCodeAt(0));
  nameBytes.push(0x20); // Add space to make 8 bytes

  for (let i = 0; i < 8; i++) {
    modifiedPage0[1 + i] = nameBytes[i] || 0x20;
  }
  console.log(`  Mode name: "${originalName}" → "${newName}"`);

  // Modify first control's CC value (scan for first control definition)
  // Controls start after mode name and color data
  // Looking for pattern: control_id, message_type, channel, ...
  // Let's modify CC values we can find

  let modificationsCount = 0;

  // Scan for CC values in control definitions (typically byte pattern: ... CC_NUM ...)
  // In the Launch Control XL3 format, CC numbers appear in predictable positions
  // Let's modify a few CC values by adding 10 to them (wrapping at 127)
  for (let i = 20; i < modifiedPage0.length - 10; i++) {
    const byte = modifiedPage0[i];
    // Look for bytes that look like CC numbers (reasonable range 0-119)
    if (byte >= 13 && byte <= 119 && modificationsCount < 3) {
      const newCC = Math.min(127, byte + 10);
      console.log(`  Byte ${i}: CC value ${byte} → ${newCC}`);
      modifiedPage0[i] = newCC;
      modificationsCount++;
    }
  }

  // Modify some bytes in page 3 as well (label data)
  // Find first text label and modify it
  for (let i = 0; i < modifiedPage3.length - 10; i++) {
    // Look for marker byte (0x60-0x7F range indicates label length)
    if (modifiedPage3[i] >= 0x60 && modifiedPage3[i] <= 0x6F) {
      const labelLength = modifiedPage3[i] - 0x60;
      if (labelLength > 0 && labelLength <= 8 && i + labelLength < modifiedPage3.length) {
        // Found a label, modify first character if it's ASCII
        const firstChar = modifiedPage3[i + 1];
        if (firstChar >= 0x41 && firstChar <= 0x5A) { // A-Z
          const originalLabel = String.fromCharCode(...modifiedPage3.slice(i + 1, i + 1 + labelLength));
          modifiedPage3[i + 1] = 0x58; // 'X'
          const newLabel = String.fromCharCode(...modifiedPage3.slice(i + 1, i + 1 + labelLength));
          console.log(`  Label: "${originalLabel}" → "${newLabel}"`);
          break; // Just modify one label
        }
      }
    }
  }

  return { page0: modifiedPage0, page3: modifiedPage3 };
}

function compareBytes(expected: number[], actual: number[]): { match: boolean; differences: string[] } {
  const differences: string[] = [];

  if (expected.length !== actual.length) {
    differences.push(`Length mismatch: expected=${expected.length}, actual=${actual.length}`);
  }

  const minLength = Math.min(expected.length, actual.length);
  for (let i = 0; i < minLength; i++) {
    if (expected[i] !== actual[i]) {
      differences.push(`Byte ${i}: expected 0x${expected[i].toString(16).padStart(2, '0')}, got 0x${actual[i].toString(16).padStart(2, '0')}`);
    }
  }

  return {
    match: differences.length === 0,
    differences
  };
}

async function main() {
  console.log('═══════════════════════════════════════════════════');
  console.log('Issue #36 - Round-Trip Test WITH DATA CHANGES');
  console.log('═══════════════════════════════════════════════════\n');

  const midiOutput = new midi.Output();
  const midiInput = new midi.Input();

  // CRITICAL: Enable SysEx
  midiInput.ignoreTypes(false, false, false);

  const midiOutPort = await findPort(midiOutput, MIDI_IN_PORT);
  const midiInPort = await findPort(midiInput, MIDI_OUT_PORT);

  midiOutput.openPort(midiOutPort);
  midiInput.openPort(midiInPort);

  console.log('✓ Ports opened\n');

  let lastResponse: number[] | null = null;
  midiInput.on('message', (_deltaTime: number, message: number[]) => {
    lastResponse = Array.from(message);
  });

  const sourceSlot = 10;
  const targetSlot = 3;

  // Step 1: Read from source slot (10)
  console.log(`Step 1: Reading from slot ${sourceSlot} (known good)...`);

  const sourcePage0: number[] = [];
  const sourcePage3: number[] = [];

  // Read page 0 from slot 10
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, sourceSlot));
  await delay(1000);

  if (!lastResponse) {
    console.log('✗ No response from device for page 0');
    process.exit(1);
  }

  sourcePage0.push(...lastResponse.slice(10, -1));
  console.log(`  Page 0: ${sourcePage0.length} bytes`);

  // Read page 3 from slot 10
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(3, sourceSlot));
  await delay(1000);

  if (!lastResponse) {
    console.log('✗ No response from device for page 3');
    process.exit(1);
  }

  sourcePage3.push(...lastResponse.slice(10, -1));
  console.log(`  Page 3: ${sourcePage3.length} bytes`);
  console.log(`\n✓ Read complete: ${sourcePage0.length} + ${sourcePage3.length} bytes total\n`);

  // Step 2: Modify the data
  console.log('Step 2: Modifying data...');
  const { page0: modifiedPage0, page3: modifiedPage3 } = modifyModeData(sourcePage0, sourcePage3);
  console.log('\n✓ Modifications complete\n');

  // Step 3: Write modified data to target slot (3) using Novation protocol
  console.log(`Step 3: Writing MODIFIED data to slot ${targetSlot}...`);

  // 3a. Write page 0
  console.log('  → Write page 0');
  lastResponse = null;
  midiOutput.sendMessage(buildWriteMessage(0, targetSlot, ...modifiedPage0));
  await delay(500);

  if (!lastResponse) {
    console.log('  ✗ No response for page 0 write');
  } else {
    console.log(`  ✓ Response: ${toHex(lastResponse)}`);
  }

  // 3b. Send command 0x77
  console.log('  → Send command 0x77');
  midiOutput.sendMessage(buildTemplateChange(targetSlot));
  await delay(100);

  // 3c. Write page 0 again (Novation protocol)
  console.log('  → Write page 0 again');
  lastResponse = null;
  midiOutput.sendMessage(buildWriteMessage(0, targetSlot, ...modifiedPage0));
  await delay(500);

  if (!lastResponse) {
    console.log('  ✗ No response for page 0 second write');
  } else {
    console.log(`  ✓ Response: ${toHex(lastResponse)}`);
  }

  // 3d. Write page 3
  console.log('  → Write page 3');
  lastResponse = null;
  midiOutput.sendMessage(buildWriteMessage(3, targetSlot, ...modifiedPage3));
  await delay(500);

  if (!lastResponse) {
    console.log('  ✗ No response for page 3 write');
  } else {
    console.log(`  ✓ Response: ${toHex(lastResponse)}`);
  }

  console.log('\n✓ Write sequence complete\n');

  // Wait for device to finish processing writes
  console.log('Waiting for device to settle...');
  await delay(1000);

  // Step 4: Read back from target slot (3)
  console.log(`Step 4: Reading back from slot ${targetSlot}...`);

  // IMPORTANT: Select the target slot before reading
  console.log('  → Select target slot with command 0x77');
  midiOutput.sendMessage(buildTemplateChange(targetSlot));
  await delay(200);

  const readPage0: number[] = [];
  const readPage3: number[] = [];

  // Read page 0 from slot 3
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, targetSlot));
  await delay(1000);

  if (!lastResponse) {
    console.log('✗ No response from device for page 0 read-back');
    process.exit(1);
  }

  readPage0.push(...lastResponse.slice(10, -1));
  console.log(`  Page 0: ${readPage0.length} bytes`);

  // Read page 3 from slot 3
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(3, targetSlot));
  await delay(1000);

  if (!lastResponse) {
    console.log('✗ No response from device for page 3 read-back');
    process.exit(1);
  }

  readPage3.push(...lastResponse.slice(10, -1));
  console.log(`  Page 3: ${readPage3.length} bytes`);
  console.log(`\n✓ Read-back complete: ${readPage0.length} + ${readPage3.length} bytes total\n`);

  // Step 5: Compare read-back with MODIFIED data (not original)
  console.log('Step 5: Comparing read-back with MODIFIED data...\n');

  const page0Comparison = compareBytes(modifiedPage0, readPage0);
  const page3Comparison = compareBytes(modifiedPage3, readPage3);

  console.log('Page 0 Comparison (Modified vs Read-back):');
  console.log('───────────────────────────────────────────');
  if (page0Comparison.match) {
    console.log('✓ PERFECT MATCH - All modified bytes preserved');
  } else {
    console.log(`✗ MISMATCH - ${page0Comparison.differences.length} differences found:`);
    page0Comparison.differences.slice(0, 20).forEach(diff => console.log(`  ${diff}`));
    if (page0Comparison.differences.length > 20) {
      console.log(`  ... and ${page0Comparison.differences.length - 20} more`);
    }
  }

  console.log('\nPage 3 Comparison (Modified vs Read-back):');
  console.log('───────────────────────────────────────────');
  if (page3Comparison.match) {
    console.log('✓ PERFECT MATCH - All modified bytes preserved');
  } else {
    console.log(`✗ MISMATCH - ${page3Comparison.differences.length} differences found:`);
    page3Comparison.differences.slice(0, 20).forEach(diff => console.log(`  ${diff}`));
    if (page3Comparison.differences.length > 20) {
      console.log(`  ... and ${page3Comparison.differences.length - 20} more`);
    }
  }

  // Also compare with original to show we actually changed something
  const page0OriginalComparison = compareBytes(sourcePage0, readPage0);
  const page3OriginalComparison = compareBytes(sourcePage3, readPage3);

  console.log('\n\nVerification that data WAS actually modified:');
  console.log('──────────────────────────────────────────────────');
  console.log(`Page 0: ${page0OriginalComparison.differences.length} differences from original`);
  console.log(`Page 3: ${page3OriginalComparison.differences.length} differences from original`);

  // Overall result
  const overallMatch = page0Comparison.match && page3Comparison.match;
  const dataWasModified = page0OriginalComparison.differences.length > 0 || page3OriginalComparison.differences.length > 0;

  console.log('\n═══════════════════════════════════════════════════');
  console.log('RESULT');
  console.log('═══════════════════════════════════════════════════');

  if (overallMatch && dataWasModified) {
    console.log('✓ TEST PASSED');
    console.log(`  Modified data written to slot ${targetSlot}`);
    console.log('  Read-back matches modified data perfectly');
    console.log('  Data modifications were successfully preserved');
  } else if (!dataWasModified) {
    console.log('⚠ TEST INCONCLUSIVE');
    console.log('  Data was not actually modified (modification logic failed)');
  } else {
    console.log('✗ TEST FAILED');
    console.log('  Modified data was NOT preserved correctly');
    console.log('  Device may have rejected or corrupted the changes');
  }
  console.log('═══════════════════════════════════════════════════\n');

  // Save results
  const results = {
    timestamp: new Date().toISOString(),
    sourceSlot,
    targetSlot,
    modifiedPage0Length: modifiedPage0.length,
    modifiedPage3Length: modifiedPage3.length,
    readPage0Length: readPage0.length,
    readPage3Length: readPage3.length,
    page0MatchesModified: page0Comparison.match,
    page3MatchesModified: page3Comparison.match,
    overallMatch,
    dataWasModified,
    page0DifferencesFromModified: page0Comparison.differences,
    page3DifferencesFromModified: page3Comparison.differences,
    page0DifferencesFromOriginal: page0OriginalComparison.differences.length,
    page3DifferencesFromOriginal: page3OriginalComparison.differences.length
  };

  const resultsPath = `/Users/orion/work/ol_dsp/modules/audio-control/tmp/round-trip-with-changes-${Date.now()}.json`;
  writeFileSync(resultsPath, JSON.stringify(results, null, 2));
  console.log(`Results saved to: ${resultsPath}\n`);

  midiOutput.closePort();
  midiInput.closePort();
}

main().catch(err => {
  console.error('Error:', err);
  process.exit(1);
});
