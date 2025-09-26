# Audio Control Troubleshooting Guide

This guide helps you diagnose and fix common issues with the audio-control workflow tools.

## Quick Diagnostics

### System Health Check
```bash
# Run comprehensive health check
pnpm workflow:health --detailed

# Auto-fix common issues
pnpm workflow:health --fix

# Get health report as JSON for analysis
pnpm workflow:health --format json
```

## Phase 1: Plugin Interrogation Issues

### Plugin Extraction Fails

**Problem**: `pnpm plugins:extract` fails or times out

**Common Causes:**
1. **Plugin crashes during scanning**
2. **Insufficient permissions**
3. **Plugin host binary missing**
4. **Unsupported plugin formats**

**Solutions:**

```bash
# Check plugin health first
pnpm plugins:health --verbose

# Extract with force to bypass cache
pnpm plugins:extract:force

# Extract specific plugin to isolate issue
pnpm plugins:extract --plugin "PluginName"

# Check available plugins
pnpm plugins:list --status missing
```

**Advanced Debugging:**
```bash
# Manual plugin host test (if available)
cd tools/juce-plugin-host
./plughost --scan

# Check plugin installation directories
ls -la /Library/Audio/Plug-Ins/VST3/
ls -la ~/.vst3/
```

### No Plugins Found

**Problem**: Plugin extraction reports no plugins found

**Diagnosis:**
```bash
# Check plugin directories
pnpm plugins:list --verbose

# Verify plugin formats supported
pnpm plugins:extract --help
```

**Solutions:**
1. **Install audio plugins** in standard locations
2. **Check plugin formats** (VST3, AU, VST2)
3. **Verify plugin permissions** (readable by current user)
4. **Update plugin host** if needed

### Plugin Crashes

**Problem**: Specific plugins crash during extraction

**Workaround:**
```bash
# Extract all except problematic plugins
pnpm plugins:extract --exclude "UAD,Native Instruments"

# Extract one plugin at a time
for plugin in $(pnpm plugins:list --format json | jq -r '.[].name'); do
  echo "Extracting: $plugin"
  pnpm plugins:extract --plugin "$plugin" || echo "Failed: $plugin"
done
```

### Extraction Performance Issues

**Problem**: Plugin extraction is very slow

**Optimization:**
```bash
# Use cache whenever possible (avoid --force)
pnpm plugins:extract

# Extract only updated plugins
pnpm plugins:extract --since "1 day ago"

# Parallel extraction (if supported)
pnpm plugins:extract --parallel 4
```

## Phase 2: Canonical Mapping Issues

### Mapping Validation Errors

**Problem**: `pnpm maps:validate` reports errors

**Common Issues:**

#### YAML Syntax Errors
```bash
# Check YAML syntax
pnpm maps:validate maps/my-device/my-plugin.yaml --verbose

# Auto-fix common YAML issues
pnpm maps:validate --fix-yaml
```

#### Missing Plugin Descriptors
```bash
# Check if plugin descriptor exists
pnpm maps:check --verbose

# Re-extract missing plugin
pnpm plugins:extract --plugin "MissingPlugin"

# Update mapping to use correct descriptor path
```

#### Invalid Parameter References
```bash
# Cross-validate with plugin descriptor
pnpm maps:check maps/my-device/my-plugin.yaml

# List available parameters
pnpm plugins:list --plugin "PluginName" --show-parameters
```

### Schema Validation Failures

**Problem**: Mapping files fail Zod schema validation

**Solutions:**

```bash
# Validate with detailed error messages
pnpm maps:validate --strict --verbose

# Check against latest schema
pnpm maps:validate --schema tools/types/canonical-mapping.schema.json

# Auto-migrate old format mappings
pnpm maps:migrate --from-version 0.9 --to-version 1.0
```

### MIDI CC Conflicts

**Problem**: Multiple controls mapped to same MIDI CC

**Detection:**
```bash
# Check for CC conflicts
pnpm maps:check --detect-conflicts

# List all CC assignments
pnpm maps:list --show-cc-usage
```

**Resolution:**
1. **Review mapping file** and reassign conflicting CCs
2. **Use different MIDI channels** if hardware supports it
3. **Group related controls** logically

### Parameter Index Mismatches

**Problem**: Plugin parameter indices don't match descriptor

**Diagnosis:**
```bash
# Compare mapping with current plugin descriptor
pnpm maps:check --cross-validate

# Show parameter changes
pnpm plugins:extract --plugin "PluginName" --compare-with-existing
```

**Solutions:**
1. **Re-extract plugin** (parameters may have changed)
2. **Update mapping file** with correct indices
3. **Check plugin version** compatibility

## Phase 3: DAW Generation Issues

### Ardour Generation Fails

**Problem**: `pnpm daw:generate:ardour` fails

**Common Causes:**

#### XML Generation Errors
```bash
# Generate with verbose output
pnpm daw:generate:ardour --verbose

# Test with single mapping
pnpm daw:generate:ardour --mapping maps/test/simple.yaml

# Validate generated XML
pnpm daw:generate:ardour --validate-xml
```

#### Installation Issues
```bash
# Check Ardour config directory
ls -la ~/.config/ardour8/midi_maps/

# Generate without installation first
pnpm daw:generate:ardour --no-install

# Manual installation
cp generated/ardour-maps/*.map ~/.config/ardour8/midi_maps/
```

### Generated Files Invalid

**Problem**: DAW doesn't recognize generated mapping files

**Validation:**
```bash
# Validate generated files
pnpm daw:list --validate

# Check file permissions
ls -la generated/ardour-maps/

# Test with minimal mapping
pnpm daw:generate:ardour --template minimal
```

### Missing Dependencies

**Problem**: DAW generation requires missing tools or libraries

**Solutions:**
```bash
# Check dependencies
pnpm daw:generate --check-dependencies

# Install missing tools
npm install -g xml-formatter

# Update tool versions
pnpm update
```

## Workflow Management Issues

### Complete Workflow Fails

**Problem**: `pnpm workflow:complete` fails at specific step

**Step-by-Step Debugging:**
```bash
# Run each phase individually
pnpm plugins:extract --verbose
pnpm maps:validate --verbose
pnpm daw:generate --verbose

# Check between phases
pnpm plugins:health
pnpm maps:check
pnpm daw:list --validate
```

### Performance Issues

**Problem**: Workflow takes too long to complete

**Optimization:**
```bash
# Profile workflow execution
pnpm workflow:complete --profile

# Use cached data
pnpm workflow:complete --use-cache

# Skip unnecessary steps
pnpm workflow:complete --skip-unchanged
```

### Health Check Failures

**Problem**: `pnpm workflow:health` reports low scores

**Investigation:**
```bash
# Detailed health breakdown
pnpm workflow:health --breakdown

# Fix specific issues
pnpm workflow:health --fix --category "plugin-coverage"

# Monitor health over time
pnpm workflow:health --log-to-file health.log
```

## Common Error Messages

### "Plugin descriptor not found"
```bash
# Re-extract missing plugin
pnpm plugins:extract --plugin "PluginName"

# Check descriptor path in mapping
grep -n "descriptor:" maps/device/plugin.yaml

# Update path if moved
sed -i 's|old/path|new/path|g' maps/device/plugin.yaml
```

### "Parameter index out of range"
```bash
# Check plugin parameters
pnpm plugins:list --plugin "PluginName" --show-parameters

# Update mapping with valid indices
pnpm maps:validate --fix-indices maps/device/plugin.yaml
```

### "MIDI CC out of range"
```bash
# Validate CC ranges (should be 0-127)
pnpm maps:validate --check-cc-range

# Fix invalid CC values
pnpm maps:validate --fix-cc-range maps/device/plugin.yaml
```

### "DAW installation directory not found"
```bash
# Check DAW installation
pnpm daw:generate:ardour --check-installation

# Specify custom directory
pnpm daw:generate:ardour --install-dir ~/.config/ardour8/midi_maps/

# Create directory if missing
mkdir -p ~/.config/ardour8/midi_maps/
```

## Environment Issues

### Node.js/TypeScript Issues

**Problem**: Scripts fail with TypeScript compilation errors

**Solutions:**
```bash
# Check TypeScript version
npx tsc --version

# Reinstall dependencies
pnpm install

# Clear cache
pnpm store prune

# Check tsconfig
npx tsc --showConfig
```

### File Permission Issues

**Problem**: Cannot read/write files or directories

**Solutions:**
```bash
# Check file permissions
ls -la plugin-descriptors/
ls -la maps/
ls -la generated/

# Fix permissions
chmod 755 tools/**/*.ts
chmod 644 maps/**/*.yaml
chmod 644 plugin-descriptors/**/*.json

# Check directory permissions
chmod 755 plugin-descriptors/ maps/ generated/
```

### Path Issues

**Problem**: Tools can't find required files or directories

**Solutions:**
```bash
# Verify working directory
pwd
# Should be: /path/to/audio-control

# Check file structure
ls -la tools/ maps/ plugin-descriptors/ generated/

# Reset paths if needed
cd /path/to/audio-control
```

## Advanced Debugging

### Enable Debug Logging

Set environment variables for detailed logging:

```bash
# Enable debug mode
export DEBUG_AUDIO_CONTROL=true

# Increase log level
export LOG_LEVEL=debug

# Run with debug output
pnpm workflow:complete
```

### Tool-Specific Debugging

```bash
# Plugin extraction debug
export DEBUG_PLUGIN_EXTRACTION=true
pnpm plugins:extract --verbose

# Mapping validation debug
export DEBUG_MAPPING_VALIDATION=true
pnpm maps:validate --verbose

# DAW generation debug
export DEBUG_DAW_GENERATION=true
pnpm daw:generate --verbose
```

### Performance Profiling

```bash
# Profile memory usage
node --inspect tools/workflow/complete.ts

# Time execution
time pnpm workflow:complete

# Profile individual phases
hyperfine 'pnpm plugins:extract' 'pnpm maps:validate' 'pnpm daw:generate'
```

## Getting Help

### Built-in Help
```bash
# Tool-specific help
pnpm plugins:extract --help
pnpm maps:validate --help
pnpm daw:generate --help

# Workflow help
pnpm workflow:complete --help
pnpm workflow:health --help
```

### Diagnostic Information
```bash
# System information
pnpm workflow:health --system-info

# Version information
pnpm --version
node --version
npx tsc --version

# Environment check
pnpm workflow:health --check-environment
```

### Log Files

Default log locations:
- **Plugin extraction**: `logs/plugin-extraction.log`
- **Mapping validation**: `logs/mapping-validation.log`
- **DAW generation**: `logs/daw-generation.log`
- **Workflow**: `logs/workflow.log`

```bash
# View recent logs
tail -f logs/workflow.log

# Search for errors
grep -i "error" logs/*.log

# Clear old logs
rm logs/*.log
```

### Support Resources

1. **Documentation**: [docs/](./docs/)
2. **Examples**: [examples/](../examples/)
3. **Issues**: Project issue tracker
4. **Community**: Project discussions/forums

This troubleshooting guide covers the most common issues and their solutions. For complex problems, run the diagnostic commands first to gather information before attempting fixes.