# Device Sync Workflow

One-click synchronization from Launch Control XL 3 hardware to DAW mappings.

## Overview

The device sync workflow enables you to:
1. Configure a custom mode on your Launch Control XL 3 hardware
2. Automatically import those mappings into the canonical format
3. Deploy the mappings to all supported DAWs (Ableton Live, Ardour)

This eliminates manual mapping maintenance and ensures your hardware configuration is the source of truth.

## Requirements

### Hardware
- **Launch Control XL 3** connected via USB
- Device powered on
- Custom mode configured in desired slot (1-15)

### Software
- **JUCE MIDI Server** running (handles device communication)
- **Node.js 18+** (for running scripts)
- **pnpm 8+** (package manager)
- **Claude Code CLI** (for AI-powered parameter matching)

### Configuration Files
- **Plugin Descriptor** - JSON file describing the target plugin's parameters
- **Canonical Mapping** - YAML file to update (created if doesn't exist)

## Usage

### One-Click Sync

```bash
# Start JUCE MIDI server (in separate terminal)
cd ../launch-control-xl3
pnpm env:juce-server

# Run complete sync workflow
pnpm sync-from-device <slot> <descriptor.json> <canonical-mapping.yaml>
```

### Example

```bash
pnpm sync-from-device 1 \
  ../canonical-midi-maps/plugin-descriptors/analogobsession-channev.json \
  ../canonical-midi-maps/maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml
```

This will:
1. ✅ Fetch custom mode from device slot 1
2. ✅ Extract control labels and CC assignments
3. ✅ Use AI to match control names to plugin parameters
4. ✅ Update the canonical mapping with device configuration
5. ✅ Convert to internal Live formats
6. ✅ Deploy to Ableton Live Max for Live device
7. ⚠️ Generate Ardour mappings (pending implementation)

## Workflow Steps

### Step 1: Import from Device

**What it does:**
- Connects to Launch Control XL 3 via JUCE MIDI server
- Reads custom mode from specified slot (1-15)
- Extracts all 48 controls with their labels
- Uses Claude AI for semantic matching of control names to plugin parameters

**Input:**
- Device slot number (1-15, physical slot number)
- Plugin descriptor JSON (describes available parameters)
- Canonical mapping path (will be created or updated)

**Output:**
- Updated canonical YAML mapping with:
  - Device control labels
  - Matched plugin parameters
  - Preserved metadata
  - "device-synced" tag added

### Step 2: Convert Maps

**What it does:**
- Runs `pnpm run convert-maps`
- Converts canonical YAML to internal TypeScript format
- Generates optimized lookup structures for runtime

**Output:**
- `dist/mappings/*.ts` - TypeScript mapping modules
- Used by the CC router at runtime

### Step 3: Deploy to Live

**What it does:**
- Builds the TypeScript CC router
- Deploys to Ableton Live Max for Live directory
- Updates the installed device with new mappings

**Output:**
- Updated `cc-router.amxd` device
- New mappings available immediately in Live

### Step 4: Generate Ardour Maps (Pending)

**What it does:**
- Converts canonical mapping to Ardour XML format
- Deploys to Ardour MIDI maps directory

**Status:** Not yet implemented (see Implementation Status below)

## Plugin Descriptors

Plugin descriptors are JSON files that describe a plugin's parameters:

```json
{
  "manufacturer": "Analog Obsession",
  "pluginName": "CHANNEV",
  "pluginType": "VST3",
  "parameterCount": 45,
  "parameters": [
    {
      "index": 1,
      "name": "Mic Pre Gain",
      "type": "continuous",
      "range": [0, 127]
    },
    {
      "index": 2,
      "name": "Line Amp",
      "type": "continuous",
      "range": [0, 127]
    }
  ]
}
```

### Creating Plugin Descriptors

Use the plugin descriptor generator (if available) or create manually by:
1. Load the plugin in your DAW
2. Document each parameter name and index
3. Save as JSON in `../canonical-midi-maps/plugin-descriptors/`

## AI-Powered Matching

The workflow uses Claude AI to intelligently match control names to plugin parameters:

**Handles:**
- Common abbreviations (Comp → Compressor, Limit → Limiter)
- Synonyms (Dry/Wet → Mix, Enable → In)
- Semantic equivalence (High Pass → HPF, Low Freq → LF)

**Example Matching:**
```
Device Label      →  Plugin Parameter
"Mic Gain"        →  "Mic Pre Gain" (index 1)
"Line Amp"        →  "Line Amplifier Gain" (index 9)
"EQ On/Off"       →  "EQ Enable" (index 42)
```

## Canonical Mapping Updates

When syncing from device, the canonical mapping is updated while preserving:
- Version information
- Device specifications
- Plugin information
- Metadata (name, author, original tags)

**Added/Updated:**
- Description includes device sync timestamp
- Date updated to current date
- "device-synced" tag added
- All control mappings replaced with device configuration

**Example diff:**
```yaml
metadata:
  name: "Analog Obsession CHANNEV Mapping"
  description: "Professional channel strip plugin control (Updated from device: CHANNEVE)"
  date: "2025-10-03"
  tags:
    - "channel-strip"
    - "device-synced"  # <-- Added by sync
```

## Troubleshooting

### JUCE MIDI Server Not Running

```
❌ JUCE MIDI server not running!

Please start the server:
  cd ../launch-control-xl3 && pnpm env:juce-server
```

**Solution:** Start the JUCE server in a separate terminal before running sync.

### Device Not Connected

```
❌ Failed to fetch mode from device
Make sure:
  1. Launch Control XL 3 is connected via USB
  2. Device is powered on
  3. JUCE MIDI server is running
```

**Solution:**
1. Check USB connection
2. Power cycle the device
3. Verify device appears in system MIDI devices

### Claude CLI Not Found

```
Error: Claude CLI not found. Install from: https://github.com/anthropics/claude-code
```

**Solution:** Install Claude Code CLI:
```bash
# macOS
brew install claude-code

# Or download from GitHub releases
```

### Mapping Conflicts

If AI matching produces incorrect results, you can:
1. Manually edit the canonical mapping YAML
2. Use `pnpm verify-mapping:channev` to verify correctness
3. Use `pnpm fix-mapping:channev` to fix with AI assistance

## Manual Steps

### Import Only (No Deploy)

```bash
pnpm import-from-device <slot> <descriptor.json> <mapping.yaml>
```

Updates canonical mapping without deploying to DAWs.

### Verify Mapping

```bash
pnpm verify-mapping:channev
```

Checks if control names match plugin parameters semantically.

### Fix Mapping

```bash
pnpm fix-mapping:channev
```

Uses AI to correct any mismatched parameter assignments.

### Deploy Only

```bash
pnpm run deploy
```

Deploys current mappings to Ableton Live without fetching from device.

## Implementation Status

### ✅ Implemented
- Device interrogation via JUCE MIDI server
- Control label extraction from custom modes
- AI-powered semantic parameter matching
- Canonical mapping creation and updates
- Ableton Live Max for Live deployment

### ⚠️ Pending
- **Ardour MIDI map generation** - Requires:
  - Canonical → Ardour XML converter
  - `generate-from-canonical` script in ardour-midi-maps module
  - Ardour map deployment automation

### 🔮 Future Enhancements
- Support for other MIDI controllers (Akai, Behringer, etc.)
- Batch sync for multiple slots
- Preset management (save/restore hardware configurations)
- Validation against actual plugin (requires plugin loading)
- Control curve customization (linear, exponential, logarithmic)

## Advanced Usage

### Custom Workflows

The sync process can be customized by calling individual steps:

```bash
# Step 1: Import from device
pnpm import-from-device 1 descriptor.json mapping.yaml

# Step 2: Convert maps
pnpm run convert-maps

# Step 3: Deploy to Live
pnpm run deploy

# Custom: Generate Ardour maps (when implemented)
cd ../ardour-midi-maps
pnpm run generate-from-canonical ../canonical-midi-maps/maps/.../mapping.yaml
```

### Scripting

The sync scripts can be imported and used programmatically:

```javascript
const { syncFromDevice } = require('./scripts/sync-from-device.cjs');

await syncFromDevice(
  1,  // slot
  '../canonical-midi-maps/plugin-descriptors/my-plugin.json',
  '../canonical-midi-maps/maps/my-mapping.yaml'
);
```

## File Locations

### Source Files
- Scripts: `/modules/live-max-cc-router/scripts/`
- Canonical Maps: `/modules/canonical-midi-maps/maps/`
- Plugin Descriptors: `/modules/canonical-midi-maps/plugin-descriptors/`

### Generated Files
- TypeScript Mappings: `/modules/live-max-cc-router/dist/mappings/`
- Built Device: `/modules/live-max-cc-router/dist/cc-router.js`

### Deployed Files
- Max for Live Device: `~/Music/Ableton/User Library/Presets/Audio Effects/Max Audio Effect/cc-router/`
- Ardour Maps: `~/.config/ardour8/midi_maps/` (when implemented)

## Related Documentation

- [Launch Control XL3 Protocol](../../launch-control-xl3/docs/PROTOCOL.md)
- [Canonical MIDI Maps Format](../../canonical-midi-maps/README.md)
- [AI Verification](./AI-VERIFICATION.md)
- [Deployment Guide](./DEPLOYMENT.md)

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review error messages carefully
3. Verify all requirements are met
4. Check JUCE MIDI server logs
5. Test with a known-working configuration

---

**Last Updated:** 2025-10-03
**Version:** 1.0.0
**Status:** Production Ready (Ableton Live), Pending (Ardour)
