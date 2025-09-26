# Ardour MIDI Maps CLI Tools

This guide covers the command-line tools for working with Ardour MIDI map XML files. These tools provide validation and conversion capabilities specifically for Ardour DAW integration.

## Installation

### Local Development
```bash
# Install dependencies
pnpm install

# Build the project
pnpm run build

# Install CLI tools globally (optional)
pnpm link
```

### Production Usage
```bash
# Install from npm (when published)
npm install -g @audio-control/ardour-midi-maps
```

## Available Tools

### 1. validate-ardour-maps - Ardour XML Validator

Validates Ardour MIDI map XML files for proper structure, MIDI parameter ranges, and Ardour function compatibility.

#### Usage
```bash
validate-ardour-maps [options] <file|directory>...
```

#### Options
- `-h, --help`: Show help message
- `-s, --strict`: Treat warnings as errors
- `-v, --verbose`: Show detailed validation information
- `-q, --quiet`: Only show errors (suppress success messages)
- `--no-function-check`: Skip Ardour function name validation

#### Examples
```bash
# Validate single XML file
validate-ardour-maps controller.xml

# Validate all XML files in directory
validate-ardour-maps ardour-maps/

# Strict validation with detailed output
validate-ardour-maps --strict --verbose controller.xml

# Skip function validation
validate-ardour-maps --no-function-check experimental-maps/

# Quiet mode for CI/CD
validate-ardour-maps --quiet maps/
```

#### What It Validates

**XML Structure:**
- Valid XML syntax and formatting
- Proper `ArdourMIDIBindings` root element
- Required attributes (`name`, etc.)

**MIDI Parameters:**
- MIDI channels in range 1-16
- MIDI CC numbers in range 0-127
- MIDI note numbers in range 0-127
- RPN/NRPN values in range 0-16383
- Threshold values in range 0-127

**Ardour Specifics:**
- Known Ardour function names
- Binding target validation (function/action/uri)
- Device info parameter ranges
- Duplicate CC assignment detection

#### Exit Codes
- `0`: All files valid
- `1`: Validation errors found
- `2`: Invalid command line arguments

### 2. convert-to-ardour - Canonical to Ardour Converter

Converts canonical MIDI map files (YAML/JSON) to Ardour XML format with intelligent function mapping.

#### Usage
```bash
convert-to-ardour [options] <file|directory>...
```

#### Options
- `-h, --help`: Show help message
- `-o, --output <dir>`: Output directory for XML files (default: same as input)
- `-f, --functions <file>`: JSON file with custom function mappings
- `-d, --device-info <file>`: JSON file with custom device info templates
- `--no-validate`: Skip validation of input and output
- `--overwrite`: Overwrite existing files without prompt
- `-v, --verbose`: Show detailed conversion information
- `-q, --quiet`: Only show errors
- `--dry-run`: Show what would be converted without writing files

#### Examples
```bash
# Convert single canonical file
convert-to-ardour controller.yaml

# Convert with custom function mappings
convert-to-ardour --functions custom-functions.json controller.yaml

# Batch convert directory
convert-to-ardour --output ardour-maps/ canonical-maps/

# Preview conversion without writing files
convert-to-ardour --dry-run --verbose controller.yaml

# Convert with custom device info
convert-to-ardour --device-info devices.json --output ardour/ maps/
```

## Function Mapping

The converter uses a hierarchical function mapping system:

1. **Custom Function Mappings** (highest priority)
2. **Built-in Default Mappings**
3. **Generic Parameter Mappings** (fallback)

### Built-in Function Mappings

The converter includes mappings for common control names:

```javascript
{
  // Volume controls
  "volume": "master-set-gain",
  "master-volume": "master-set-gain",
  "track-volume": "track-set-gain[1]",

  // Transport controls
  "play": "transport-toggle-roll",
  "stop": "transport-stop",
  "record": "transport-record-enable",

  // Track controls
  "solo": "track-set-solo[1]",
  "mute": "track-set-mute[1]",
  "pan": "track-set-pan-azimuth[1]",

  // Navigation
  "track-left": "select-prev-track",
  "track-right": "select-next-track"
}
```

### Custom Function Mappings

Create a JSON file with custom mappings:

```json
{
  "my-volume-knob": "master-set-gain",
  "special-button": {
    "function": "transport-toggle-roll",
    "momentary": "yes"
  },
  "encoder-1": {
    "function": "track-set-pan-azimuth[1]",
    "encoder": "yes"
  }
}
```

### Supported Ardour Functions

The validator recognizes these Ardour functions:

**Transport:**
- `transport-start`, `transport-stop`, `transport-toggle-roll`
- `transport-record-enable`, `transport-goto-start`, `transport-goto-end`
- `transport-loop-toggle`, `transport-play-selection`

**Track Control:**
- `track-set-gain[N]`, `track-set-solo[N]`, `track-set-mute[N]`
- `track-set-pan-azimuth[N]`, `track-set-send-gain[N][M]`
- `track-set-monitor-disk`, `track-set-monitor-input`

**Navigation:**
- `select-track`, `select-prev-track`, `select-next-track`
- `bank-up`, `bank-down`, `channel-left`, `channel-right`

**Master:**
- `master-set-gain`, `master-set-solo`, `master-set-mute`

**Zoom:**
- `zoom-in`, `zoom-out`, `zoom-to-session`, `zoom-to-selection`

## Device Info Configuration

Device info templates can be provided in JSON format:

```json
{
  "novation-launchkey-mk3": {
    "device-name": "Novation Launchkey MK3",
    "device-info": {
      "bank-size": 8,
      "motorized": "no",
      "has-lcd": "yes",
      "has-master-fader": "yes",
      "has-global-controls": "yes",
      "uses-logic-control-buttons": "no"
    }
  }
}
```

### Device Info Attributes

**Numeric:**
- `bank-size`: Number of tracks/channels per bank
- `threshold`: MIDI threshold value (0-127)

**Boolean (yes/no):**
- `motorized`: Motorized faders
- `has-master-fader`: Dedicated master fader
- `has-lcd`: LCD display
- `has-meters`: Level meters
- `has-timecode`: Timecode display
- `has-global-controls`: Global transport/utility controls
- `has-touch-sense-faders`: Touch-sensitive faders
- `has-jog-wheel`: Jog wheel for navigation
- `has-segmented-display`: Segmented LED display
- `uses-logic-control-buttons`: Logic Control protocol buttons
- `uses-mackie-control-buttons`: Mackie Control protocol buttons
- `uses-ipmidi`: IP MIDI networking

## Common Workflows

### Basic Conversion Workflow

1. **Start with canonical map**:
   ```bash
   # Assume you have controller.yaml from canonical-midi-maps
   ```

2. **Convert to Ardour format**:
   ```bash
   convert-to-ardour controller.yaml
   ```

3. **Validate the generated XML**:
   ```bash
   validate-ardour-maps controller.xml
   ```

4. **Install in Ardour**:
   ```bash
   # Copy to Ardour's MIDI map directory
   cp controller.xml ~/.config/ardour8/midi_maps/
   ```

### Custom Function Mapping Workflow

1. **Create function mappings file**:
   ```bash
   cat > my-functions.json << 'EOF'
   {
     "volume": "master-set-gain",
     "filter": "track-set-send-gain[1][1]",
     "play-button": {
       "function": "transport-toggle-roll",
       "momentary": "yes"
     }
   }
   EOF
   ```

2. **Convert with custom mappings**:
   ```bash
   convert-to-ardour --functions my-functions.json controller.yaml
   ```

3. **Validate and test**:
   ```bash
   validate-ardour-maps --verbose controller.xml
   ```

### Batch Processing Workflow

1. **Convert multiple canonical maps**:
   ```bash
   convert-to-ardour --output ardour-maps/ canonical-maps/
   ```

2. **Validate all generated maps**:
   ```bash
   validate-ardour-maps --strict ardour-maps/
   ```

3. **Install all maps**:
   ```bash
   cp ardour-maps/*.xml ~/.config/ardour8/midi_maps/
   ```

## Integration with Ardour

### Installing Generated Maps

1. **Find your Ardour config directory**:
   - Linux: `~/.config/ardour8/`
   - macOS: `~/Library/Preferences/Ardour8/`
   - Windows: `%APPDATA%/Ardour8/`

2. **Copy XML files to MIDI maps directory**:
   ```bash
   mkdir -p ~/.config/ardour8/midi_maps/
   cp *.xml ~/.config/ardour8/midi_maps/
   ```

3. **Restart Ardour** to detect new maps

4. **Select your controller** in Ardour:
   - Go to `Window > Preferences > Control Surfaces`
   - Enable `Generic MIDI`
   - Select your generated map file

### Testing in Ardour

1. **Enable MIDI input** in Ardour preferences
2. **Connect your controller** via MIDI
3. **Test each control** by moving/pressing and observing Ardour's response
4. **Check the log** for any binding errors

### Troubleshooting

**Controller not responding:**
- Verify MIDI connections in Ardour's MIDI setup
- Check MIDI channel configuration
- Enable MIDI learn mode for debugging

**Wrong parameters controlled:**
- Check function mappings in your JSON file
- Verify control IDs match between canonical and function maps
- Use `--verbose` flag to see conversion details

**XML validation errors:**
- Check MIDI parameter ranges (channels, CC numbers)
- Verify Ardour function names
- Use `--no-function-check` to skip function validation

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Validate Ardour Maps

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '18'

      - name: Install dependencies
        run: pnpm install

      - name: Build project
        run: pnpm run build

      - name: Validate Ardour maps
        run: validate-ardour-maps --strict --quiet ardour-maps/

      - name: Convert and validate canonical maps
        run: |
          convert-to-ardour --output temp/ canonical-maps/
          validate-ardour-maps --strict temp/
```

## Performance and Limitations

### Performance
- **XML parsing**: Optimized for files up to 100KB
- **Validation**: Sub-second for typical controller maps
- **Batch processing**: Processes files sequentially

### Current Limitations
- **Plugin parameters**: Generic mapping only (no plugin-specific automation)
- **Complex bindings**: Advanced Ardour features not fully supported
- **XML namespaces**: Basic XML support (no complex namespace handling)
- **Mackie/Logic protocols**: Generic MIDI only

### Future Enhancements
- Plugin-specific parameter automation
- Mackie Control protocol support
- Advanced XML schema validation
- Interactive function mapping wizard

## Support

For issues with Ardour CLI tools:

1. **Validation errors**: Use `--verbose` for detailed error information
2. **Conversion issues**: Check canonical map format first
3. **Ardour integration**: Test XML files manually in Ardour
4. **Function mappings**: Refer to Ardour documentation for available functions

For bug reports or feature requests, please file an issue in the project repository.