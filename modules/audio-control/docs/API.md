# Audio Control Tools API Documentation

This document provides comprehensive API documentation for the 12 workflow scripts in the audio-control project.

## Overview

The tools are organized into three phases with supporting workflow management:

- **Phase 1: Plugin Interrogation** - Extract plugin parameter information
- **Phase 2: Canonical Mapping** - Validate and manage MIDI mappings
- **Phase 3: DAW Generation** - Generate DAW-specific configuration files
- **Workflow Management** - End-to-end automation and health monitoring

## Phase 1: Plugin Interrogation

### `pnpm plugins:extract`

Extract parameter information from installed audio plugins using JUCE host.

**Usage:**
```bash
pnpm plugins:extract [options]
```

**Options:**
- `--force` - Force re-extraction even if cache exists
- `--format <type>` - Output format: json, yaml (default: json)
- `--output <path>` - Custom output file path
- `--plugin <name>` - Extract specific plugin only
- `--help, -h` - Show help information

**Examples:**
```bash
# Extract all plugins
pnpm plugins:extract

# Force re-extraction (bypass 24-hour cache)
pnpm plugins:extract:force

# Extract specific plugin
pnpm plugins:extract --plugin "Serum"

# Custom output location
pnpm plugins:extract --output ./custom-descriptors/
```

**Output:**
- Plugin descriptor JSON files in `plugin-descriptors/`
- Each file named as `manufacturer-plugin-name.json`
- Summary catalog in `plugins-catalog-batch.json`

### `pnpm plugins:list`

List available plugins and their extraction status.

**Usage:**
```bash
pnpm plugins:list [options]
```

**Options:**
- `--format <type>` - Output format: table, json, yaml (default: table)
- `--status <filter>` - Filter by status: extracted, missing, error
- `--manufacturer <name>` - Filter by manufacturer
- `--help, -h` - Show help information

**Examples:**
```bash
# List all plugins in table format
pnpm plugins:list

# List only extracted plugins
pnpm plugins:list --status extracted

# List plugins from specific manufacturer
pnpm plugins:list --manufacturer "Native Instruments"

# JSON output for scripting
pnpm plugins:list --format json
```

### `pnpm plugins:health`

Validate extracted plugin descriptors and check system health.

**Usage:**
```bash
pnpm plugins:health [options]
```

**Options:**
- `--fix` - Automatically fix common issues
- `--verbose, -v` - Detailed output
- `--format <type>` - Output format: summary, detailed, json (default: summary)
- `--help, -h` - Show help information

**Examples:**
```bash
# Basic health check
pnpm plugins:health

# Detailed health report
pnpm plugins:health --verbose

# Auto-fix issues
pnpm plugins:health --fix
```

**Health Checks:**
- Parameter index validation
- Required field presence
- Data type consistency
- MIDI protocol compliance
- File integrity verification

## Phase 2: Canonical Mapping

### `pnpm maps:validate`

Validate canonical mapping files against schemas and plugin descriptors.

**Usage:**
```bash
pnpm maps:validate [options] [files...]
```

**Options:**
- `--schema <path>` - Custom Zod schema file
- `--strict` - Enable strict validation mode
- `--format <type>` - Output format: summary, detailed, json (default: summary)
- `--help, -h` - Show help information

**Examples:**
```bash
# Validate all mapping files
pnpm maps:validate

# Validate specific files
pnpm maps:validate maps/novation-lcxl3/*.yaml

# Strict validation with detailed output
pnpm maps:validate --strict --format detailed
```

**Validation Checks:**
- YAML/JSON syntax validation
- Zod schema compliance
- Plugin descriptor cross-references
- MIDI CC range validation
- Parameter index verification

### `pnpm maps:list`

List available canonical mappings with metadata.

**Usage:**
```bash
pnpm maps:list [options]
```

**Options:**
- `--device <name>` - Filter by device manufacturer/model
- `--plugin <name>` - Filter by plugin name
- `--format <type>` - Output format: table, json, yaml (default: table)
- `--details` - Include mapping details in output
- `--help, -h` - Show help information

**Examples:**
```bash
# List all mappings
pnpm maps:list

# Filter by device
pnpm maps:list --device "Launch Control XL"

# Show mapping details
pnpm maps:list --details

# JSON output for scripting
pnpm maps:list --format json
```

### `pnpm maps:check`

Health check canonical mappings against plugin descriptors.

**Usage:**
```bash
pnpm maps:check [options]
```

**Options:**
- `--score-threshold <number>` - Minimum health score (0-100, default: 80)
- `--fix-references` - Auto-fix broken plugin descriptor references
- `--verbose, -v` - Detailed output
- `--format <type>` - Output format: summary, detailed, json (default: summary)
- `--help, -h` - Show help information

**Examples:**
```bash
# Basic health check
pnpm maps:check

# Set custom health score threshold
pnpm maps:check --score-threshold 90

# Auto-fix broken references
pnpm maps:check --fix-references --verbose
```

**Health Checks:**
- Plugin descriptor availability
- Parameter index validity
- MIDI CC collision detection
- Control mapping completeness
- Semantic consistency scoring

## Phase 3: DAW Generation

### `pnpm daw:generate`

Generate DAW-specific configuration files from canonical mappings.

**Usage:**
```bash
pnpm daw:generate [options]
```

**Options:**
- `--target <daw>` - Target DAW: ardour, ableton, reaper (default: all)
- `--mapping <file>` - Generate from specific mapping file
- `--output <path>` - Custom output directory
- `--format <type>` - Output format specific to target DAW
- `--help, -h` - Show help information

**Examples:**
```bash
# Generate all DAW formats
pnpm daw:generate

# Generate Ardour only
pnpm daw:generate --target ardour

# Generate from specific mapping
pnpm daw:generate --mapping maps/novation-lcxl3/tal-j8.yaml

# Custom output directory
pnpm daw:generate --output ./custom-daw-maps/
```

### `pnpm daw:generate:ardour`

Generate Ardour-specific MIDI mapping files.

**Usage:**
```bash
pnpm daw:generate:ardour [options]
```

**Options:**
- `--install` - Install generated files to Ardour config directory
- `--ardour-version <version>` - Target Ardour version (default: 8)
- `--template <name>` - Use specific Ardour template
- `--help, -h` - Show help information

**Examples:**
```bash
# Generate Ardour mappings
pnpm daw:generate:ardour

# Generate and install to Ardour
pnpm daw:generate:ardour:install

# Target specific Ardour version
pnpm daw:generate:ardour --ardour-version 7
```

**Output:**
- Ardour `.map` files in `generated/ardour-maps/`
- Automatic installation to `~/.config/ardour8/midi_maps/` (when using `--install`)

### `pnpm daw:list`

List generated DAW configuration files.

**Usage:**
```bash
pnpm daw:list [options]
```

**Options:**
- `--daw <name>` - Filter by DAW type
- `--format <type>` - Output format: table, json, yaml (default: table)
- `--status` - Include file status (size, modification time)
- `--help, -h` - Show help information

**Examples:**
```bash
# List all generated files
pnpm daw:list

# List Ardour files only
pnpm daw:list --daw ardour

# Include file status
pnpm daw:list --status
```

## Workflow Management

### `pnpm workflow:complete`

Execute the complete workflow: extract → validate → generate.

**Usage:**
```bash
pnpm workflow:complete [options]
```

**Options:**
- `--force-extract` - Force plugin re-extraction
- `--target-daw <daw>` - Generate for specific DAW only
- `--install` - Install generated files to DAW config directories
- `--verbose, -v` - Detailed progress output
- `--help, -h` - Show help information

**Examples:**
```bash
# Run complete workflow
pnpm workflow:complete

# Force extraction and install to Ardour
pnpm workflow:complete --force-extract --target-daw ardour --install

# Verbose output
pnpm workflow:complete --verbose
```

**Workflow Steps:**
1. **Plugin Extraction** - Extract parameters from installed plugins
2. **Mapping Validation** - Validate all canonical mappings
3. **Cross-Validation** - Check mappings against descriptors
4. **DAW Generation** - Generate target DAW configuration files
5. **Health Report** - Final system health summary

### `pnpm workflow:health`

Comprehensive system health check across all phases.

**Usage:**
```bash
pnpm workflow:health [options]
```

**Options:**
- `--detailed` - Include detailed diagnostics
- `--fix` - Automatically fix resolvable issues
- `--format <type>` - Output format: summary, detailed, json (default: summary)
- `--threshold <number>` - Overall health score threshold (default: 80)
- `--help, -h` - Show help information

**Examples:**
```bash
# Basic health check
pnpm workflow:health

# Detailed diagnostics
pnpm workflow:health --detailed

# Auto-fix issues
pnpm workflow:health --fix

# JSON output for CI/CD
pnpm workflow:health --format json
```

**Health Dimensions:**
- **Plugin Coverage** - Percentage of plugins successfully extracted
- **Mapping Validity** - Canonical mapping validation score
- **Reference Integrity** - Plugin descriptor cross-reference accuracy
- **DAW Compatibility** - Generated file compliance with DAW specifications
- **Performance Metrics** - Tool execution times and resource usage

## Error Handling

All tools implement consistent error handling:

### Exit Codes
- `0` - Success
- `1` - General error
- `2` - Invalid arguments
- `3` - File not found
- `4` - Validation failure
- `5` - Plugin extraction error
- `6` - DAW generation error

### Error Output Format
```json
{
  "success": false,
  "errors": [
    {
      "type": "ValidationError",
      "message": "Parameter index 105 not found in plugin descriptor",
      "file": "maps/novation-lcxl3/tal-j8.yaml",
      "line": 15,
      "suggestions": ["Check plugin descriptor file", "Verify parameter index"]
    }
  ],
  "metadata": {
    "phase": "validation",
    "tool": "maps:validate",
    "timestamp": "2024-09-26T09:30:00Z"
  }
}
```

## Performance Characteristics

### Execution Times (Typical)
- **Plugin Extraction**: ~30s per plugin (first run), ~5s (cached)
- **Mapping Validation**: <10ms per mapping file
- **DAW Generation**: <20ms per mapping
- **Complete Workflow**: <2s for typical project
- **Health Checks**: <100ms system-wide

### Resource Usage
- **Memory**: <50MB for typical operations
- **Disk**: Plugin descriptors ~1MB per 100 plugins
- **CPU**: Optimized for real-time performance

## TypeScript Interfaces

### Core Data Types

```typescript
interface PluginDescriptor {
  plugin: {
    manufacturer: string;
    name: string;
    version: string;
    format: 'VST3' | 'AU' | 'VST2';
    uid: string;
  };
  parameters: PluginParameter[];
}

interface PluginParameter {
  index: number;
  name: string;
  min: number;
  max: number;
  default: number;
  group: string;
  type: 'continuous' | 'discrete' | 'boolean';
  automatable: boolean;
}

interface CanonicalMapping {
  version: string;
  device: DeviceInfo;
  plugin: PluginReference;
  controls: ControlMapping[];
}

interface ControlMapping {
  id: string;
  name: string;
  type: 'encoder' | 'slider' | 'button' | 'pad';
  cc: number;
  channel: 'global' | number;
  plugin_parameter: number;
  behavior?: 'absolute' | 'relative' | 'toggle';
}
```

### Tool Result Types

```typescript
interface ToolResult<T> {
  success: boolean;
  data?: T;
  errors: ToolError[];
  metadata: {
    tool: string;
    phase: string;
    timestamp: string;
    duration: number;
  };
}

interface HealthCheckResult {
  overall_score: number;
  dimensions: {
    plugin_coverage: number;
    mapping_validity: number;
    reference_integrity: number;
    daw_compatibility: number;
  };
  issues: HealthIssue[];
  recommendations: string[];
}
```

## Development and Testing

### Adding New Tools
1. Create tool script in appropriate `tools/{phase}/` directory
2. Implement consistent CLI interface with `parseArgs`
3. Add error handling with descriptive messages
4. Include help documentation
5. Add script entry to root `package.json`
6. Write unit tests covering all functionality

### Testing Individual Tools
```bash
# Test with --help flag
pnpm plugins:extract --help

# Test error conditions
pnpm maps:validate non-existent-file.yaml

# Test JSON output for scripting
pnpm plugins:list --format json | jq '.[] | .name'
```

### Integration Testing
```bash
# Test complete workflow
pnpm workflow:complete --verbose

# Test health monitoring
pnpm workflow:health --detailed

# Test error recovery
pnpm workflow:health --fix
```

This API documentation provides comprehensive coverage of all 12 workflow scripts, their options, usage patterns, and integration points within the audio-control ecosystem.