#!/usr/bin/env tsx
/**
 * Test Issue #36 with FULL custom mode data
 *
 * 1. Read a clean slot (10) to get valid mode data
 * 2. Try writing that data to different slots with/without pre-selection
 */

import midi from 'midi';
import { setTimeout as delay } from 'timers/promises';

const MIDI_OUT_PORT = 'LCXL3 1 MIDI Out';
const MIDI_IN_PORT = 'LCXL3 1 MIDI In';
const DAW_IN_PORT = 'LCXL3 1 DAW In';
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

async function main() {
  console.log('═══════════════════════════════════════════════════');
  console.log('Issue #36 - Full Mode Write Test');
  console.log('═══════════════════════════════════════════════════\n');

  const midiOutput = new midi.Output();
  const midiInput = new midi.Input();
  const dawOutput = new midi.Output();

  // CRITICAL: Enable SysEx
  midiInput.ignoreTypes(false, false, false);

  const midiOutPort = await findPort(midiOutput, MIDI_IN_PORT);
  const midiInPort = await findPort(midiInput, MIDI_OUT_PORT);
  const dawOutPort = await findPort(dawOutput, DAW_IN_PORT);

  midiOutput.openPort(midiOutPort);
  midiInput.openPort(midiInPort);
  dawOutput.openPort(dawOutPort);

  console.log('✓ Ports opened\n');

  let lastResponse: number[] | null = null;
  midiInput.on('message', (_deltaTime: number, message: number[]) => {
    lastResponse = Array.from(message);
  });

  // Step 1: Read slot 10 (page 0 and page 3)
  console.log('Step 1: Reading slot 10...');

  const page0Data: number[] = [];
  const page3Data: number[] = [];

  // Read page 0
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, 10));
  await delay(1000);

  if (lastResponse) {
    console.log(`  Page 0: ${lastResponse.length} bytes`);
    // Extract data between F0...F7 (skip header/footer)
    page0Data.push(...lastResponse.slice(10, -1));
  } else {
    console.log('  Page 0: No response!');
    process.exit(1);
  }

  // Read page 3
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(3, 10));
  await delay(1000);

  if (lastResponse) {
    console.log(`  Page 3: ${lastResponse.length} bytes`);
    page3Data.push(...lastResponse.slice(10, -1));
  } else {
    console.log('  Page 3: No response!');
    process.exit(1);
  }

  console.log(`\n✓ Read complete: ${page0Data.length} + ${page3Data.length} bytes of mode data\n`);

  // Step 2: Test writing to different slots
  const testSlots = [0, 1, 3, 5];

  for (const targetSlot of testSlots) {
    console.log(`\n${'='.repeat(60)}`);
    console.log(`Testing slot ${targetSlot}`);
    console.log('='.repeat(60));

    // Test A: Write without pre-selection
    console.log(`\nTest A: Write to slot ${targetSlot} WITHOUT pre-selection`);
    lastResponse = null;
    midiOutput.sendMessage(buildWriteMessage(0, targetSlot, ...page0Data));
    await delay(500);

    const writeNoPreStatus = lastResponse ? lastResponse[lastResponse.length - 2] : null;
    console.log(`  Page 0: ${lastResponse ? toHex(lastResponse) : 'TIMEOUT'}`);
    console.log(`  Status: ${writeNoPreStatus === 0x09 ? '✗ REJECTED (0x09)' : writeNoPreStatus ? `✓ ${writeNoPreStatus.toString(16)}` : 'TIMEOUT'}`);

    // Test B: Write WITH command 0x77 pre-selection
    console.log(`\nTest B: Write to slot ${targetSlot} WITH command 0x77 (100ms delay)`);
    midiOutput.sendMessage(buildTemplateChange(targetSlot));
    await delay(100);

    lastResponse = null;
    midiOutput.sendMessage(buildWriteMessage(0, targetSlot, ...page0Data));
    await delay(500);

    const write77Status = lastResponse ? lastResponse[lastResponse.length - 2] : null;
    console.log(`  Page 0: ${lastResponse ? toHex(lastResponse) : 'TIMEOUT'}`);
    console.log(`  Status: ${write77Status === 0x09 ? '✗ REJECTED (0x09)' : write77Status ? `✓ ${write77Status.toString(16)}` : 'TIMEOUT'}`);
  }

  console.log('\n\n═══════════════════════════════════════════════════');
  console.log('Test Complete');
  console.log('═══════════════════════════════════════════════════\n');

  midiOutput.closePort();
  midiInput.closePort();
  dawOutput.closePort();
}

main().catch(err => {
  console.error('Error:', err);
  process.exit(1);
});
