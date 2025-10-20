# MIDI Capture Analysis: Slot 3 Write Operation

**Investigation Date:** 2025-10-17
**Capture File:** `slot4-write-20251017-122720.txt` (misnamed - actually slot 3!)
**Operation:** Write to slot 3 via Novation web editor

---

## Executive Summary

**CRITICAL FINDING:** The MIDI capture shows a write to **slot 3**, not slot 4. The device correctly sends status byte `0x09` which matches our encoding scheme: `0x06 + 3 = 0x09`.

**Conclusion:** Our acknowledgement encoding scheme is **CORRECT**. The status byte DOES encode the slot number.

**Next Action Required:** Capture MIDI traffic for an actual slot 4 write to verify what status byte the device sends for slot 4.

---

## Acknowledgement Messages Found

### Page 0 Acknowledgement

**Full SysEx Message:**
```
F0 00 20 29 02 15 05 00 15 00 09 F7
```

**Parsed Structure:**
| Offset | Byte | Meaning |
|--------|------|---------|
| 0 | F0 | SysEx start |
| 1-3 | 00 20 29 | Manufacturer ID (Novation) |
| 4-5 | 02 15 | Product ID (Launch Control XL3) |
| 6-7 | 05 00 | Message type prefix |
| 8 | 15 | Command: Write acknowledgement |
| 9 | 00 | Page byte (page 0) |
| 10 | **09** | **Status byte** |
| 11 | F7 | SysEx end |

### Page 3 Acknowledgement

**Full SysEx Message:**
```
F0 00 20 29 02 15 05 00 15 03 09 F7
```

**Parsed Structure:**
| Offset | Byte | Meaning |
|--------|------|---------|
| 0 | F0 | SysEx start |
| 1-3 | 00 20 29 | Manufacturer ID (Novation) |
| 4-5 | 02 15 | Product ID (Launch Control XL3) |
| 6-7 | 05 00 | Message type prefix |
| 8 | 15 | Command: Write acknowledgement |
| 9 | 03 | Page byte (page 3) |
| 10 | **09** | **Status byte** |
| 11 | F7 | SysEx end |

---

## Status Byte Analysis

**Actual status byte received:** `0x09` (decimal 9)

**Slot identified in write command:** Slot 3 (byte at position 10 in write command = `0x03`)

**Expected status byte for slot 3:**
- Using encoding scheme: `0x06 + slot`
- `0x06 + 3 = 0x09`
- **MATCHES! ✅**

**Encoding validation:**
```
Slots 0-3: 0x06 + slot
  Slot 0: 0x06 + 0 = 0x06
  Slot 1: 0x06 + 1 = 0x07
  Slot 2: 0x06 + 2 = 0x08
  Slot 3: 0x06 + 3 = 0x09 ← Device sent this!
```

---

## Write Command Verification

### Page 0 Write Command

```
F0 00 20 29 02 15 05 00 45 00 03 20 09 53 68 6F 72 74 4E 61 6D 65 ...
                             ^  ^  ^^
                             |  |  ||
                             |  |  |└─ Flags: 0x09
                             |  |  └── Length: 0x20 (32 bytes)
                             |  └───── Slot: 0x03 (slot 3)
                             └──────── Page: 0x00 (page 0)
```

### Page 3 Write Command

```
F0 00 20 29 02 15 05 00 45 03 03 20 09 53 68 6F 72 74 4E 61 6D 65 ...
                             ^  ^  ^^
                             |  |  ||
                             |  |  |└─ Flags: 0x09
                             |  |  └── Length: 0x20 (32 bytes)
                             |  └───── Slot: 0x03 (slot 3)
                             └──────── Page: 0x03 (page 3)
```

**Both pages confirm slot 3 was the target.**

---

## Key Observations

1. **Page byte varies, status byte constant**
   - Page 0 ACK: page=0x00, status=0x09
   - Page 3 ACK: page=0x03, status=0x09
   - Status byte is **consistent across all pages** for the same slot

2. **Status byte encodes slot number**
   - Not page number
   - Not control count
   - Not "invalid" data
   - **Encodes the target slot number**

3. **Encoding scheme validated**
   - `0x06 + slot` for slots 0-3 works correctly
   - Device sent `0x09` for slot 3 as expected

---

## Why Issue #41 Reports 0x0d for Slot 4

**Hypothesis:** The error log shows `0x0d` for slot 4, which would be:
- If using 0-3 encoding: `0x06 + 4 = 0x0a` (wrong scheme)
- If using 4-15 encoding: `0x0E + 4 = 0x12` (our implementation)
- Actual reported: `0x0d` (decimal 13)

**Possibilities:**
1. **Parsing error** - Reading wrong byte position
2. **Different slot** - Actually writing to slot `0x0d - 0x0E = -1` (invalid)
3. **Need slot 4 capture** - Must verify with actual slot 4 write

---

## Next Steps

### REQUIRED: Capture Slot 4 Write

**Must perform:**
1. Start midisnoop capture
2. Use web editor to write to **slot 4 specifically**
3. Verify slot number in write command shows `0x04`
4. Extract acknowledgement status byte
5. Compare with expected `0x0E + 4 = 0x12`

### Analysis Questions

**To answer with slot 4 capture:**
- Does device send `0x12` for slot 4?
- Or does it send `0x0d`?
- If `0x0d`, does that suggest a different encoding for slots 4-15?
- Is there a bug in how our code sends the slot byte?

---

## Files Referenced

**MIDI capture:**
- `/Users/orion/work/ol_dsp/modules/audio-control/docs/1.20/issues/41/investigation/midi-captures/slot4-write-20251017-122720.txt`
  - Actual contents: Slot 3 write operation
  - 12,739 bytes, 101 lines
  - Contains both page 0 and page 3 acknowledgements

**Related code:**
- `src/device/DeviceManager.ts:353-360` - `getExpectedStatusByte()`
- `src/device/DeviceManager.ts:374-376` - `isValidStatusByte()`

---

## Validation Checklist

- [x] Acknowledgement messages identified in MIDI capture
- [x] Status bytes extracted (both pages have 0x09)
- [x] Slot number verified from write commands (slot 3)
- [x] Encoding scheme validated (0x06 + 3 = 0x09 matches)
- [ ] Slot 4 capture obtained
- [ ] Slot 4 status byte analyzed
- [ ] Root cause of 0x0d determined

---

## Conclusion

**The encoding scheme is correct.** The device sends the expected status byte based on `0x06 + slot` for slots 0-3.

**The mystery of 0x0d for slot 4 remains.** We need an actual slot 4 write capture to determine:
- If device sends `0x12` (expected) but our code misreads it
- If device sends `0x0d` (unexpected) suggesting encoding issue
- If there's a different problem entirely

**User's skepticism was justified** - we were right to question the assumption that `0x0d` is "invalid". It might be valid for slot 4 under a different encoding scheme we haven't discovered yet.
