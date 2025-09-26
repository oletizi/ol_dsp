# Audio Control Workflow Process

This document describes the complete workflow for creating accurate MIDI controller mappings from plugin interrogation to DAW integration.

## Overview

The process follows three distinct phases:

1. **Plugin Interrogation** - Extract accurate parameter data from audio plugins
2. **Canonical Mapping** - Define controller-specific CC to parameter mappings
3. **DAW Generation** - Generate DAW-specific mapping files

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   PHASE 1    │    │   PHASE 2    │    │   PHASE 3    │
│   Plugin     │───▶│   Canonical  │───▶│     DAW      │
│ Interrogation│    │   Mapping    │    │  Generation  │
└──────────────┘    └──────────────┘    └──────────────┘
```

## Phase 1: Plugin Interrogation

### Purpose
Extract comprehensive parameter information from installed audio plugins to ensure mappings use real, valid parameter indices.

### Process

#### 1.1 Build the Plugin Host
See top-level project README

#### 1.2 Run Plugin Interrogation
```bash
# Extract plugin parameters (from project root)
pnpm plugins:extract

# Force re-extraction (bypass cache)
pnpm plugins:extract:force

# List available plugins
pnpm plugins:list

# Validate extracted descriptors
pnpm plugins:health
```

This command:
- Launches the custom `plughost` binary
- Scans all installed VST3 plugins (skipping problematic ones like UAD)
- Extracts parameter information for each plugin
- Outputs individual JSON stanzas to a log file
- Caches results for 24 hours to avoid re-scanning

#### 1.3 Process Interrogation Results
The batch generator:
- Reads JSON stanzas from the log file
- Converts raw parameter data to structured plugin descriptors
- Categorizes parameters into logical groups (oscillator, filter, envelope, etc.)
- Saves individual descriptor files as `manufacturer-plugin-name.json`

### Output
```
plugin-descriptors/
├── tal-togu-audio-line-tal-j-8.json     
├── arturia-jup-8-v4.json                 # Jupiter-8 emulation
├── analogobsession-oss.json              # Channel strip
└── plugins-catalog-batch.json           # Summary catalog
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
    }
  ]
}
```

## Phase 2: Canonical Mapping Definition

### Purpose
Define how physical controls on a MIDI controller map to plugin parameters, using the accurate parameter indices from Phase 1.

### Process

#### 2.1 Choose Target Hardware & Plugin
Identify the specific combination:
- **Hardware Controller**: e.g., Novation Launch Control XL 3
- **Audio Plugin**: e.g., TAL-J-8 (Jupiter-8 emulation)
- **Plugin Descriptor**: Reference the JSON file from Phase 1

#### 2.2 Create Canonical Mapping File
Create a YAML file in the appropriate directory structure:
```
maps/
└── novation-launch-control-xl-3/
    └── roland-jupiter-8/
        └── tal-j8.yaml
```

#### 2.3 Define Control Mappings
Map each physical control to a plugin parameter. For the Novation Launch Control XL 3, an efficient way to create these
canonical bindings is to create a "custom mode" in the Novation components editor with the mapping you want, making sure
to label the controls; then, use Claude Code with playwright mcp to snarf the mapping from the components editor and generate
a canonical mapping file like the following:

#### 2.4 Validate Canonical Mappings
```bash
# Validate all mapping files
pnpm maps:validate

# List available canonical mappings
pnpm maps:list

# Health check (validate against plugin descriptors)
pnpm maps:check
```

```yaml
version: 1.0.0
device:
  manufacturer: Novation
  model: Launch Control XL 3

plugin:
  manufacturer: TAL Software
  name: TAL-J-8
  descriptor: plugin-descriptors/tal-togu-audio-line-tal-j-8.json

controls:
  # Physical encoder 1 → Plugin parameter 105 (VCF Cutoff)
  - id: encoder_1
    name: VCF Cutoff
    type: encoder
    cc: 13                # MIDI CC number
    channel: global
    plugin_parameter: 105  # Real parameter index from descriptor

  # Physical slider 1 → Plugin parameter 65 (Attack)
  - id: slider_1
    name: Attack
    type: slider
    cc: 53                # MIDI CC number
    channel: global
    plugin_parameter: 65   # Real parameter index from descriptor
```

## Phase 3: DAW-Specific Generation

### Purpose
Convert canonical mappings into DAW-specific formats for actual use.

### Process

#### 3.1 DAW Format Generation
Use the new consolidated scripts to generate DAW-specific formats:

```bash
# Generate all DAW formats
pnpm daw:generate

# Generate Ardour only
pnpm daw:generate:ardour

# Generate and install to Ardour config directory
pnpm daw:generate:ardour:install

# List generated DAW files
pnpm daw:list
```

#### 3.2 Installation in DAW
```bash
# Copy to Ardour's MIDI maps directory (Linux/macOS)
cp tal-j8-lcxl3.map ~/.config/ardour8/midi_maps/

# On Windows:
# copy tal-j8-lcxl3.map "%APPDATA%\Ardour8\midi_maps\"

# The mapping will now appear in Ardour's control surface preferences
```

#### 3.3 Other DAW Formats (Future)
The same canonical mapping can generate:
- **Ableton Live**: Remote scripts (Python)
- **Reaper**: ReaControlMIDI mappings
- **Bitwig**: Controller scripts (JavaScript)
~~- **Logic Pro**: Controller assignments~~

## Workflow Management

### Complete Workflow Commands

```bash
# Run complete workflow (extract → validate → generate)
pnpm workflow:complete

# System health check across all phases
pnpm workflow:health
```

### Individual Phase Commands

**Phase 1: Plugin Interrogation**
```bash
pnpm plugins:extract              # Main batch extraction
pnpm plugins:extract:force        # Force re-extraction
pnpm plugins:list                 # List available plugins
pnpm plugins:health               # Validate extracted descriptors
```

**Phase 2: Canonical Mapping**
```bash
pnpm maps:validate                # Validate mapping files
pnpm maps:list                    # List available mappings
pnpm maps:check                   # Health check against descriptors
```

**Phase 3: DAW Generation**
```bash
pnpm daw:generate                 # Generate all DAW formats
pnpm daw:generate:ardour          # Generate Ardour only
pnpm daw:generate:ardour:install  # Generate and install to Ardour
pnpm daw:list                     # List generated files
```

## Complete Example Workflow

### Scenario: Mapping TAL-J-8 to Launch Control XL 3

```bash
# Step 1: Extract plugin parameters
pnpm plugins:extract
# Output: plugin-descriptors/tal-togu-audio-line-tal-j-8.json

# Step 2: Create canonical mapping
cat > maps/novation-launch-control-xl-3/roland-jupiter-8/tal-j8.yaml << 'EOF'
version: 1.0.0
device:
  manufacturer: Novation
  model: Launch Control XL 3
plugin:
  manufacturer: TAL Software
  name: TAL-J-8
  descriptor: plugin-descriptors/tal-togu-audio-line-tal-j-8.json
controls:
  - id: encoder_1
    name: VCF Cutoff
    cc: 13
    plugin_parameter: 105
EOF

# Step 3: Validate the mapping
pnpm maps:validate

# Step 4: Generate and install Ardour mapping
pnpm daw:generate:ardour:install
# Mapping files will be generated and installed to Ardour config directory

# Optional: Run complete workflow
pnpm workflow:complete
# Runs extraction → validation → generation in sequence
```