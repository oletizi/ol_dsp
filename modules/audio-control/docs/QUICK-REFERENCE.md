# Audio Control Quick Reference

Quick reference for the 12 workflow scripts in the audio-control project.

## Workflow Commands

### Complete Workflow
```bash
pnpm workflow:complete            # Extract → Validate → Generate
pnpm workflow:health              # System health check
```

## Phase 1: Plugin Interrogation

```bash
pnpm plugins:extract              # Extract plugin parameters
pnpm plugins:extract:force        # Force re-extraction (bypass cache)
pnpm plugins:list                 # List available plugins
pnpm plugins:health               # Validate extracted descriptors
```

**Common Options:**
- `--force` - Bypass cache
- `--plugin <name>` - Target specific plugin
- `--format json|yaml|table` - Output format
- `--help` - Show help

## Phase 2: Canonical Mapping

```bash
pnpm maps:validate                # Validate mapping files
pnpm maps:list                    # List available mappings
pnpm maps:check                   # Health check against descriptors
```

**Common Options:**
- `--strict` - Strict validation mode
- `--device <name>` - Filter by device
- `--fix` - Auto-fix issues
- `--verbose` - Detailed output

## Phase 3: DAW Generation

```bash
pnpm daw:generate                 # Generate all DAW formats
pnpm daw:generate:ardour          # Generate Ardour only
pnpm daw:generate:ardour:install  # Generate and install to Ardour
pnpm daw:list                     # List generated files
```

**Common Options:**
- `--target <daw>` - Target specific DAW
- `--install` - Install to DAW config directory
- `--mapping <file>` - Use specific mapping

## Quick Examples

### New Plugin Setup
```bash
# 1. Extract plugin parameters
pnpm plugins:extract --plugin "Serum"

# 2. Create canonical mapping (manually edit YAML)
# Edit: maps/device-name/plugin-category/plugin-name.yaml

# 3. Validate mapping
pnpm maps:validate maps/device-name/plugin-category/plugin-name.yaml

# 4. Generate DAW files
pnpm daw:generate:ardour:install
```

### Daily Workflow
```bash
# Quick health check
pnpm workflow:health

# Update all mappings
pnpm workflow:complete --verbose

# Fix any issues
pnpm workflow:health --fix
```

### Troubleshooting
```bash
# Check plugin extraction issues
pnpm plugins:health --verbose

# Validate specific mapping
pnpm maps:validate --strict maps/my-device/my-plugin.yaml

# Check generated files
pnpm daw:list --status
```

## File Structure Reference

```
audio-control/
├── plugin-descriptors/          # Phase 1 output
│   ├── manufacturer-plugin.json
│   └── plugins-catalog-batch.json
├── maps/                        # Phase 2 input
│   └── device-name/
│       └── plugin-category/
│           └── plugin-name.yaml
├── generated/                   # Phase 3 output
│   └── ardour-maps/
│       └── mapping-name.map
└── tools/                       # Script implementations
    ├── plugins/
    ├── maps/
    ├── daw/
    └── workflow/
```

## Common Patterns

### Plugin Extraction
```bash
# Extract all plugins
pnpm plugins:extract

# Force re-extraction
pnpm plugins:extract:force

# Extract specific manufacturer
pnpm plugins:extract --plugin "Native Instruments"
```

### Mapping Validation
```bash
# Validate all mappings
pnpm maps:validate

# Validate with auto-fix
pnpm maps:validate --fix

# Strict validation mode
pnpm maps:validate --strict
```

### DAW Generation
```bash
# Generate for all DAWs
pnpm daw:generate

# Ardour with installation
pnpm daw:generate:ardour:install

# Custom output directory
pnpm daw:generate --output ./custom-output/
```

## Error Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | General error |
| 2 | Invalid arguments |
| 3 | File not found |
| 4 | Validation failure |
| 5 | Plugin extraction error |
| 6 | DAW generation error |

## Performance Tips

- **Use cache**: Avoid `--force` unless necessary (plugin extraction caches for 24h)
- **Target specific**: Use `--plugin` or `--mapping` for faster operations
- **Batch operations**: Run `workflow:complete` for full updates
- **Health monitoring**: Regular `workflow:health` checks catch issues early

## JSON Output for Scripting

Most commands support `--format json` for integration:

```bash
# Get plugin list as JSON
pnpm plugins:list --format json

# Health check results as JSON
pnpm workflow:health --format json

# Mapping details as JSON
pnpm maps:list --format json --details
```

## Configuration Files

### Plugin Descriptor Schema
```json
{
  "plugin": {
    "manufacturer": "string",
    "name": "string",
    "version": "string",
    "format": "VST3|AU|VST2",
    "uid": "string"
  },
  "parameters": [
    {
      "index": "number",
      "name": "string",
      "min": "number",
      "max": "number",
      "default": "number",
      "group": "string",
      "type": "continuous|discrete|boolean",
      "automatable": "boolean"
    }
  ]
}
```

### Canonical Mapping Schema
```yaml
version: "1.0.0"
device:
  manufacturer: "string"
  model: "string"
plugin:
  manufacturer: "string"
  name: "string"
  descriptor: "path/to/descriptor.json"
controls:
  - id: "string"
    name: "string"
    type: "encoder|slider|button|pad"
    cc: "number"
    channel: "global|number"
    plugin_parameter: "number"
    behavior: "absolute|relative|toggle"  # optional
```

## Help System

Every command has built-in help:

```bash
pnpm plugins:extract --help
pnpm maps:validate --help
pnpm daw:generate:ardour --help
pnpm workflow:complete --help
```

For detailed documentation, see:
- [API.md](./API.md) - Complete API reference
- [PROCESS.md](./PROCESS.md) - Workflow documentation
- [ARCHITECTURE.md](./ARCHITECTURE.md) - System architecture