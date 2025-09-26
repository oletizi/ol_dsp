# Audio Control CLI Tools Installation Guide

This guide covers installing and setting up the CLI tools for both canonical MIDI maps and Ardour MIDI maps modules.

## Quick Start

### Prerequisites
- Node.js 18+
- pnpm (recommended) or npm
- Git (for development)

### Development Installation

1. **Clone and setup the monorepo**:
   ```bash
   git clone <repository-url> audio-control
   cd audio-control
   pnpm install
   ```

2. **Build all modules**:
   ```bash
   pnpm run build
   ```

3. **Link CLI tools globally** (optional):
   ```bash
   # Link canonical-midi-maps tools
   cd modules/canonical-midi-maps
   pnpm link

   # Link ardour-midi-maps tools
   cd ../ardour-midi-maps
   pnpm link
   ```

4. **Verify installation**:
   ```bash
   validate-maps --help
   validate-ardour-maps --help
   ```

### Production Installation (when published)

```bash
# Install canonical MIDI maps tools
npm install -g @audio-control/canonical-midi-maps

# Install Ardour MIDI maps tools
npm install -g @audio-control/ardour-midi-maps

# Or install both at once
npm install -g @audio-control/canonical-midi-maps @audio-control/ardour-midi-maps
```

## Available CLI Tools

### Canonical MIDI Maps Tools

After installation, these commands are available:

- **`validate-maps`** - Validate MIDI map files
- **`convert-maps`** - Convert between YAML, JSON, XML formats
- **`batch-process`** - Batch process multiple files
- **`generate-template`** - Generate templates and scaffolding

### Ardour MIDI Maps Tools

- **`validate-ardour-maps`** - Validate Ardour XML files
- **`convert-to-ardour`** - Convert canonical maps to Ardour XML

## Setup Examples

### Basic Usage Setup

1. **Create a workspace**:
   ```bash
   mkdir my-midi-project
   cd my-midi-project
   mkdir canonical-maps ardour-maps
   ```

2. **Generate your first map**:
   ```bash
   generate-template --device "Novation Launchkey MK3" --output canonical-maps/launchkey.yaml
   ```

3. **Validate the generated map**:
   ```bash
   validate-maps canonical-maps/launchkey.yaml
   ```

4. **Convert to Ardour format**:
   ```bash
   convert-to-ardour --output ardour-maps/ canonical-maps/launchkey.yaml
   ```

5. **Validate Ardour map**:
   ```bash
   validate-ardour-maps ardour-maps/launchkey.xml
   ```

### Project Structure Setup

For a typical project, organize files like this:

```
my-midi-project/
├── canonical-maps/           # Source canonical maps
│   ├── controllers/         # Controller-specific maps
│   ├── plugins/            # Plugin-specific maps
│   └── templates/          # Custom templates
├── ardour-maps/            # Generated Ardour XML files
├── function-mappings/      # Custom function mappings
│   ├── default.json       # Default function mappings
│   └── custom.json        # Project-specific mappings
├── device-info/           # Device info templates
│   └── devices.json       # Device capabilities
├── tests/                 # Generated test files
└── docs/                  # Generated documentation
```

### Advanced Configuration

1. **Create custom function mappings**:
   ```bash
   mkdir function-mappings
   cat > function-mappings/studio-setup.json << 'EOF'
   {
     "master-volume": "master-set-gain",
     "track-1-volume": "track-set-gain[1]",
     "track-1-pan": "track-set-pan-azimuth[1]",
     "play-button": {
       "function": "transport-toggle-roll",
       "momentary": "yes"
     },
     "stop-button": "transport-stop",
     "record-button": {
       "function": "transport-record-enable",
       "momentary": "yes"
     }
   }
   EOF
   ```

2. **Create device info templates**:
   ```bash
   mkdir device-info
   cat > device-info/devices.json << 'EOF'
   {
     "novation-launchkey-mk3": {
       "device-name": "Novation Launchkey MK3 49",
       "device-info": {
         "bank-size": 8,
         "motorized": "no",
         "has-lcd": "yes",
         "has-master-fader": "yes",
         "has-global-controls": "yes"
       }
     }
   }
   EOF
   ```

3. **Set up batch processing workflow**:
   ```bash
   # Create processing script
   cat > process-maps.sh << 'EOF'
   #!/bin/bash
   set -e

   echo "Validating canonical maps..."
   validate-maps --strict canonical-maps/

   echo "Converting to Ardour format..."
   convert-to-ardour \
     --functions function-mappings/studio-setup.json \
     --device-info device-info/devices.json \
     --output ardour-maps/ \
     canonical-maps/

   echo "Validating Ardour maps..."
   validate-ardour-maps --strict ardour-maps/

   echo "Processing complete!"
   EOF

   chmod +x process-maps.sh
   ```

## IDE Integration

### VS Code Setup

1. **Install recommended extensions**:
   - YAML Language Support
   - JSON Tools
   - XML Tools

2. **Configure workspace settings** (`.vscode/settings.json`):
   ```json
   {
     "yaml.validate": true,
     "yaml.hover": true,
     "yaml.completion": true,
     "json.validate": {
       "enable": true
     },
     "files.associations": {
       "*.yaml": "yaml",
       "*.yml": "yaml"
     }
   }
   ```

3. **Add tasks** (`.vscode/tasks.json`):
   ```json
   {
     "version": "2.0.0",
     "tasks": [
       {
         "label": "Validate Maps",
         "type": "shell",
         "command": "validate-maps",
         "args": ["--verbose", "canonical-maps/"],
         "group": "build",
         "problemMatcher": []
       },
       {
         "label": "Convert to Ardour",
         "type": "shell",
         "command": "convert-to-ardour",
         "args": ["--output", "ardour-maps/", "canonical-maps/"],
         "group": "build",
         "dependsOn": "Validate Maps"
       }
     ]
   }
   ```

## CI/CD Integration

### GitHub Actions Workflow

Create `.github/workflows/validate-maps.yml`:

```yaml
name: MIDI Maps Validation

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  validate:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v3

    - name: Setup Node.js
      uses: actions/setup-node@v3
      with:
        node-version: '18'
        cache: 'pnpm'

    - name: Install pnpm
      uses: pnpm/action-setup@v2
      with:
        version: 8

    - name: Install dependencies
      run: pnpm install

    - name: Build project
      run: pnpm run build

    - name: Validate canonical maps
      run: validate-maps --strict --quiet canonical-maps/

    - name: Convert to Ardour format
      run: convert-to-ardour --output temp-ardour/ canonical-maps/

    - name: Validate Ardour maps
      run: validate-ardour-maps --strict --quiet temp-ardour/

    - name: Generate documentation
      run: |
        generate-template --type documentation --output MAPS.md
        batch-process analyze --verbose canonical-maps/ > ANALYSIS.md

    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: generated-maps
        path: |
          temp-ardour/
          MAPS.md
          ANALYSIS.md
```

### Docker Setup

Create `Dockerfile`:

```dockerfile
FROM node:18-alpine

# Install pnpm
RUN npm install -g pnpm

# Set working directory
WORKDIR /app

# Copy package files
COPY package.json pnpm-lock.yaml ./
COPY modules/canonical-midi-maps/package.json ./modules/canonical-midi-maps/
COPY modules/ardour-midi-maps/package.json ./modules/ardour-midi-maps/

# Install dependencies
RUN pnpm install --frozen-lockfile

# Copy source code
COPY . .

# Build project
RUN pnpm run build

# Install CLI tools globally
RUN cd modules/canonical-midi-maps && pnpm link
RUN cd modules/ardour-midi-maps && pnpm link

# Set up working directory for maps
WORKDIR /workspace

# Entry point for CLI operations
ENTRYPOINT ["sh", "-c"]
```

Build and use:

```bash
# Build image
docker build -t audio-control-cli .

# Use for validation
docker run -v $(pwd):/workspace audio-control-cli "validate-maps maps/"

# Use for conversion
docker run -v $(pwd):/workspace audio-control-cli "convert-to-ardour --output ardour/ canonical/"
```

## Shell Integration

### Bash Completion

Add to your `~/.bashrc`:

```bash
# Enable completion for MIDI map tools
eval "$(validate-maps --completion bash)"
eval "$(convert-maps --completion bash)"
eval "$(batch-process --completion bash)"
eval "$(generate-template --completion bash)"
eval "$(validate-ardour-maps --completion bash)"
eval "$(convert-to-ardour --completion bash)"
```

### Aliases and Functions

Add helpful aliases to `~/.bashrc`:

```bash
# MIDI map aliases
alias vmap='validate-maps --verbose'
alias cmap='convert-maps'
alias amap='convert-to-ardour'

# Batch operations
alias validate-all='validate-maps --strict'
alias convert-all='batch-process convert --format json --parallel 4'

# Quick template generation
alias new-controller='generate-template --type basic'
alias new-plugin='generate-template --type plugin'

# Development workflow
midi-workflow() {
  local name=$1
  if [ -z "$name" ]; then
    echo "Usage: midi-workflow <controller-name>"
    return 1
  fi

  echo "Creating $name workflow..."
  generate-template --device "$name" --output "$name.yaml"
  validate-maps --verbose "$name.yaml"
  convert-to-ardour --output ardour/ "$name.yaml"
  validate-ardour-maps "ardour/$name.xml"
  echo "Workflow complete for $name"
}
```

## Troubleshooting Installation

### Common Issues

1. **Command not found after installation**:
   ```bash
   # Check if CLI tools are in PATH
   which validate-maps

   # If not found, try explicit path
   npx validate-maps --help

   # Or reinstall with global flag
   pnpm link
   ```

2. **Permission errors**:
   ```bash
   # Fix permissions on bin files
   chmod +x node_modules/.bin/*

   # Or use sudo for global install
   sudo npm install -g @audio-control/canonical-midi-maps
   ```

3. **Module not found errors**:
   ```bash
   # Ensure project is built
   pnpm run build

   # Check dist directory exists
   ls -la modules/*/dist/

   # Reinstall dependencies
   rm -rf node_modules
   pnpm install
   ```

4. **TypeScript/ESM import issues**:
   ```bash
   # Ensure using Node 18+
   node --version

   # Check package.json has "type": "module"
   grep '"type"' modules/*/package.json
   ```

### Getting Help

- Use `--help` with any command for detailed usage
- Check CLI guides in each module's directory
- Enable verbose output with `--verbose` for debugging
- File issues on the project repository

### Verification

After installation, verify all tools work:

```bash
# Test canonical tools
validate-maps --help
convert-maps --help
batch-process --help
generate-template --help

# Test Ardour tools
validate-ardour-maps --help
convert-to-ardour --help

# Test with sample data
generate-template --device "Test Controller" --output test.yaml
validate-maps test.yaml
convert-to-ardour test.yaml
validate-ardour-maps test.xml
rm test.yaml test.xml

echo "Installation verified successfully!"
```

This completes the installation setup. You now have comprehensive CLI tools for MIDI map development, validation, and conversion workflows.