# Roland S-330 Front Panel Button SysEx Messages

## Overview

The S-330 sends SysEx messages when front-panel buttons are pressed. This is **undocumented** in the official Roland MIDI implementation and was discovered through hardware testing in January 2026.

## Physical Button Layout

The S-330 has the following physical buttons:

**Navigation buttons (D-pad):**
- Up, Down, Left, Right arrows

**Value buttons:**
- Inc (increment) - increases values, acts as "Yes" in boolean contexts
- Dec (decrement) - decreases values, acts as "No" in boolean contexts

**Function buttons:**
- MODE - Opens mode selection (PLAY, EDIT, DISK, FUNC, MIDI, UTIL)
- MENU - Opens context-sensitive submenu
- SUB MENU - Opens sub-menu (context dependent)
- COM (Command) - Opens command menu
- Execute - Confirms/executes selection

**Note:** The screen labels (Dec, Inc, exit, Sel, etc.) shown at the top of the display are clickable areas for the **optional mouse peripheral**, not physical buttons.

## Message Format

All front-panel button messages use the DT1 (Data Set) command:

```
F0 41 [dev] 1E 12 00 04 00 00 [cat] [code] [checksum] F7
```

| Byte | Value | Description |
|------|-------|-------------|
| F0 | - | SysEx Start |
| 41 | - | Roland Manufacturer ID |
| dev | 00-1F | Device ID |
| 1E | - | S-330 Model ID |
| 12 | - | DT1 (Data Set) command |
| 00 04 00 00 | - | Address (UI state area) |
| cat | 01 or 09 | Button category |
| code | varies | Button code |
| checksum | - | Roland checksum |
| F7 | - | SysEx End |

## Address Space

The address `00 04 00 00` is an **undocumented UI state area** not listed in the official S-330 MIDI implementation. This is separate from:
- `00 00 xx xx` - Patch parameters
- `00 01 xx xx` - Function parameters
- `00 02 xx xx` - Tone/MIDI parameters

## Button Categories

### Category `09` - Navigation/Value Buttons

These buttons send **two messages**: one on press, one on release.
- Press code: `00-05`
- Release code: Press code + `0x08`

| Button | Press | Release | Notes |
|--------|-------|---------|-------|
| Right Arrow | `09 00` | `09 08` | Cursor right |
| Left Arrow | `09 01` | `09 09` | Cursor left |
| Up Arrow | `09 02` | `09 0A` | Cursor up |
| Down Arrow | `09 03` | `09 0B` | Cursor down |
| Inc | `09 04` | `09 0C` | Increment value / Yes |
| Dec | `09 05` | `09 0D` | Decrement value / No |

### Category `01` - Menu Events and Function Buttons

Category `01` contains two ranges of codes:

#### Menu Navigation Events (`01 00-05`)

When navigation buttons are pressed **within a menu context**, an additional message is sent with category `01` before the normal `09` release message. These indicate menu-specific cursor movement.

| Event | Code | Data | Notes |
|-------|------|------|-------|
| Blocked/no-op | `00` | `01 00` | Nav button has no effect in current context |
| Menu right | `01` | `01 01` | Cursor right in menu (unconfirmed) |
| Menu up | `02` | `01 02` | Cursor up in menu |
| Menu down | `03` | `01 03` | Cursor down in menu |
| Menu inc | `04` | `01 04` | Increment in menu (unconfirmed) |
| Menu dec | `05` | `01 05` | Decrement in menu (unconfirmed) |

**Example sequence for DOWN in MODE menu:**
1. `01 03` - Menu cursor down event
2. `09 0B` - Down arrow release

#### Function Buttons (`01 0B-0F`)

These buttons send **one message** only (no separate press/release).

| Button | Code | Data | Notes |
|--------|------|------|-------|
| MODE | `0B` | `01 0B` | Opens mode selection menu |
| MENU | `0C` | `01 0C` | Opens context menu |
| SUB MENU | `0D` | `01 0D` | Opens sub-menu (sent even if disabled) |
| COM | `0E` | `01 0E` | Opens command menu |
| Execute | `0F` | `01 0F` | Confirms/executes selection |

**Note:** Function buttons send their code even when disabled or when the action has no effect.

## Example Messages

### Down Arrow Press
```
F0 41 00 1E 12 00 04 00 00 09 03 70 F7
```
Checksum: `128 - ((00+04+00+00+09+03) & 0x7F) = 128 - 16 = 112 = 0x70`

### Down Arrow Release
```
F0 41 00 1E 12 00 04 00 00 09 0B 68 F7
```
Checksum: `128 - ((00+04+00+00+09+0B) & 0x7F) = 128 - 24 = 104 = 0x68`

### MODE Button
```
F0 41 00 1E 12 00 04 00 00 01 0B 70 F7
```

### Execute Button
```
F0 41 00 1E 12 00 04 00 00 01 0F 6C F7
```

### Menu Navigation (DOWN in MODE menu)
```
F0 41 00 1E 12 00 04 00 00 01 03 78 F7  (menu cursor down)
F0 41 00 1E 12 00 04 00 00 09 0B 68 F7  (down arrow release)
```

## Timing

- Navigation buttons (category `09`): ~120-170ms between press and release messages
- Function buttons (category `01`): Single message only
- Menu navigation: `01` event followed by `09` release within ~40ms

## Potential Uses

1. **Remote monitoring**: Track user interactions with the hardware
2. **Automation**: Detect when user changes parameters on hardware
3. **Synchronization**: Keep software editor in sync with hardware state
4. **Macro recording**: Record button sequences for playback

## Remote Control (SUPPORTED!)

**Tested January 2026:** Sending these button messages TO the S-330 **works**! The S-330 accepts DT1 messages to address `00 04 00 00` and responds as if the physical button was pressed.

### Confirmed Working

| Button | Result |
|--------|--------|
| Down | Cursor moves down |
| Up | Cursor moves up |
| Right | Cursor moves right |
| Left | Cursor moves left |
| Inc | Value increments |
| Dec | Value decrements (assumed) |
| MODE | Opens mode menu (untested) |
| Execute | Confirms selection (untested) |

### Remote Control Message Format

Send DT1 with press code, wait ~150ms, send release code:

```
# Press DOWN
F0 41 00 1E 12 00 04 00 00 09 03 70 F7

# (wait 150ms)

# Release DOWN
F0 41 00 1E 12 00 04 00 00 09 0B 68 F7
```

### Potential Uses

This enables **full remote control** of the S-330 UI:
- Navigate menus programmatically
- Change parameter values
- Automate repetitive tasks
- Build software editors with bidirectional sync

## Unknown/Untested

The following have not yet been tested:
- Mouse clicks on screen labels (requires optional mouse peripheral)
- Other mode-specific button behaviors

## Hardware Testing Notes

- Tested on Roland S-330 with Device ID 0 (displays as 1)
- Messages captured via Volt 4 MIDI interface
- All tests performed in PLAY-Standard mode unless noted

## References

- [S-330 SysEx Documentation](./s330_sysex.md) - Main protocol documentation
- Roland S-330 Owner's Manual (1987)
