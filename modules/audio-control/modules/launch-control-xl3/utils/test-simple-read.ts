#!/usr/bin/env tsx
import midi from 'midi';
import { setTimeout as delay } from 'timers/promises';

const MIDI_OUT_PORT = 'LCXL3 1 MIDI Out';
const MIDI_IN_PORT = 'LCXL3 1 MIDI In';
const MANUFACTURER_ID = [0x00, 0x20, 0x29];
const DEVICE_ID = 0x02;

function buildReadMessage(page: number, slot: number): number[] {
  return [0xF0, ...MANUFACTURER_ID, DEVICE_ID, 0x15, 0x05, 0x00, 0x40, page, slot, 0xF7];
}

function buildTemplateChange(slot: number): number[] {
  return [0xF0, ...MANUFACTURER_ID, DEVICE_ID, 0x77, slot, 0xF7];
}

async function findPort(midiIO: any, exactName: string): Promise<number> {
  const count = midiIO.getPortCount();
  for (let i = 0; i < count; i++) {
    if (midiIO.getPortName(i) === exactName) return i;
  }
  throw new Error(`Port "${exactName}" not found`);
}

async function main() {
  const midiOutput = new midi.Output();
  const midiInput = new midi.Input();
  midiInput.ignoreTypes(false, false, false);

  midiOutput.openPort(await findPort(midiOutput, MIDI_IN_PORT));
  midiInput.openPort(await findPort(midiInput, MIDI_OUT_PORT));

  let lastResponse: number[] | null = null;
  midiInput.on('message', (_: number, message: number[]) => {
    lastResponse = Array.from(message);
  });

  console.log('Test 1: Read from slot 10');
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, 10));
  await delay(1000);
  console.log(lastResponse ? `✓ Got ${lastResponse.length} bytes` : '✗ No response');

  console.log('\nTest 2: Send 0x77 to slot 3');
  midiOutput.sendMessage(buildTemplateChange(3));
  await delay(200);

  console.log('\nTest 3: Read from slot 3');
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, 3));
  await delay(1000);
  console.log(lastResponse ? `✓ Got ${lastResponse.length} bytes` : '✗ No response');

  console.log('\nTest 4: Read from slot 10 again');
  lastResponse = null;
  midiOutput.sendMessage(buildReadMessage(0, 10));
  await delay(1000);
  console.log(lastResponse ? `✓ Got ${lastResponse.length} bytes` : '✗ No response');

  midiOutput.closePort();
  midiInput.closePort();
}

main().catch(console.error);
