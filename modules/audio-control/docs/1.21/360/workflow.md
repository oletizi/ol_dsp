# Feature 360 Workflow

**Version:** 1.21
**Last Updated:** 2025-10-11

## Overview

The 360 workflow provides a complete "round-trip" process for deploying MIDI controller configurations to multiple DAWs. This document describes the end-to-end workflow from hardware interrogation through DAW deployment.

## Complete Workflow

```
┌────────────────────────────────────────────────────────────────────────┐
│                  Feature 360 Complete Workflow                         │
└────────────────────────────────────────────────────────────────────────┘

Phase 1: PLUGIN INTERROGATION          Phase 2: CANONICAL MAPPING
─────────────────────────────          ──────────────────────────
1. Extract plugin parameters           4. Read controller configuration
2. Generate plugin descriptors         5. Convert to canonical format
3. Categorize and cache                6. Save canonical YAML (optional)

Phase 3: DAW DEPLOYMENT
────────────────────────
7. Deploy to Ardour (XML)
8. Deploy to Live (JSON)
9. Install to DAW directories
10. Verify deployment
```

## Phase 1: Plugin Interrogation

### Purpose

Extract accurate parameter information from audio plugins to ensure mappings use real, valid parameter indices.

### Prerequisites

- JUCE-based plughost binary (built from project)
- Installed VST3 plugins
- canonical-midi-maps module

### Process

#### Step 1: Build Plugin Host

```bash
# From project root
cd path/to/juce-plughost
make
# Binary output: build/plughost
```

#### Step 2: Run Batch Interrogation

```bash
# Navigate to canonical-midi-maps
cd modules/audio-control/modules/canonical-midi-maps

# Run batch plugin scanning
pnpm plugin:generate-batch
```

**What happens:**
1. Launches `plughost` with `--batch-interrogate` flag
2. Scans all VST3 plugins in standard directories
3. Skips problematic plugins (UAD, etc.)
4. Extracts parameter information for each plugin
5. Outputs JSON stanzas to log file
6. Caches results for 24 hours

#### Step 3: Process Results

The batch generator automatically:
- Reads JSON stanzas from plughost output
- Converts to structured plugin descriptors
- Categorizes parameters (oscillator, filter, envelope, etc.)
- Saves individual descriptor files
- Updates catalog index

### Output

```
plugin-descriptors/
├── tal-togu-audio-line-tal-j-8.json      # Jupiter-8 emulation
├── arturia-jup-8-v4.json                  # Arturia version
├── analogobsession-channev.json           # Channel strip
└── plugins-catalog-batch.json             # Master catalog
```

### Example Plugin Descriptor

```json
{
  "plugin": {
    "manufacturer": "TAL-Togu Audio Line",
    "name": "TAL-J-8",
    "version": "1.0.0",
    "format": "VST3",
    "uid": "-664001267"
  },
  "metadata": {
    "parameter_count": 2234,
    "created": "2025-10-11T10:00:00Z",
    "author": "plughost-batch",
    "tags": ["synthesizer", "analog", "jupiter-8"]
  },
  "parameters": [
    {
      "index": 105,
      "name": "VCF Cutoff",
      "min": 0,
      "max": 1,
      "default": 0.5,
      "group": "filter",
      "type": "continuous",
      "automatable": true
    },
    {
      "index": 65,
      "name": "Env Attack",
      "min": 0,
      "max": 1,
      "default": 0.1,
      "group": "envelope",
      "type": "continuous",
      "automatable": true
    }
  ],
  "groups": {
    "filter": {
      "parameters": [105, 107, 109, 111]
    },
    "envelope": {
      "parameters": [65, 67, 69, 71]
    }
  }
}
```

## Phase 2: Canonical Mapping

### Purpose

Create device-specific MIDI controller → plugin parameter mappings using accurate parameter indices from Phase 1.

### Two Approaches

#### Approach A: Manual YAML Creation (Traditional)

1. Choose hardware controller (e.g., Launch Control XL 3)
2. Choose target plugin (e.g., TAL-J-8)
3. Reference plugin descriptor for parameter indices
4. Create YAML mapping file

#### Approach B: Hardware Extraction (360 Feature) ⭐

1. Create custom mode on hardware controller
2. Label controls with parameter names
3. Use `controller-deploy` to extract configuration
4. Automatically generate canonical mapping

### Manual YAML Creation

#### Step 1: Choose Target Combination

- **Hardware:** Novation Launch Control XL 3
- **Plugin:** TAL-J-8
- **Descriptor:** `plugin-descriptors/tal-togu-audio-line-tal-j-8.json`

#### Step 2: Create YAML File

**Location:** `maps/novation-launch-control-xl-3/roland-jupiter-8/tal-j8.yaml`

```yaml
version: 1.0.0

device:
  manufacturer: Novation
  model: Launch Control XL 3

plugin:
  manufacturer: TAL Software
  name: TAL-J-8
  descriptor: plugin-descriptors/tal-togu-audio-line-tal-j-8.json

metadata:
  name: Jupiter-8 Template
  description: Controls for TAL-J-8 synthesizer
  date: 2025-10-11
  author: Your Name
  tags: [synthesizer, jupiter-8]

midi_channel: 1

controls:
  # Row A Encoders - Filter Section
  - id: encoder_1
    name: VCF Cutoff
    type: encoder
    cc: 13
    channel: 1
    plugin_parameter: 105    # From descriptor: VCF Cutoff
    range: [0, 127]

  - id: encoder_2
    name: VCF Resonance
    type: encoder
    cc: 14
    plugin_parameter: 107    # From descriptor: VCF Resonance

  # Faders - Envelopes
  - id: slider_1
    name: Env Attack
    type: slider
    cc: 53
    plugin_parameter: 65     # From descriptor: Env Attack

  - id: slider_2
    name: Env Decay
    type: slider
    cc: 54
    plugin_parameter: 67     # From descriptor: Env Decay

  # Buttons - Toggles
  - id: button_1
    name: Filter Mode
    type: button
    cc: 41
    plugin_parameter: 112    # From descriptor: Filter Mode Toggle
```

### Hardware Extraction (One-Click Deployment) ⭐

#### Step 1: Create Custom Mode on Controller

Using Novation Components software:
1. Open Launch Control XL 3 editor
2. Create new custom mode in slot 0
3. Assign CC numbers to controls
4. **Label each control** with parameter name
5. Save to device

#### Step 2: Extract Configuration

```bash
# List available configurations
npx controller-deploy list

# Output:
# 📱 Detected: Novation Launch Control XL 3
#
# Available configurations:
#   Slot 0: Jupiter-8 Template
#   Slot 1: Minimoog Template
#   Slot 2: (empty)
#   ...
```

#### Step 3: Deploy to DAWs

```bash
# Deploy single DAW
npx controller-deploy deploy --slot 0 --daw ardour

# Deploy multiple DAWs
npx controller-deploy deploy --slot 0 --daw ardour live

# With plugin context
npx controller-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour live \
  --install

# Save canonical map for reference
npx controller-deploy deploy \
  --slot 0 \
  --output ./my-mappings \
  --install
```

**What happens:**
1. Reads custom mode from controller slot 0
2. Extracts control assignments and labels
3. Converts to canonical MIDI map format
4. Deploys to specified DAWs
5. Optionally saves canonical YAML

## Phase 3: DAW Deployment

### Purpose

Convert canonical mappings into DAW-specific formats and install to appropriate directories.

### Supported DAWs

| DAW | Format | Deployer | Status |
|-----|--------|----------|--------|
| Ardour | XML | ArdourDeployer | ✅ Complete |
| Ableton Live | JSON/M4L | LiveDeployer | ✅ Complete (Tier 2) |
| Reaper | ReaControlMIDI | (Future) | 📋 Planned |
| Bitwig | Controller Scripts | (Future) | 📋 Planned |

### Ardour Deployment

#### Automatic (via controller-deploy)

```bash
npx controller-deploy deploy --slot 0 --daw ardour --install
```

#### Manual (from canonical maps)

```bash
# Navigate to ardour-midi-maps
cd modules/audio-control/modules/ardour-midi-maps

# Generate and install
pnpm generate:install
```

**Output:**
- **File:** `dist/ardour-maps/novation-launch-control-xl-3.map`
- **Installed:** `~/Library/Preferences/Ardour8/midi_maps/` (macOS)

**Ardour XML Structure:**
```xml
<ArdourMIDIBindings version="1.0.0" name="LCXL3 - TAL-J-8">
  <DeviceInfo bank-size="8"/>

  <!-- Filter Controls -->
  <Binding channel="1" ctl="13" function="plugin-parameter"
           uri="TAL-J-8/param/105"/>  <!-- VCF Cutoff -->
  <Binding channel="1" ctl="14" function="plugin-parameter"
           uri="TAL-J-8/param/107"/>  <!-- VCF Resonance -->

  <!-- Envelope Controls -->
  <Binding channel="1" ctl="53" function="plugin-parameter"
           uri="TAL-J-8/param/65"/>   <!-- Attack -->
  <Binding channel="1" ctl="54" function="plugin-parameter"
           uri="TAL-J-8/param/67"/>   <!-- Decay -->
</ArdourMIDIBindings>
```

#### Using in Ardour

1. Open Ardour preferences
2. Navigate to Control Surfaces
3. Select "Generic MIDI"
4. Choose "Novation Launch Control XL 3 - TAL-J-8" mapping
5. Select MIDI input device
6. Click "Apply"

### Ableton Live Deployment (Dual-Pipeline)

#### Automatic (via controller-deploy)

```bash
npx controller-deploy deploy --slot 0 --daw live --install
```

**What happens:**
1. Converts canonical map to plugin mapping format
2. Writes to `live-max-cc-router/data/plugin-mappings.json`
3. Runtime loader merges with canonical mappings
4. Max for Live device uses merged mappings

**Dual-Pipeline Architecture:**

```
Tier 1: Canonical (Build-time)         Tier 2: Runtime (Device-extracted)
───────────────────────────            ──────────────────────────────────
Source: YAML files                      Source: Controller slot
Build:  convert-canonical-maps.cjs     Extract: controller-deploy
Output: canonical-plugin-maps.ts        Output: plugin-mappings.json

                    Runtime Merge
                    { ...canonical, ...runtime }
```

See [live-deployer/architecture.md](./live-deployer/architecture.md) for details.

#### Manual JSON Update

```json
// live-max-cc-router/data/plugin-mappings.json
{
  "version": "1.0.0",
  "lastUpdated": "2025-10-11T10:00:00Z",
  "mappings": {
    "launch-control-xl-3_tal-j-8": {
      "controller": {
        "manufacturer": "Novation",
        "model": "Launch Control XL 3"
      },
      "pluginName": "TAL-J-8",
      "pluginManufacturer": "TAL Software",
      "mappings": {
        "13": {
          "deviceIndex": 0,
          "parameterIndex": 105,
          "parameterName": "VCF Cutoff",
          "curve": "linear"
        },
        "53": {
          "deviceIndex": 0,
          "parameterIndex": 65,
          "parameterName": "Env Attack",
          "curve": "linear"
        }
      },
      "metadata": {
        "name": "Jupiter-8 Template",
        "source": "device-extracted",
        "controllerSlot": 0,
        "extractedAt": "2025-10-11T10:00:00Z"
      }
    }
  }
}
```

## Common Workflows

### Workflow 1: New Plugin Mapping (Manual)

```bash
# 1. Extract plugin parameters
cd modules/canonical-midi-maps
pnpm plugin:generate-batch

# 2. Create canonical mapping (edit YAML)
vim maps/novation-launch-control-xl-3/synth-category/my-plugin.yaml

# 3. Generate Ardour map
cd ../ardour-midi-maps
pnpm generate:install

# 4. Use in Ardour
# (Select in control surface preferences)
```

### Workflow 2: One-Click Hardware Extraction ⭐

```bash
# 1. Create custom mode on controller (use Novation Components)
# 2. Deploy in one command
npx controller-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour live \
  --install \
  --output ./my-mappings

# Done! Both DAWs configured from hardware
```

### Workflow 3: Update Existing Mapping

```bash
# 1. Edit canonical YAML
vim maps/novation-launch-control-xl-3/synth/my-plugin.yaml

# 2. Regenerate Ardour map
cd modules/ardour-midi-maps
pnpm generate:install

# 3. Live updates automatically (watches JSON file)
```

### Workflow 4: Multi-Plugin Session

```bash
# Deploy multiple plugins from controller slots
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour live
npx controller-deploy deploy --slot 1 --plugin "Mini V4" --daw ardour live
npx controller-deploy deploy --slot 2 --plugin "Jup-8 V4" --daw ardour live

# All three plugins now mapped from hardware
```

## CLI Reference

### controller-deploy list

List connected controllers and available configurations.

```bash
npx controller-deploy list
```

**Output:**
```
📱 Detected: Novation Launch Control XL 3

Available configurations:
  Slot 0: Jupiter-8 Template
  Slot 1: Minimoog Template
  Slot 2: (empty)
  Slot 3: Channel Strip
  ...
```

### controller-deploy deploy

Deploy controller configuration to DAW(s).

**Options:**
- `-c, --controller <type>`: Controller type (auto-detect if omitted)
- `-s, --slot <number>`: Configuration slot (default: 0)
- `-d, --daw <daws...>`: Target DAWs (default: ardour)
- `-p, --plugin <name>`: Plugin name for context
- `-m, --midi-channel <number>`: MIDI channel override
- `-o, --output <dir>`: Output directory for canonical YAML
- `--install`: Auto-install to DAW config directories
- `--dry-run`: Preview without deploying

**Examples:**
```bash
# Basic deployment
npx controller-deploy deploy --slot 0 --daw ardour

# Multiple DAWs
npx controller-deploy deploy --slot 0 --daw ardour live

# With plugin context
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour

# Save canonical and install
npx controller-deploy deploy --slot 0 --output ./mappings --install

# Preview only
npx controller-deploy deploy --slot 0 --daw ardour --dry-run
```

## Troubleshooting

### Plugin Interrogation Issues

**Problem:** Plugin crashes during scan
```bash
# Skip problematic plugins by editing
modules/canonical-midi-maps/src/tools/batch-plugin-generator.ts

const SKIP_PATTERNS = [
  /UAD/i,           # Universal Audio plugins
  /Waves/i,         # Waves plugins (if problematic)
  /YourPlugin/i     # Add problematic plugin
];
```

**Problem:** No parameters found
- Check plugin format (VST3 required)
- Verify plugin is installed correctly
- Try manual interrogation: `plughost --interrogate /path/to/plugin.vst3`

### Canonical Mapping Issues

**Problem:** Wrong parameter indices
- Verify plugin descriptor exists and is current
- Check parameter index in descriptor JSON
- Regenerate descriptor if plugin was updated

**Problem:** YAML validation errors
- Check YAML syntax (indentation, colons, quotes)
- Verify required fields (version, device, plugin, controls)
- Use YAML validator: `npx js-yaml --validate mapping.yaml`

### Deployment Issues

**Problem:** Ardour map not appearing
- Check installation path: `~/Library/Preferences/Ardour8/midi_maps/`
- Verify XML is valid: `xmllint --noout mapping.map`
- Restart Ardour after installation

**Problem:** Live mapping not working
- Check `plugin-mappings.json` syntax
- Verify runtime-loader is loading file
- Check Max for Live device logs
- Ensure plugin name matches exactly

**Problem:** Controller not detected
- Check MIDI connections (Web MIDI API requires Chrome/Edge)
- Verify controller is powered on
- Try manual adapter creation (bypass auto-detection)

## Performance Tips

### Cache Plugin Descriptors

Plugin interrogation is cached for 24 hours. To force refresh:
```bash
rm plugin-descriptors/plugins-catalog-batch.json
pnpm plugin:generate-batch
```

### Batch Deployments

Deploy multiple configurations in one script:
```bash
#!/bin/bash
for slot in {0..3}; do
  npx controller-deploy deploy --slot $slot --daw ardour live --install
done
```

### Validate Before Deploy

Use dry-run to verify before actual deployment:
```bash
npx controller-deploy deploy --slot 0 --daw ardour live --dry-run
# Review output, then run without --dry-run
npx controller-deploy deploy --slot 0 --daw ardour live --install
```

## Next Steps

- **[Architecture](./architecture.md)** - Understand system design
- **[Implementation Workplan](./implementation/workplan.md)** - Technical details
- **[Status](./status.md)** - Current implementation progress

---

**Questions?** See [README.md](./README.md) for navigation and resources.
