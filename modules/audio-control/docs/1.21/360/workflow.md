# Feature 360 Workflow

**Version:** 1.21
**Last Updated:** 2025-10-12

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

Phase 2.5: FUZZY PARAMETER MATCHING (AI-Powered) ⭐
────────────────────────────────────────────────────
6.5. Match control names → plugin parameters (Claude AI)

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
5. **AI-powered parameter matching** (Phase 2.5)

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

# With plugin context (triggers fuzzy matching)
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
4. **[Phase 2.5] AI-powered fuzzy matching** (when --plugin provided)
5. Deploys to specified DAWs
6. Optionally saves canonical YAML

## Phase 2.5: AI-Powered Fuzzy Parameter Matching ⭐

### Purpose

Automatically match human-readable control names from hardware (e.g., "VCF Cutoff", "Env Attack") to plugin parameter indices using Claude AI.

### When It Runs

Fuzzy matching is triggered when you provide the `--plugin` flag during deployment:

```bash
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
```

### How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│              Fuzzy Parameter Matching Workflow                  │
└─────────────────────────────────────────────────────────────────┘

1. Extract Control Names               2. Load Plugin Descriptor
   ↓                                      ↓
┌──────────────────┐                 ┌──────────────────┐
│ Hardware Labels  │                 │ Plugin Descriptor│
│ ────────────────│                 │ ────────────────│
│ • VCF Cutoff     │                 │ • param[105]:    │
│ • Env Attack     │                 │   "VCF Cutoff"   │
│ • Filter Res     │                 │ • param[65]:     │
│ • LFO Rate       │                 │   "Env Attack"   │
└──────────────────┘                 └──────────────────┘
         │                                    │
         └────────────────┬───────────────────┘
                          ↓
              ┌───────────────────────┐
              │  Claude AI Matcher    │
              │  ─────────────────────│
              │  Shell out to:        │
              │  • claude CLI, OR     │
              │  • Anthropic API      │
              └───────────────────────┘
                          ↓
              ┌───────────────────────┐
              │  Matched Parameters   │
              │  ─────────────────────│
              │  VCF Cutoff → 105     │
              │  Env Attack → 65      │
              │  Filter Res → 107     │
              │  LFO Rate → 89        │
              └───────────────────────┘
                          ↓
              ┌───────────────────────┐
              │  Enhanced Canonical   │
              │  ─────────────────────│
              │  controls:            │
              │    - id: encoder_1    │
              │      name: VCF Cutoff │
              │      cc: 13           │
              │      plugin_parameter:│
              │        105  ← ADDED   │
              └───────────────────────┘
```

### Before and After

#### Before Fuzzy Matching (Generic URIs)

```yaml
controls:
  - id: encoder_1
    name: VCF Cutoff        # From hardware
    cc: 13
    # No plugin_parameter field
```

**Ardour Output:**
```xml
<Binding channel="1" ctl="13" function="track-set-gain[1]"/>
<!-- Generic function, not plugin-specific -->
```

#### After Fuzzy Matching (Plugin-Specific URIs)

```yaml
controls:
  - id: encoder_1
    name: VCF Cutoff        # From hardware
    cc: 13
    plugin_parameter: 105    # ← Added by AI matcher
```

**Ardour Output:**
```xml
<Binding channel="1" ctl="13" function="plugin-parameter"
         uri="TAL-J-8/param/105"/>  <!-- VCF Cutoff -->
<!-- Plugin-specific parameter binding -->
```

### Integration Methods

#### Method 1: Claude CLI (Preferred)

```typescript
// Shell out to claude CLI
const result = await execAsync(
  `claude --prompt "Match these control names to plugin parameters: ${JSON.stringify(controls)}" --plugin-descriptor ${descriptorPath}`
);
```

**Requirements:**
- `claude` CLI installed and in PATH
- API key configured in environment

#### Method 2: Anthropic API (Direct)

```typescript
import Anthropic from '@anthropic-ai/sdk';

const anthropic = new Anthropic({
  apiKey: process.env.ANTHROPIC_API_KEY,
});

const message = await anthropic.messages.create({
  model: 'claude-3-5-sonnet-20241022',
  max_tokens: 1024,
  messages: [{
    role: 'user',
    content: `Match control names to plugin parameters:\n${prompt}`
  }],
});
```

### Matching Algorithm

Claude AI analyzes:
1. **Semantic similarity**: "VCF Cutoff" matches "VCF Cutoff" (exact)
2. **Abbreviations**: "Filter Res" matches "VCF Resonance"
3. **Synonyms**: "Env Attack" matches "Attack Time"
4. **Context**: Groups related parameters (filters, envelopes, etc.)
5. **Confidence scoring**: Returns match confidence for validation

### Example Prompt

```
You are a MIDI parameter matching expert. Match hardware control names to plugin parameters.

Hardware Controls:
- VCF Cutoff (CC 13)
- Filter Res (CC 14)
- Env Attack (CC 53)
- LFO Rate (CC 21)

Plugin Parameters (TAL-J-8):
- [105] VCF Cutoff
- [107] VCF Resonance
- [109] VCF Envelope Amount
- [65] Attack Time
- [67] Decay Time
- [89] LFO 1 Rate
- [91] LFO 1 Amount

Return JSON with matches:
{
  "matches": [
    { "control": "VCF Cutoff", "cc": 13, "parameter": 105, "confidence": 1.0 },
    { "control": "Filter Res", "cc": 14, "parameter": 107, "confidence": 0.95 },
    ...
  ]
}
```

### Output Format

```json
{
  "matches": [
    {
      "control_name": "VCF Cutoff",
      "control_cc": 13,
      "parameter_index": 105,
      "parameter_name": "VCF Cutoff",
      "confidence": 1.0,
      "match_type": "exact"
    },
    {
      "control_name": "Filter Res",
      "control_cc": 14,
      "parameter_index": 107,
      "parameter_name": "VCF Resonance",
      "confidence": 0.95,
      "match_type": "abbreviation"
    },
    {
      "control_name": "Env Attack",
      "control_cc": 53,
      "parameter_index": 65,
      "parameter_name": "Attack Time",
      "confidence": 0.9,
      "match_type": "synonym"
    }
  ],
  "unmatched": []
}
```

### Troubleshooting Fuzzy Matching

#### Problem: No matches found

**Causes:**
- Plugin descriptor not found
- Control names too generic ("Knob 1", "Encoder A")
- Plugin parameters unnamed or use internal IDs

**Solutions:**
```bash
# Verify plugin descriptor exists
ls plugin-descriptors/tal-togu-audio-line-tal-j-8.json

# Use more descriptive control names in hardware
# Good: "VCF Cutoff", "Resonance", "Attack"
# Bad: "Knob1", "Enc A", "Control 1"

# Manually specify parameter indices in YAML if AI fails
```

#### Problem: Wrong parameter matched

**Causes:**
- Ambiguous control names
- Multiple parameters with similar names
- Low confidence matches

**Solutions:**
```bash
# Review match confidence scores
# Confidence < 0.7 may indicate ambiguous match

# Use more specific control names
# Instead of: "Cutoff" → "VCF Cutoff"
# Instead of: "Attack" → "Env 1 Attack"

# Override matches in canonical YAML
```

#### Problem: Claude API errors

**Causes:**
- API key not configured
- Rate limiting
- Network issues

**Solutions:**
```bash
# Set API key
export ANTHROPIC_API_KEY=your_api_key

# Or use claude CLI
which claude  # Verify CLI installed
claude --version

# Check rate limits in Anthropic Console
# Retry with exponential backoff
```

#### Problem: Slow matching performance

**Solutions:**
```bash
# Cache matches for reuse
# Matches saved in canonical YAML

# Batch multiple controls in single API call
# Faster than individual requests

# Use local descriptor cache
# Avoid re-reading descriptor files
```

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

**Ardour XML Structure (With Fuzzy Matching):**
```xml
<ArdourMIDIBindings version="1.0.0" name="LCXL3 - TAL-J-8">
  <DeviceInfo bank-size="8"/>

  <!-- Filter Controls (AI-matched parameters) -->
  <Binding channel="1" ctl="13" function="plugin-parameter"
           uri="TAL-J-8/param/105"/>  <!-- VCF Cutoff -->
  <Binding channel="1" ctl="14" function="plugin-parameter"
           uri="TAL-J-8/param/107"/>  <!-- VCF Resonance -->

  <!-- Envelope Controls (AI-matched parameters) -->
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
2. **AI-matched parameters included in JSON**
3. Writes to `live-max-cc-router/data/plugin-mappings.json`
4. Runtime loader merges with canonical mappings
5. Max for Live device uses merged mappings

**Dual-Pipeline Architecture:**

```
Tier 1: Canonical (Build-time)         Tier 2: Runtime (Device-extracted)
───────────────────────────            ──────────────────────────────────
Source: YAML files                      Source: Controller slot
Build:  convert-canonical-maps.cjs     Extract: controller-deploy
Output: canonical-plugin-maps.ts        Output: plugin-mappings.json
                                                (with AI-matched params)

                    Runtime Merge
                    { ...canonical, ...runtime }
```

See [live-deployer/architecture.md](./live-deployer/architecture.md) for details.

#### Manual JSON Update

```json
// live-max-cc-router/data/plugin-mappings.json
{
  "version": "1.0.0",
  "lastUpdated": "2025-10-12T10:00:00Z",
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
          "parameterIndex": 105,        // ← AI-matched
          "parameterName": "VCF Cutoff",
          "curve": "linear",
          "matchConfidence": 1.0        // ← Match quality
        },
        "53": {
          "deviceIndex": 0,
          "parameterIndex": 65,         // ← AI-matched
          "parameterName": "Env Attack",
          "curve": "linear",
          "matchConfidence": 0.9
        }
      },
      "metadata": {
        "name": "Jupiter-8 Template",
        "source": "device-extracted",
        "controllerSlot": 0,
        "extractedAt": "2025-10-12T10:00:00Z",
        "aiMatchingEnabled": true      // ← Fuzzy matching used
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

### Workflow 2: One-Click Hardware Extraction with AI Matching ⭐

```bash
# 1. Create custom mode on controller (use Novation Components)
# 2. Deploy in one command with AI-powered parameter matching
npx controller-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour live \
  --install \
  --output ./my-mappings

# Done! Both DAWs configured from hardware with AI-matched parameters
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
# Deploy multiple plugins from controller slots with AI matching
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour live
npx controller-deploy deploy --slot 1 --plugin "Mini V4" --daw ardour live
npx controller-deploy deploy --slot 2 --plugin "Jup-8 V4" --daw ardour live

# All three plugins now mapped from hardware with AI-matched parameters
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
- `-p, --plugin <name>`: Plugin name for AI parameter matching ⭐
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

# With plugin context (enables AI matching)
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

### Fuzzy Matching Issues

**Problem:** AI matching fails completely
- Verify `ANTHROPIC_API_KEY` environment variable set
- Check `claude` CLI is installed: `which claude`
- Ensure plugin descriptor exists and is valid
- Check control names are descriptive (not "Knob1", "EncA")

**Problem:** Low confidence matches (< 0.7)
- Use more specific control names on hardware
- Add semantic context ("VCF Cutoff" vs "Cutoff")
- Manually override in canonical YAML if needed

**Problem:** Multiple parameters matched to same control
- Review match confidence scores in output
- Use most specific parameter name on hardware
- Manually specify correct parameter in YAML

**Problem:** Rate limiting or API errors
- Batch controls in single request (default behavior)
- Add retry logic with exponential backoff
- Use cached results when available

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

### Cache AI Matches

AI matches are saved in canonical YAML for reuse:
```bash
# First deployment: AI matching runs (~2-5 seconds)
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --output ./cache

# Subsequent deployments: Reads from cached YAML (~instant)
npx controller-deploy deploy --input ./cache/lcxl3-jupiter8.yaml --daw ardour
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
