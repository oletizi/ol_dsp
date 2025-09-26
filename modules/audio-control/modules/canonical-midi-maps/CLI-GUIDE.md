# Canonical MIDI Maps CLI Tools

This guide covers the command-line tools for working with canonical MIDI map files. These tools provide validation, conversion, batch processing, and template generation capabilities.

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
npm install -g @audio-control/canonical-midi-maps
```

## Available Tools

### 1. validate-maps - MIDI Map Validator

Validates canonical MIDI map files for syntax, structure, and best practices.

#### Usage
```bash
validate-maps [options] <file|directory>...
```

#### Options
- `-h, --help`: Show help message
- `-s, --strict`: Treat warnings as errors
- `-v, --verbose`: Show detailed validation information
- `-q, --quiet`: Only show errors (suppress success messages)

#### Examples
```bash
# Validate single file
validate-maps controller.yaml

# Validate all maps in directory
validate-maps maps/

# Strict validation with verbose output
validate-maps --strict --verbose controller.yaml

# Quiet mode for CI/CD
validate-maps --quiet maps/
```

#### Exit Codes
- `0`: All files valid
- `1`: Validation errors found
- `2`: Invalid command line arguments

### 2. convert-maps - Format Converter

Converts MIDI map files between YAML, JSON, and XML formats.

#### Usage
```bash
convert-maps [options] <file|directory>...
```

#### Options
- `-h, --help`: Show help message
- `--from <format>`: Source format (yaml, json, xml) - auto-detected if not specified
- `--to <format>`: Target format (yaml, json, xml) [default: yaml]
- `-o, --output <dir>`: Output directory (default: same as input)
- `--validate`: Validate during conversion [default: true]
- `--no-validate`: Skip validation during conversion
- `--backup`: Create backup files before conversion
- `--no-pretty`: Disable pretty formatting for JSON/YAML
- `--overwrite`: Overwrite existing files without prompt
- `-v, --verbose`: Show detailed conversion information
- `-q, --quiet`: Only show errors

#### Examples
```bash
# Convert YAML to JSON
convert-maps --to json controller.yaml

# Batch convert directory to YAML
convert-maps --to yaml --output converted/ maps/

# Convert with backup and explicit format
convert-maps --from json --to yaml --backup file.json

# Convert with validation disabled
convert-maps --no-validate --to json maps/
```

### 3. batch-process - Batch Processor

Process multiple MIDI map files with parallel execution and advanced operations.

#### Usage
```bash
batch-process <operation> [options] <file|directory>...
```

#### Operations
- `validate`: Validate MIDI map files
- `convert`: Convert between formats
- `optimize`: Optimize and reformat files
- `analyze`: Analyze maps and generate statistics

#### Options
- `-h, --help`: Show help message
- `-p, --parallel <n>`: Number of parallel workers [default: 2]
- `-o, --output <dir>`: Output directory for processed files
- `-f, --format <fmt>`: Output format (yaml, json) [default: yaml]
- `--optimize`: Apply optimization during processing
- `-s, --strict`: Treat warnings as errors
- `-v, --verbose`: Show detailed processing information
- `-q, --quiet`: Only show summary and errors
- `--dry-run`: Show what would be processed without making changes

#### Examples
```bash
# Batch validate all maps
batch-process validate maps/

# Batch convert to JSON with 4 parallel workers
batch-process convert --format json --parallel 4 --output dist/ maps/

# Analyze maps with detailed output
batch-process analyze --verbose large-collection/

# Optimize maps in dry-run mode
batch-process optimize --dry-run --verbose maps/
```

### 4. generate-template - Template Generator

Generate MIDI map templates, test scaffolding, and documentation.

#### Usage
```bash
generate-template [options]
```

#### Options
- `-h, --help`: Show help message and list available templates
- `-t, --type <type>`: Template type [default: basic]
  - `basic`: Generate basic MIDI map template
  - `plugin`: Generate plugin-specific mapping template
  - `test`: Generate test files and scaffolding
  - `documentation`: Generate documentation templates
- `-d, --device <name>`: Device name or template key
- `-p, --plugin <name>`: Plugin name or template key
- `-n, --name <name>`: Template name/identifier
- `-o, --output <file>`: Output file path
- `-f, --format <format>`: Output format (yaml, json) [default: yaml]
- `-i, --interactive`: Interactive template creation
- `--overwrite`: Overwrite existing files
- `-v, --verbose`: Show detailed information

#### Device Templates
- `novation-launchkey-mk3`: Novation Launchkey MK3 series
- `akai-mpk-mini-mk3`: Akai MPK Mini MK3
- `arturia-beatstep`: Arturia BeatStep

#### Plugin Templates
- `serum`: Xfer Records Serum
- `massive-x`: Native Instruments Massive X

#### Examples
```bash
# Generate basic device template
generate-template --device "Novation Launchkey MK3"

# Generate plugin-specific template
generate-template --plugin serum --output serum-map.yaml

# Generate test scaffolding
generate-template --type test --name my-controller

# Generate documentation template
generate-template --type documentation --name "My Controller"

# Interactive template creation
generate-template --interactive

# List available templates
generate-template --help
```

## Common Workflows

### Development Workflow

1. **Create a new mapping**:
   ```bash
   generate-template --device "Your Controller" --output my-controller.yaml
   ```

2. **Edit the generated template** in your preferred editor

3. **Validate your changes**:
   ```bash
   validate-maps --verbose my-controller.yaml
   ```

4. **Generate test scaffolding**:
   ```bash
   generate-template --type test --name my-controller
   ```

5. **Run tests**:
   ```bash
   npm test
   ```

### Production Workflow

1. **Validate all maps**:
   ```bash
   batch-process validate --strict maps/
   ```

2. **Convert to required formats**:
   ```bash
   batch-process convert --format json --output dist/ maps/
   ```

3. **Generate documentation**:
   ```bash
   generate-template --type documentation --name "Project Maps"
   ```

### CI/CD Integration

```yaml
# Example GitHub Actions workflow
- name: Validate MIDI Maps
  run: validate-maps --strict --quiet maps/

- name: Convert Maps
  run: batch-process convert --format json --output dist/ maps/
```

## Error Handling

All tools use consistent exit codes:
- `0`: Success
- `1`: Operation failed (validation errors, conversion failures)
- `2`: Invalid command line arguments

### Common Issues

1. **Validation Failures**:
   - Check MIDI channel range (1-16)
   - Verify CC numbers (0-127)
   - Ensure unique control IDs
   - Fix duplicate CC assignments

2. **Conversion Issues**:
   - Validate input format first
   - Check file permissions
   - Ensure target directory exists

3. **Template Generation**:
   - Specify output file path
   - Check for existing files (use --overwrite)
   - Verify device/plugin template names

### Debugging

Use `--verbose` flag with any tool for detailed output:
```bash
validate-maps --verbose --strict controller.yaml
convert-maps --verbose --to json controller.yaml
batch-process --verbose analyze maps/
```

## Integration Examples

### Node.js/JavaScript
```javascript
import { MapValidator } from '@audio-control/canonical-midi-maps/cli';

const validator = new MapValidator();
const result = await validator.validate(['controller.yaml'], {
  strict: true,
  verbose: false,
  quiet: false
});

if (result) {
  console.log('Validation passed!');
} else {
  console.error('Validation failed!');
}
```

### Shell Scripts
```bash
#!/bin/bash

# Validate all maps
if validate-maps --strict maps/; then
  echo "All maps valid, proceeding with build..."
  batch-process convert --format json --output dist/ maps/
else
  echo "Validation failed, aborting build"
  exit 1
fi
```

## Performance Notes

- **Batch Processing**: Use `--parallel` option to control concurrency
- **Large Collections**: Use `batch-process` instead of individual tool calls
- **CI/CD**: Use `--quiet` flag to reduce log output
- **Memory Usage**: Tools are optimized for files up to 10MB each

## Support

For issues with CLI tools:
1. Check command syntax with `--help`
2. Use `--verbose` for detailed error information
3. Validate input files with `validate-maps` first
4. Check file permissions and paths

For feature requests or bugs, please file an issue in the project repository.