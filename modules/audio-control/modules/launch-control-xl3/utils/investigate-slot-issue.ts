#!/usr/bin/env node
/**
 * Investigation script for Issue #36 - Slot selection behavior
 *
 * This script tests MIDI communication with Launch Control XL3 to determine:
 * 1. What MIDI messages are sent when pressing slot buttons
 * 2. Whether writes work without explicit slot selection
 * 3. Whether command 0x77 enables writes to inactive slots
 */

import midi from 'midi';
import { setTimeout as delay } from 'timers/promises';
import { writeFile } from 'fs/promises';

const MIDI_OUT_PORT = 'LCXL3 1 MIDI Out';  // Source: receive FROM device
const MIDI_IN_PORT = 'LCXL3 1 MIDI In';    // Destination: send TO device
const DAW_OUT_PORT = 'LCXL3 1 DAW Out';    // DAW source: receive FROM device
const DAW_IN_PORT = 'LCXL3 1 DAW In';      // DAW destination: send TO device
const MANUFACTURER_ID = [0x00, 0x20, 0x29];
const DEVICE_ID = 0x02;

// Helper to format bytes as hex string
function toHex(bytes) {
  return bytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
}

// Helper to create custom mode read request
// Format: F0 00 20 29 02 15 05 00 40 [PAGE] [SLOT] F7
function buildReadMessage(page, slot) {
  return [
    0xF0,             // SysEx start
    ...MANUFACTURER_ID, // 00 20 29
    DEVICE_ID,        // 02
    0x15,             // Command (Custom mode)
    0x05,             // Sub-command
    0x00,             // Reserved
    0x40,             // Read operation
    page,             // Page number (0 or 3)
    slot,             // Slot number (0-14)
    0xF7              // SysEx end
  ];
}

// Helper to create custom mode write SysEx message
// Format: F0 00 20 29 02 15 05 00 45 [PAGE] [SLOT] [data] F7
function buildWriteMessage(page, slot, ...data) {
  return [
    0xF0,             // SysEx start
    ...MANUFACTURER_ID, // 00 20 29
    DEVICE_ID,        // 02
    0x15,             // Command (Custom mode)
    0x05,             // Sub-command
    0x00,             // Reserved
    0x45,             // Write operation with data
    page,             // Page number (0 or 3 for write)
    slot,             // Slot number (0-14)
    ...data,          // Data bytes
    0xF7              // SysEx end
  ];
}

// Helper to create template change SysEx message (command 0x77)
// Format: F0 00 20 29 02 77 [SLOT] F7
function buildTemplateChange(slot) {
  return [
    0xF0,             // SysEx start
    ...MANUFACTURER_ID, // 00 20 29
    DEVICE_ID,        // 02
    0x77,             // Template change command
    slot,             // Slot number
    0xF7              // SysEx end
  ];
}

// Helper to convert slot number to CC 30 value
// Based on official Novation documentation:
// Slots 0-3: CC values 6-9
// Slots 4-15: CC values 18-29
function slotToCC30Value(slot) {
  if (slot >= 0 && slot <= 3) {
    return 0x06 + slot;  // Slots 0-3: values 6-9
  } else if (slot >= 4 && slot <= 15) {
    return 0x0E + slot;  // Slots 4-15: values 18-29
  }
  throw new Error(`Invalid slot: ${slot} (must be 0-15)`);
}

// Helper to create CC 30 slot selection message
// Format: B6 1E [VALUE]
// B6 = Control Change on channel 7 (0-indexed: channel 6)
// 1E = CC number 30 (hex)
// VALUE = slot value from slotToCC30Value()
function buildCC30SlotSelect(slot) {
  const ccValue = slotToCC30Value(slot);
  return [0xB6, 0x1E, ccValue];
}

// Helper to create feature control enable message
// Format: 9F 0B 7F
// May be required before using feature controls
function buildFeatureControlEnable() {
  return [0x9F, 0x0B, 0x7F];
}

// Find device port by exact name
async function findPort(midiIO, exactName) {
  const count = midiIO.getPortCount();
  console.log(`  Searching ${count} ports for "${exactName}"...`);
  for (let i = 0; i < count; i++) {
    const portName = midiIO.getPortName(i);
    console.log(`    [${i}] ${portName}`);
    if (portName === exactName) {
      console.log(`  ✓ Found port "${exactName}" at index ${i}`);
      return i;
    }
  }
  throw new Error(`Port "${exactName}" not found. Available ports:\n${
    Array.from({ length: count }, (_, i) => `  ${i}: ${midiIO.getPortName(i)}`).join('\n')
  }`);
}

console.log('═══════════════════════════════════════════════════');
console.log('Launch Control XL3 - Issue #36 Investigation');
console.log('═══════════════════════════════════════════════════\n');

// TEST 1: Monitor slot button presses
async function test1_MonitorButtons() {
  console.log('TEST 1: Monitor Physical Slot Button Presses');
  console.log('──────────────────────────────────────────────');
  console.log('Instructions: Press slot buttons 0, 1, 3, 5 on the device');
  console.log('Monitoring for 30 seconds...\n');

  const input = new midi.Input();
  // CRITICAL: Enable SysEx messages (disabled by default in node-midi)
  input.ignoreTypes(false, false, false);
  const port = await findPort(input, MIDI_OUT_PORT);

  const captured = [];
  input.on('message', (deltaTime, message) => {
    const hex = toHex(message);
    const timestamp = new Date().toISOString();
    console.log(`${timestamp}: ${hex}`);
    captured.push({ timestamp, message: Array.from(message), hex });
  });

  input.openPort(port);
  console.log(`Listening on port ${port}: ${input.getPortName(port)}\n`);

  await delay(30000);

  input.closePort();

  // Save capture
  const captureFile = `/Users/orion/work/ol_dsp/modules/audio-control/tmp/slot-button-capture-${Date.now()}.json`;
  await writeFile(captureFile, JSON.stringify(captured, null, 2));

  console.log(`\nCapture saved to: ${captureFile}`);
  console.log(`Captured ${captured.length} MIDI messages\n`);

  return captured;
}

// TEST 2: Write without pre-selection
async function test2_ImplicitSelection() {
  console.log('\nTEST 2: Write Without Pre-selection (Implicit Selection)');
  console.log('──────────────────────────────────────────────────────────');

  const output = new midi.Output();
  const input = new midi.Input();
  // CRITICAL: Enable SysEx messages (disabled by default in node-midi)
  input.ignoreTypes(false, false, false);

  const outPort = await findPort(output, MIDI_IN_PORT);   // Send TO device
  const inPort = await findPort(input, MIDI_OUT_PORT);    // Receive FROM device

  output.openPort(outPort);
  console.log(`  ✓ Opened output port: ${output.getPortName(outPort)}`);
  input.openPort(inPort);
  console.log(`  ✓ Opened input port: ${input.getPortName(inPort)}`);

  const results = [];
  let lastResponse = null;

  input.on('message', (deltaTime, message) => {
    lastResponse = Array.from(message);
  });

  // Test writing to different slots without pre-selection
  const slots = [0, 1, 3, 5];

  for (const slot of slots) {
    console.log(`\nTesting write to slot ${slot} (no pre-selection)...`);
    lastResponse = null;

    // Use correct write format: F0 00 20 29 02 15 05 00 45 [PAGE] [SLOT] [data] F7
    const writeCmd = buildWriteMessage(0, slot, 0x00, 0x00, 0x00, 0x00);
    console.log(`  Sending: ${toHex(writeCmd)}`);
    output.sendMessage(writeCmd);
    console.log(`  → Message sent, waiting 500ms for response...`);
    await delay(500);
    console.log(`  → Wait complete`);

    if (lastResponse) {
      const statusByte = lastResponse[lastResponse.length - 2];
      const success = statusByte !== 0x09;
      console.log(`  Response: ${toHex(lastResponse)}`);
      console.log(`  Status: ${success ? '✓ SUCCESS' : '✗ FAILED (status 0x09)'}`);
      results.push({ slot, success, response: lastResponse, responseHex: toHex(lastResponse) });
    } else {
      console.log('  No response received');
      results.push({ slot, success: false, response: null, responseHex: 'TIMEOUT' });
    }
  }

  output.closePort();
  input.closePort();

  return results;
}

// TEST 3: Command 0x77 with varying delays
async function test3_Command0x77() {
  console.log('\nTEST 3: Command 0x77 Slot Selection with Delays');
  console.log('────────────────────────────────────────────────');

  const output = new midi.Output();
  const input = new midi.Input();
  // CRITICAL: Enable SysEx messages (disabled by default in node-midi)
  input.ignoreTypes(false, false, false);

  const outPort = await findPort(output, MIDI_IN_PORT);   // Send TO device
  const inPort = await findPort(input, MIDI_OUT_PORT);    // Receive FROM device

  output.openPort(outPort);
  console.log(`  ✓ Opened output port: ${output.getPortName(outPort)}`);
  input.openPort(inPort);
  console.log(`  ✓ Opened input port: ${input.getPortName(inPort)}`);

  const results = [];
  let lastResponse = null;

  input.on('message', (deltaTime, message) => {
    lastResponse = Array.from(message);
  });

  const delays = [0, 100, 200, 500];
  const targetSlot = 3;

  for (const delayMs of delays) {
    console.log(`\nTesting with ${delayMs}ms delay...`);

    // Send slot selection command using correct template change format
    const selectCmd = buildTemplateChange(targetSlot);
    console.log(`  Select: ${toHex(selectCmd)}`);
    output.sendMessage(selectCmd);

    await delay(delayMs);

    // Send write command using correct write format
    lastResponse = null;
    const writeCmd = buildWriteMessage(0, targetSlot, 0x00, 0x00, 0x00, 0x00);
    console.log(`  Write:  ${toHex(writeCmd)}`);
    output.sendMessage(writeCmd);

    await delay(500);

    if (lastResponse) {
      const statusByte = lastResponse[lastResponse.length - 2];
      const success = statusByte !== 0x09;
      console.log(`  Response: ${toHex(lastResponse)}`);
      console.log(`  Status: ${success ? '✓ SUCCESS' : '✗ FAILED (status 0x09)'}`);
      results.push({ delay: delayMs, success, response: lastResponse, responseHex: toHex(lastResponse) });
    } else {
      console.log('  No response received');
      results.push({ delay: delayMs, success: false, response: null, responseHex: 'TIMEOUT' });
    }
  }

  output.closePort();
  input.closePort();

  return results;
}

// TEST 4: CC 30 on DAW Port
async function test4_CC30Protocol() {
  console.log('\nTEST 4: CC 30 Slot Selection on DAW Port');
  console.log('──────────────────────────────────────────');
  console.log('Testing official Novation documented protocol\n');

  const midiOutput = new midi.Output();
  const midiInput = new midi.Input();
  // CRITICAL: Enable SysEx messages (disabled by default in node-midi)
  midiInput.ignoreTypes(false, false, false);
  const dawOutput = new midi.Output();

  const midiOutPort = await findPort(midiOutput, MIDI_IN_PORT);   // Send SysEx TO device
  const midiInPort = await findPort(midiInput, MIDI_OUT_PORT);    // Receive SysEx FROM device
  const dawOutPort = await findPort(dawOutput, DAW_IN_PORT);      // Send CC TO device

  midiOutput.openPort(midiOutPort);
  console.log(`  ✓ Opened MIDI output: ${midiOutput.getPortName(midiOutPort)}`);
  midiInput.openPort(midiInPort);
  console.log(`  ✓ Opened MIDI input: ${midiInput.getPortName(midiInPort)}`);
  dawOutput.openPort(dawOutPort);
  console.log(`  ✓ Opened DAW output: ${dawOutput.getPortName(dawOutPort)}\n`);

  const results = [];
  let lastResponse = null;

  midiInput.on('message', (deltaTime, message) => {
    lastResponse = Array.from(message);
  });

  // Phase 1: Try feature control enable
  console.log('Phase 1: Feature Control Enable');
  const enableMsg = buildFeatureControlEnable();
  console.log(`  Sending: ${toHex(enableMsg)} on DAW port`);
  dawOutput.sendMessage(enableMsg);
  await delay(200);
  console.log('  Feature control enable sent\n');

  // Phase 2: Test CC 30 + Write with varying delays
  const testSlots = [0, 1, 3, 5];
  const delays = [0, 100, 200];

  for (const slot of testSlots) {
    for (const delayMs of delays) {
      console.log(`Testing slot ${slot} with ${delayMs}ms delay...`);
      lastResponse = null;

      // Send CC 30 slot selection on DAW port
      const selectMsg = buildCC30SlotSelect(slot);
      console.log(`  CC 30:  ${toHex(selectMsg)} (slot ${slot} → CC value ${slotToCC30Value(slot)})`);
      dawOutput.sendMessage(selectMsg);

      await delay(delayMs);

      // Send write on MIDI port
      const writeMsg = buildWriteMessage(0, slot, 0x00, 0x00, 0x00, 0x00);
      console.log(`  Write:  ${toHex(writeMsg)}`);
      midiOutput.sendMessage(writeMsg);

      await delay(500);

      if (lastResponse) {
        const statusByte = lastResponse[lastResponse.length - 2];
        const success = statusByte !== 0x09;
        console.log(`  Response: ${toHex(lastResponse)}`);
        console.log(`  Status: ${success ? '✓ SUCCESS' : '✗ FAILED (status 0x09)'}\n`);
        results.push({
          slot,
          delay: delayMs,
          success,
          response: lastResponse,
          responseHex: toHex(lastResponse)
        });
      } else {
        console.log(`  No response received\n`);
        results.push({
          slot,
          delay: delayMs,
          success: false,
          response: null,
          responseHex: 'TIMEOUT'
        });
      }
    }
  }

  midiOutput.closePort();
  midiInput.closePort();
  dawOutput.closePort();

  return results;
}

// Main execution
async function main() {
  console.log('✓ main() function started - async operations will execute\n');
  try {
    // Skip Test 1 - requires manual button pressing
    const test1Results = [];
    console.log('TEST 1: Monitor Physical Slot Button Presses - SKIPPED');
    console.log('(Requires manual button interaction)\n');

    const test2Results = await test2_ImplicitSelection();
    const test3Results = await test3_Command0x77();
    const test4Results = await test4_CC30Protocol();

    // Generate summary report
    console.log('\n\n═══════════════════════════════════════════════════');
    console.log('INVESTIGATION SUMMARY');
    console.log('═══════════════════════════════════════════════════\n');

    console.log('Test 1: Physical Button Behavior');
    console.log(`  Messages captured: ${test1Results.length}`);
    if (test1Results.length > 0) {
      console.log('  Sample messages:');
      test1Results.slice(0, 5).forEach(m => console.log(`    ${m.hex}`));
    } else {
      console.log('  No MIDI messages detected from button presses');
    }

    console.log('\nTest 2: Implicit Selection Results');
    test2Results.forEach(r => {
      console.log(`  Slot ${r.slot}: ${r.success ? '✓' : '✗'} ${r.responseHex}`);
    });
    const implicitWorks = test2Results.some(r => r.success);
    console.log(`  Conclusion: Implicit selection ${implicitWorks ? 'WORKS' : 'DOES NOT WORK'}`);

    console.log('\nTest 3: Command 0x77 Results');
    test3Results.forEach(r => {
      console.log(`  ${r.delay}ms delay: ${r.success ? '✓' : '✗'} ${r.responseHex}`);
    });
    const cmd77Works = test3Results.some(r => r.success);
    console.log(`  Conclusion: Command 0x77 ${cmd77Works ? 'WORKS' : 'DOES NOT WORK'}`);
    if (cmd77Works) {
      const optimalDelay = test3Results.find(r => r.success)?.delay;
      console.log(`  Optimal delay: ${optimalDelay}ms`);
    }

    console.log('\nTest 4: CC 30 on DAW Port Results');
    console.log('  Testing official Novation documented protocol:');
    test4Results.forEach(r => {
      console.log(`  Slot ${r.slot}, ${r.delay}ms: ${r.success ? '✓' : '✗'} ${r.responseHex}`);
    });
    const cc30Works = test4Results.some(r => r.success);
    console.log(`  Conclusion: CC 30 protocol ${cc30Works ? 'WORKS' : 'DOES NOT WORK'}`);
    if (cc30Works) {
      const successfulTests = test4Results.filter(r => r.success);
      const minDelay = Math.min(...successfulTests.map(r => r.delay));
      console.log(`  Minimum delay: ${minDelay}ms`);
      console.log(`  Success rate: ${successfulTests.length}/${test4Results.length}`);
    }

    // Save complete results
    const fullResults = {
      timestamp: new Date().toISOString(),
      test1_buttonCapture: test1Results,
      test2_implicitSelection: test2Results,
      test3_command0x77: test3Results,
      test4_cc30Protocol: test4Results,
      conclusions: {
        buttonsSendMIDI: test1Results.length > 0,
        implicitSelectionWorks: implicitWorks,
        command0x77Works: cmd77Works,
        cc30ProtocolWorks: cc30Works,
        optimalDelay: cmd77Works ? test3Results.find(r => r.success)?.delay : null,
        cc30MinDelay: cc30Works ? Math.min(...test4Results.filter(r => r.success).map(r => r.delay)) : null,
        cc30SuccessRate: cc30Works ? `${test4Results.filter(r => r.success).length}/${test4Results.length}` : '0/0',
      }
    };

    const resultsFile = `/Users/orion/work/ol_dsp/modules/audio-control/tmp/investigation-results-${Date.now()}.json`;
    await writeFile(resultsFile, JSON.stringify(fullResults, null, 2));
    console.log(`\nFull results saved to: ${resultsFile}`);

  } catch (error) {
    console.error('\nError during investigation:', error.message);
    process.exit(1);
  }

  console.log('\n✓ main() function completed successfully');
}

main().catch(err => {
  console.error('Fatal error:', err);
  process.exit(1);
});
