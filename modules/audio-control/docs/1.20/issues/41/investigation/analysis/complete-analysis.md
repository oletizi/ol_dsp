# Issue #41 Investigation: Page 3 Acknowledgement Mismatch

**Issue:** Multi-page writes fail on page 3 with status mismatch
- **Expected:** `0x12` (slot 4 encoding: `0x0E + 4 = 0x12`)
- **Received:** `0x0d` (decimal 13)
- **Last working:** v1.20.17
- **Broken:** v1.20.18-alpha.0

**Investigation Date:** 2025-10-17
**Investigator:** TypeScript Pro Agent

---

## Phase 1: Code Comparison (v1.20.17 vs Current)

### Acknowledgement Validation Code

**Result:** ✅ **NO CHANGES between v1.20.17 and current**

The `getExpectedStatusByte()` function and acknowledgement validation logic are **identical** in both versions:

```typescript
private getExpectedStatusByte(slot: number): number {
  if (slot >= 0 && slot <= 3) {
    return 0x06 + slot;
  } else if (slot >= 4 && slot <= 15) {
    return 0x0E + slot;
  }
  throw new Error(`Invalid slot number: ${slot} (must be 0-15)`);
}
```

**Conclusion:** This is NOT a regression caused by changes to acknowledgement validation code.

---

## Phase 2: Protocol Analysis

### Understanding Page Byte Mapping

According to `docs/PROTOCOL.md`:

**Write Protocol (Lines 252-255):**
| Logical Page | SysEx Page Byte | Control IDs |
|--------------|-----------------|-------------|
| 0            | 0x00            | 0x10-0x27 (16-39) |
| 1            | 0x03            | 0x28-0x3F (40-63) |

**Web Editor Example (Lines 340-364):**
```
Page 0 Write: F0 00 20 29 02 15 05 00 45 00 00 ...
Page 0 ACK:   F0 00 20 29 02 15 05 00 15 00 06 F7
                                       ↑  ↑  ↑
                                       │  │  └─ Status: 0x06 (slot 0)
                                       │  └─ Page: 0x00
                                       └─ ACK command: 0x15

Page 1 Write: F0 00 20 29 02 15 05 00 45 03 00 ...
Page 1 ACK:   F0 00 20 29 02 15 05 00 15 03 06 F7
                                       ↑  ↑  ↑
                                       │  │  └─ Status: 0x06 (slot 0)
                                       │  └─ Page: 0x03 (matches write!)
                                       └─ ACK command: 0x15
```

**Key Finding:** Acknowledgements echo back the SAME page byte as the write command.
- Write with page byte `0x00` → ACK with page `0x00`
- Write with page byte `0x03` → ACK with page `0x03`

### Current Implementation

**DeviceManager.ts lines 138-147:**
```typescript
// Send page 3 (only if there are controls in this range)
if (page3Controls.length > 0) {
  const page3Data = { ...validatedModeData, controls: page3Controls, labels: page3Labels };
  const page3Message = SysExParser.buildCustomModeWriteRequest(slot, 3, page3Data);
  await this.sendSysEx(page3Message);

  // Wait for page 3 acknowledgement
  // WORKAROUND: MIDI backend transport sometimes doesn't forward page 3 ACKs immediately
  // Device sends ACK within 24-80ms, but backend may delay delivery
  await this.waitForWriteAcknowledgement(3, slot, 2000); // Wait up to 2000ms for page 3 ACK
}
```

**Analysis:** The code correctly:
1. Builds write request with page `3` (which becomes SysEx byte `0x03`)
2. Waits for acknowledgement on page `3`
3. Expects status byte `0x12` for slot 4 (`0x0E + 4`)

---

## Phase 3: Status Byte Analysis

### Invalid Status Byte Received: `0x0d` (decimal 13)

**Expected for slot 4:** `0x12` (0x0E + 4 = 18 decimal)
**Received:** `0x0d` (13 decimal)

### Validity Check

**Slot encoding scheme (CC 30 protocol):**
- Slots 0-3:   `0x06 + slot` → Range: `0x06` to `0x09`
- Slots 4-15:  `0x0E + slot` → Range: `0x12` to `0x1D`

**Gap:** `0x0a` to `0x11` are NOT used (10-17 decimal)

**Analysis:** `0x0d` (13 decimal) falls in the **UNUSED GAP**!

```
Valid ranges:
0x06-0x09 (6-9):     Slots 0-3
0x0a-0x11 (10-17):   UNUSED (gap)
0x12-0x1D (18-29):   Slots 4-15

Received: 0x0d (13): FALLS IN UNUSED GAP!
```

**Conclusion:** `0x0d` is NOT a valid slot identifier according to the protocol.

---

## Phase 4: Root Cause Hypotheses

### Hypothesis 1: Parsing Bug

**Theory:** The parser is reading the wrong byte position as the status byte.

**Evidence:**
- Parsing code in `SysExParser.ts` line ~1068:
  ```typescript
  // Expected format after F0/F7 and manufacturer ID stripped:
  // 02 15 05 00 15 [PAGE] [STATUS]
  // Positions: 0  1  2  3  4     5      6
  const operation = data[4];
  const page = data[5] ?? 0;
  const status = data[6] ?? 0;
  ```

**Counter-evidence:**
- This parsing has worked correctly for page 0 acknowledgements
- Code hasn't changed between v1.20.17 and current
- Web editor examples show status is at correct position

**Likelihood:** Low

### Hypothesis 2: Device Firmware Bug

**Theory:** The device is actually sending `0x0d` in the status byte for page 3.

**Evidence:**
- `0x0d` doesn't match any known slot encoding
- Only happens on page 3, not page 0

**Counter-evidence:**
- Device firmware hasn't changed
- Web editor successfully writes to all slots
- Issue mentions "last working: v1.20.17"

**Likelihood:** Medium

### Hypothesis 3: MIDI Transport Message Corruption

**Theory:** The JUCE MIDI backend is corrupting page 3 acknowledgements.

**Evidence:**
- DeviceManager.ts line 145-147 comment mentions "MIDI backend transport sometimes doesn't forward page 3 ACKs immediately"
- Extended timeout to 2000ms suggests known backend issues
- Last working version might have had different timing

**Counter-evidence:**
- Why would backend corrupt the status byte specifically?
- Page byte seems to be parsed correctly (error message implies page 3 was matched)

**Likelihood:** High

### Hypothesis 4: Page 3 vs Page 1 Confusion

**Theory:** The acknowledgement might be coming back with page byte `0x01` instead of `0x03`.

**Evidence:**
- Protocol uses logical pages 0 and 1
- But SysEx bytes are 0x00 and 0x03
- Maybe device uses different page byte in ACK?

**Counter-evidence:**
- Web editor example clearly shows ACK page byte matches write page byte
- Code waits for page `3`, which should match SysEx byte `0x03`

**Likelihood:** Medium

---

## Phase 5: Recommended Next Steps

### Immediate Actions

1. **MIDI Traffic Capture** ✅ (directories created)
   - Run `midisnoop` during multi-page write
   - Capture exact bytes of page 3 acknowledgement
   - Verify actual page byte and status byte values

2. **Add Debug Logging**
   - Log raw acknowledgement bytes before parsing
   - Log parsed page and status values
   - Compare with expected values

3. **Test with Different Slots**
   - Try writing to slots 0-3 (different encoding)
   - Verify if issue is slot-specific or page-specific

### Investigation Questions

1. **What are the ACTUAL bytes in the page 3 acknowledgement?**
   - Full SysEx message with all bytes
   - After parseNovationMessage strips F0/F7 and manufacturer ID

2. **Is the page byte correct?**
   - Does it match `0x03` as expected?
   - Or is it something else (`0x01`)?

3. **Is the status byte really `0x0d`?**
   - Or is there an off-by-one error in parsing?
   - Could the device be sending a different format?

4. **Does v1.20.17 actually work?**
   - Test to confirm it was working
   - Check if issue existed but wasn't detected

---

## Phase 6: Temporary Workaround Options

If investigation shows this is a device/backend issue:

### Option 1: Relax Validation

```typescript
// Accept any acknowledgement as success (like page 0 does)
if (ack.status >= 0x06 && ack.status <= 0x1D) {
  // Valid slot identifier range
  pending.resolve();
} else {
  // Log warning but don't fail
  console.warn(`Unexpected status byte 0x${ack.status.toString(16)} for slot ${pending.slot}`);
  pending.resolve(); // Accept anyway
}
```

### Option 2: Disable Page 3 Status Validation

```typescript
// Only validate status for page 0, accept any status for page 3
if (ack.page === 0) {
  const expectedStatus = this.getExpectedStatusByte(pending.slot);
  if (ack.status !== expectedStatus) {
    pending.reject(...);
  }
}
// Always resolve for other pages
pending.resolve();
```

### Option 3: Retry Logic

```typescript
// If status mismatch, retry the page 3 write once
if (ack.status !== expectedStatus && retryCount < 1) {
  retryCount++;
  await this.sendSysEx(page3Message);
  await this.waitForWriteAcknowledgement(3, slot, 2000);
}
```

---

## Conclusion

**Primary Hypothesis:** MIDI backend transport is corrupting page 3 acknowledgements, resulting in invalid status byte `0x0d`.

**Evidence Strength:** Medium (based on known backend issues and comment in code)

**Next Step:** Capture actual MIDI traffic to confirm hypothesis and determine exact root cause.

**Status:** Awaiting MIDI capture and further investigation.

---

## Files Created

- `/Users/orion/work/ol_dsp/modules/audio-control/docs/1.20/issues/41/investigation/analysis/`
- `/Users/orion/work/ol_dsp/modules/audio-control/docs/1.20/issues/41/investigation/midi-captures/`

## Related Issues

- Issue #36: Write acknowledgement status byte discovery
- Issue #40: Mode name encoding format

## References

- `docs/PROTOCOL.md` lines 126-370 (Write protocol and acknowledgements)
- `src/device/DeviceManager.ts` lines 138-147 (Page 3 write implementation)
- `src/core/SysExParser.ts` lines ~1060-1080 (Acknowledgement parsing)
