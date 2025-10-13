# Feature 360 Troubleshooting Guide

**Version:** 1.21
**Status:** Current
**Last Updated:** 2025-10-12

## Overview

This guide covers common issues encountered when using Feature 360 (MIDI Controller → DAW Deployment Pipeline). Issues are organized by phase of the workflow and include symptoms, causes, solutions, and prevention strategies.

## Quick Diagnostics

Before troubleshooting specific issues, run these quick checks:

```bash
# Check device connection
pnpm cli-deploy list

# Verify TypeScript compilation
pnpm build

# Run test suite
pnpm test

# Check Claude CLI installation (for AI matching)
which claude
claude --version
```

## Common Issues by Phase

### Phase 1: Device Connection

#### Issue 1.1: Device Not Detected

**Symptom:**
```
❌ Error: No supported controller detected. Ensure your controller is connected via USB.
Supported controllers: Novation Launch Control XL 3
```

**Cause:**
- Hardware not connected via USB
- Wrong USB port or cable
- Device in wrong mode (not in Custom Mode)
- MIDI backend initialization failure

**Solution:**

1. **Verify physical connection:**
   ```bash
   # macOS: Check if device appears in Audio MIDI Setup
   # Linux: Check /proc/asound/cards or use aconnect -l
   # Windows: Check Device Manager
   ```

2. **Check device mode:**
   - Launch Control XL3 should be in Custom Mode (not User or Factory mode)
   - Press and hold "Device" button, then press pad to select Custom Mode

3. **Try different USB port:**
   - Use a direct USB connection (not through hub if possible)
   - Try different USB cable (data cable, not charge-only)

4. **Restart device:**
   - Unplug USB cable
   - Wait 5 seconds
   - Reconnect

5. **Check MIDI permissions (macOS):**
   - System Preferences → Security & Privacy → Privacy → Automation
   - Ensure Terminal/your app has MIDI access

**Prevention:**
- Always connect device before running commands
- Use reliable USB cables
- Keep device in Custom Mode when using Feature 360

---

#### Issue 1.2: Connection Timeout

**Symptom:**
```
❌ Error: Connection timeout after 5000ms
```

**Cause:**
- Device responding slowly
- USB communication interference
- Firmware issues
- Another application holding MIDI port

**Solution:**

1. **Check for port conflicts:**
   ```bash
   # Close other MIDI applications (DAWs, Novation Components, etc.)
   # Restart device after closing applications
   ```

2. **Update firmware:**
   - Download Novation Components
   - Check for firmware updates
   - Current validated version: 1.0.10.84

3. **Increase timeout (if needed):**
   - Not user-configurable currently
   - Report issue if persistent

**Prevention:**
- Close DAWs and MIDI applications before running deployment
- Keep firmware up to date
- Use device exclusively during deployment

---

### Phase 2: Slot Reading

#### Issue 2.1: Empty Slot

**Symptom:**
```
❌ Error: No configuration found in slot 5
```

**Cause:**
- Slot contains no custom mode configuration
- Slot was never programmed
- Configuration corrupted
- Slot 15 (factory read-only slot)

**Solution:**

1. **List all slots to verify:**
   ```bash
   pnpm cli-deploy list
   ```

2. **Use a populated slot:**
   - Slots 0-14 are user-programmable
   - Slot 15 is factory preset (may be empty or read-only)
   - Choose a slot marked with a name in the list output

3. **Program the slot if empty:**
   - Use Novation Components web editor
   - Create custom mode configuration
   - Upload to device
   - Verify with `list` command

**Prevention:**
- Always run `list` command first to see available slots
- Document which slots contain configurations
- Avoid slot 15 for custom configurations

---

#### Issue 2.2: Failed Slot Read

**Symptom:**
```
Slot 7: Empty or failed to read
```

**Cause:**
- Communication error during read
- Slot data corrupted
- USB communication interference
- Device firmware issue

**Solution:**

1. **Retry the read operation:**
   ```bash
   # Try reading the specific slot again
   pnpm cli-deploy deploy --slot 7
   ```

2. **Check device health:**
   - List all slots - if multiple slots fail, it's a device/connection issue
   - If only one slot fails, that slot may be corrupted

3. **Re-program the slot:**
   - Use Novation Components to re-upload configuration
   - Test read again

4. **Factory reset (last resort):**
   - Use Novation Components to reset device
   - Re-upload all custom modes
   - **Warning:** This erases all custom configurations

**Prevention:**
- Avoid disconnecting device during read/write operations
- Use reliable USB connection
- Back up configurations regularly using Novation Components

---

### Phase 3: AI Parameter Matching

#### Issue 3.1: Claude CLI Not Installed

**Symptom:**
```
⚠ AI matching failed: Failed to spawn Claude CLI: spawn claude ENOENT
ℹ Continuing without parameter matching...
```

**Cause:**
- Claude Code CLI not installed
- Claude CLI not in system PATH
- Installation incomplete

**Solution:**

1. **Install Claude Code CLI:**
   ```bash
   # Install globally via npm
   npm install -g @anthropic-ai/claude-cli

   # Verify installation
   which claude
   claude --version
   ```

2. **Authenticate with Claude:**
   ```bash
   claude auth login
   # Follow browser authentication flow
   ```

3. **Verify PATH configuration:**
   ```bash
   # Add to ~/.zshrc or ~/.bashrc if needed
   export PATH="$HOME/.npm-global/bin:$PATH"
   source ~/.zshrc  # or ~/.bashrc
   ```

4. **Test Claude CLI:**
   ```bash
   echo "Hello" | claude
   # Should return a response from Claude
   ```

**Prevention:**
- Install Claude CLI as part of initial setup
- Document CLI requirement in project README
- Verify authentication before long deployments

**Note:** Deployment will continue without AI matching. You'll get generic parameter mappings instead of intelligent matches.

---

#### Issue 3.2: Claude CLI Authentication Failed

**Symptom:**
```
⚠ AI matching failed: Claude CLI exited with code 1: Authentication required
ℹ Continuing without parameter matching...
```

**Cause:**
- Not logged in to Claude Code
- Authentication token expired
- Network connectivity issue

**Solution:**

1. **Re-authenticate:**
   ```bash
   claude auth login
   # Follow browser flow
   # Confirm successful authentication
   ```

2. **Check authentication status:**
   ```bash
   claude auth status
   ```

3. **Verify network connection:**
   - Ensure internet connectivity
   - Check firewall/proxy settings
   - Claude CLI requires HTTPS access to api.anthropic.com

**Prevention:**
- Stay logged in to Claude Code
- Re-authenticate periodically if tokens expire
- Check authentication before important deployments

---

#### Issue 3.3: No Named Controls Found

**Symptom:**
```
ℹ Found 0 named controls out of 48 total controls
⚠ No named controls found, skipping AI matching
```

**Cause:**
- Controls have no custom labels (using default IDs only)
- Custom mode created without setting control names
- Labels not uploaded to hardware

**Solution:**

1. **Set custom labels in Novation Components:**
   - Open Novation Components web editor
   - Load your custom mode
   - Click on each control and set meaningful names:
     - Examples: "VCF Cutoff", "Filter Res", "Env Attack", "VCA Level"
   - Upload to hardware
   - Verify labels persisted with `list` command

2. **Use meaningful names:**
   - Match plugin parameter names when possible
   - Use audio engineering conventions
   - Examples:
     - Good: "VCF Cutoff", "Resonance", "Attack Time"
     - Poor: "Knob 1", "Control A", "Param 5"

3. **Verify labels in canonical YAML:**
   - After setting labels and re-deploying, check the generated YAML
   - Control definitions should have `name` field populated

**Prevention:**
- Set custom labels when creating custom modes
- Use descriptive, plugin-specific names for better AI matching
- Document label conventions in your custom mode

**Note:** Without custom labels, AI matching is skipped automatically to avoid wasting Claude API calls.

---

#### Issue 3.4: Generic Control Names Detected

**Symptom:**
```
⚠ All controls have generic names (Control X), skipping AI matching
ℹ Hint: Set custom labels in Novation Components to enable AI parameter matching
```

**Cause:**
- Controls labeled with generic pattern "Control 1", "Control 2", etc.
- Default naming scheme not useful for AI matching
- Mass-generated labels without customization

**Solution:**

1. **Replace generic names with specific ones:**
   - Instead of: "Control 16", "Control 17"
   - Use: "VCF Cutoff", "VCF Resonance"

2. **Follow naming best practices:**
   - Use parameter names from target plugin
   - Include module/section prefixes: "VCF", "VCA", "LFO"
   - Be specific: "Attack" instead of "Time 1"

**Prevention:**
- Avoid generic naming schemes
- Plan control mapping before creating custom mode
- Reference plugin documentation when naming controls

---

#### Issue 3.5: Claude CLI Timeout

**Symptom:**
```
⚠ AI matching failed: Claude CLI request timed out after 30000ms
ℹ Continuing without parameter matching...
```

**Cause:**
- Large plugin descriptor (thousands of parameters)
- Slow network connection
- Claude API rate limiting
- System performance issues

**Solution:**

1. **Retry the deployment:**
   - Usually succeeds on second attempt
   - Claude CLI may have been initializing on first run

2. **Reduce control count:**
   - Match smaller batches of controls
   - Focus on most important controls first

3. **Check network:**
   - Ensure stable internet connection
   - Avoid deployments during network instability

**Prevention:**
- Use plugins with reasonable parameter counts (<1000)
- Ensure good network connectivity before deployment
- First run takes longer (Claude CLI initialization)

---

#### Issue 3.6: Low Confidence Matches

**Symptom:**
```
✓ AI-matched 12/24 controls (50%)
⚠ 5 matches with confidence < 0.7:
  - "Filter Env" → VCF Envelope Amount (65%)
  - "Mod Amount" → LFO1 Amount (60%)
  ...
```

**Cause:**
- Ambiguous control names
- Multiple similar parameters in plugin
- Non-standard naming conventions
- Insufficient context for AI matching

**Solution:**

1. **Review low confidence matches:**
   - Check generated Ardour XML or canonical YAML
   - Verify parameter mappings make sense
   - Test in DAW to confirm behavior

2. **Improve control names for better matching:**
   - Be more specific: "VCF Env Amount" vs "Filter Env"
   - Include module prefixes: "OSC1 Pitch" vs "Pitch"
   - Match plugin terminology exactly when possible

3. **Accept reasonable matches:**
   - 60-70% confidence may still be correct
   - Context matters more than exact string match
   - Test mappings before deciding to change

4. **Manual correction if needed:**
   - Edit canonical YAML file
   - Change `plugin_parameter` field
   - Re-deploy to DAW

**Prevention:**
- Use specific, unambiguous control names
- Follow plugin's parameter naming conventions
- Include context in names: "VCF" vs "VCA", "LFO1" vs "LFO2"

---

### Phase 4: Plugin Descriptor Loading

#### Issue 4.1: Plugin Descriptor Not Found

**Symptom:**
```
⚠ AI matching failed: Plugin descriptor not found for "TAL-J-8"
     Searched in [...]/canonical-midi-maps/plugin-descriptors for files matching "tal-j-8"
ℹ Continuing without parameter matching...
```

**Cause:**
- Plugin descriptor file doesn't exist
- Plugin name doesn't match filename
- Typo in plugin name
- Plugin not yet catalogued

**Solution:**

1. **Check available descriptors:**
   ```bash
   ls modules/canonical-midi-maps/plugin-descriptors/
   ```

2. **Use exact plugin name from descriptor filename:**
   - Descriptor: `tal-togu-audio-line-tal-j-8.json`
   - Try: `--plugin "TAL-J-8"` or `--plugin "tal-togu-audio-line-tal-j-8"`

3. **Create descriptor if missing:**
   - See [Plugin Descriptor Creation Guide](../../ARCHITECTURE.md#plugin-descriptors)
   - Use plugin introspection tools
   - Contribute back to canonical-midi-maps module

**Prevention:**
- Check available descriptors before deployment
- Use tab-completion or exact names from `ls` output
- Document plugin descriptor creation process

---

#### Issue 4.2: Invalid Plugin Descriptor

**Symptom:**
```
⚠ AI matching failed: Invalid plugin descriptor structure: missing plugin or parameters
ℹ Continuing without parameter matching...
```

**Cause:**
- Malformed JSON in descriptor file
- Missing required fields (`plugin`, `parameters`)
- Incorrect descriptor format

**Solution:**

1. **Validate descriptor JSON:**
   ```bash
   # Check JSON syntax
   cat modules/canonical-midi-maps/plugin-descriptors/[descriptor].json | jq .
   ```

2. **Verify required fields:**
   ```json
   {
     "plugin": {
       "manufacturer": "...",
       "name": "...",
       "version": "..."
     },
     "parameters": [
       {
         "index": 0,
         "name": "...",
         "group": "..."
       }
     ]
   }
   ```

3. **Fix or recreate descriptor:**
   - Use plugin introspection tools
   - Follow canonical-midi-maps schema
   - Validate with JSON schema if available

**Prevention:**
- Use automated descriptor generation tools
- Validate descriptors before committing
- Test with small deployments first

---

### Phase 5: Canonical Conversion

#### Issue 5.1: Control Type Mapping Error

**Symptom:**
```
❌ Error: Unknown control type: [type]
```

**Cause:**
- Unsupported control type from hardware
- Malformed control data
- Firmware version mismatch

**Solution:**

1. **Check supported types:**
   - encoder (knobs)
   - slider (faders)
   - button (buttons/pads)

2. **Verify hardware data:**
   ```bash
   # Enable verbose logging (if available)
   DEBUG=* pnpm cli-deploy deploy --slot 0
   ```

3. **Report issue:**
   - Include hardware model and firmware version
   - Provide control data from error output
   - May require adapter update

**Prevention:**
- Use current firmware versions
- Report unknown control types for future support

---

### Phase 6: DAW Deployment

#### Issue 6.1: Ardour Deployment Failed

**Symptom:**
```
✗ Ardour: Failed to write file to output/config.map
```

**Cause:**
- Output directory doesn't exist
- Insufficient file permissions
- Disk space full
- Invalid output path

**Solution:**

1. **Check output directory:**
   ```bash
   # Create output directory if missing
   mkdir -p output

   # Check permissions
   ls -ld output
   chmod 755 output  # If needed
   ```

2. **Verify disk space:**
   ```bash
   df -h .
   ```

3. **Check file permissions:**
   ```bash
   # If file already exists
   ls -l output/*.map
   chmod 644 output/*.map  # If needed
   ```

4. **Use explicit output path:**
   ```bash
   pnpm cli-deploy deploy --slot 0 --output ~/my-maps
   ```

**Prevention:**
- Ensure output directory exists before deployment
- Use paths with write permissions
- Monitor disk space

---

#### Issue 6.2: Ableton Live Deployment Not Implemented

**Symptom:**
```
✗ live: Ableton Live deployment not yet implemented
```

**Cause:**
- Live deployer CLI integration pending
- Feature planned but not complete

**Solution:**

1. **Use Ardour deployment for now:**
   ```bash
   pnpm cli-deploy deploy --slot 0 --daw ardour
   ```

2. **Manual Live deployment:**
   - Review LiveDeployer implementation in `modules/live-max-cc-router`
   - Dual-pipeline architecture is complete
   - CLI integration is pending

3. **Track progress:**
   - See [status.md](./status.md) for Live integration timeline
   - Expected in next release

**Prevention:**
- Check supported DAWs in documentation
- Use Ardour deployment as current workflow

---

#### Issue 6.3: Installation Permission Denied

**Symptom:**
```
⚠ Failed to install to Ardour config directory: EACCES: permission denied
✓ File saved to output/config.map but not auto-installed
```

**Cause:**
- No write permission to DAW config directory
- Directory doesn't exist
- Protected system directory (macOS)

**Solution:**

1. **Manual installation:**
   ```bash
   # Copy file manually to Ardour config directory
   # macOS:
   cp output/config.map ~/Library/Preferences/Ardour8/midi_maps/

   # Linux:
   cp output/config.map ~/.config/ardour8/midi_maps/

   # Windows:
   copy output\config.map %APPDATA%\Ardour8\midi_maps\
   ```

2. **Create config directory if missing:**
   ```bash
   # macOS:
   mkdir -p ~/Library/Preferences/Ardour8/midi_maps/

   # Linux:
   mkdir -p ~/.config/ardour8/midi_maps/
   ```

3. **Skip auto-install:**
   - Don't use `--install` flag
   - Deploy generates files without installation
   - Install manually afterward

**Prevention:**
- Ensure DAW config directories exist before deployment
- Don't use `--install` flag if permission issues persist
- Manual installation is reliable alternative

---

### Phase 7: Validation & Testing

#### Issue 7.1: Generated Map Not Working in DAW

**Symptom:**
- Ardour loads map but controls don't work
- Controls mapped to wrong parameters
- No response from plugin

**Cause:**
- Incorrect MIDI channel mismatch
- Wrong plugin URI in bindings
- Controller sending on different channel
- Plugin not loaded in DAW

**Solution:**

1. **Verify MIDI channel:**
   ```bash
   # Check canonical YAML for global MIDI channel
   cat output/config.yaml | grep midi_channel

   # Ensure controller is sending on same channel
   # Check in Novation Components
   ```

2. **Check plugin in DAW:**
   - Ensure plugin is loaded in track/bus
   - Verify plugin name matches exactly
   - Enable plugin parameter automation

3. **Test with MIDI monitor:**
   - Use DAW's MIDI monitor to verify controller is sending
   - Confirm CC numbers match map
   - Check channel matches

4. **Reload map in DAW:**
   - Preferences → Control Surfaces → Generic MIDI
   - Remove and re-add the map
   - Restart DAW if needed

**Prevention:**
- Verify MIDI channel consistency before deployment
- Test with simple plugin first
- Use MIDI monitor during initial testing

---

#### Issue 7.2: AI-Matched Parameters Incorrect

**Symptom:**
- Controls mapped to wrong plugin parameters
- Filter control mapped to envelope
- Semantic mismatch despite high confidence

**Cause:**
- Ambiguous control names
- Plugin has multiple similar parameters
- AI matching limitation
- Control name doesn't match plugin terminology

**Solution:**

1. **Review canonical YAML:**
   ```bash
   cat output/config.yaml
   # Check plugin_parameter values
   # Verify parameter names make sense
   ```

2. **Manual correction:**
   ```yaml
   controls:
     - id: encoder_1
       name: VCF Cutoff
       plugin_parameter: 105  # Change this to correct index
       confidence: 0.95
   ```

3. **Re-deploy after correction:**
   ```bash
   pnpm cli-deploy deploy --slot 0 --daw ardour
   ```

4. **Improve control names:**
   - Use exact plugin parameter names
   - Add context: "VCF Cutoff" vs "Cutoff"
   - Re-upload to hardware and re-deploy

**Prevention:**
- Use plugin parameter names exactly
- Test mappings in DAW after deployment
- Iterate on control names for better matching

---

## Debugging Techniques

### Enable Verbose Logging

```bash
# Environment variable approach (if implemented)
DEBUG=controller-workflow:* pnpm cli-deploy deploy --slot 0

# Check test output for detailed behavior
pnpm test -- --reporter=verbose
```

### Inspect Generated Files

```bash
# Check canonical YAML structure
cat output/config.yaml

# Validate YAML syntax
npm install -g js-yaml
js-yaml output/config.yaml

# Check Ardour XML structure
cat output/config.map | xmllint --format -

# Count controls in canonical map
cat output/config.yaml | grep "^  - id:" | wc -l
```

### Test Device Connection Manually

```bash
# List MIDI devices (macOS)
ls /dev/cu.*

# Test with simpler LCXL3 commands
# (Use launch-control-xl3 library directly)
node
> const { LaunchControlXL3, NodeMidiBackend } = require('@oletizi/launch-control-xl3/node');
> const backend = new NodeMidiBackend();
> const device = new LaunchControlXL3({ midiBackend: backend });
> await device.connect();
> await device.verifyDevice();
```

### Check Plugin Descriptor Contents

```bash
# List parameters in descriptor
cat modules/canonical-midi-maps/plugin-descriptors/[plugin].json | jq '.parameters[] | "\(.index): \(.name)"'

# Count parameters
cat modules/canonical-midi-maps/plugin-descriptors/[plugin].json | jq '.parameters | length'

# Search for parameter by name
cat modules/canonical-midi-maps/plugin-descriptors/[plugin].json | jq '.parameters[] | select(.name | contains("Cutoff"))'
```

### Verify Test Coverage

```bash
# Run tests with coverage
pnpm test -- --coverage

# Check specific component
pnpm test -- ParameterMatcher

# Verify adapter tests
pnpm test -- LaunchControlXL3Adapter
```

### Manual Integration Test

```bash
# Complete workflow test
pnpm cli-deploy list
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour --output ./test-output
ls -la test-output/
cat test-output/*.yaml
cat test-output/*.map
```

## Getting Help

### Before Reporting Issues

1. **Run diagnostics:**
   - `pnpm cli-deploy list` (device connection)
   - `pnpm build` (TypeScript compilation)
   - `pnpm test` (test suite)

2. **Collect information:**
   - Hardware model and firmware version
   - Operating system and version
   - Node.js version (`node --version`)
   - Command that failed (exact command)
   - Full error output

3. **Check existing documentation:**
   - [README](./README.md) - Feature overview
   - [Workflow Guide](./workflow.md) - Process details
   - [Hardware Validation Report](./hardware-validation-report.md) - Known working configs

### Issue Template

When reporting issues, include:

```markdown
**Environment:**
- Hardware: [e.g., Novation Launch Control XL3]
- Firmware: [e.g., 1.0.10.84]
- OS: [e.g., macOS 14.6.0]
- Node.js: [e.g., v20.11.0]
- Branch: [e.g., feat/cc-mapping-360]

**Command:**
```bash
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
```

**Expected Behavior:**
[What you expected to happen]

**Actual Behavior:**
[What actually happened]

**Error Output:**
```
[Full error message]
```

**Steps to Reproduce:**
1. [First step]
2. [Second step]
3. [Third step]

**Additional Context:**
[Any other relevant information]
```

## Preventive Maintenance

### Regular Checks

1. **Keep firmware updated:**
   - Check Novation website for firmware updates
   - Current validated version: 1.0.10.84

2. **Verify Claude CLI authentication:**
   ```bash
   claude auth status
   # Re-authenticate if expired
   ```

3. **Clean output directories:**
   ```bash
   # Remove old generated files
   rm -rf output/*.yaml output/*.map
   ```

4. **Update dependencies:**
   ```bash
   pnpm install
   pnpm test  # Verify after updates
   ```

### Best Practices

- Always run `list` before `deploy` to verify slot contents
- Use meaningful control names for AI matching
- Test maps in DAW before production use
- Back up custom modes in Novation Components
- Document your custom mode configurations
- Keep output files under version control
- Use explicit MIDI channel settings when targeting specific plugins

---

## Additional Resources

- **[README](./README.md)** - Feature 360 overview and quick start
- **[Workflow Guide](./workflow.md)** - Complete 3-phase workflow
- **[Architecture](./architecture.md)** - System design and components
- **[Hardware Validation Report](./hardware-validation-report.md)** - Physical device test results
- **[AI Matching Validation](./ai-matching-validation-results.md)** - AI matching test results
- **[Status Tracking](./status.md)** - Implementation progress
- **[Module Documentation](../../../modules/)** - Component-specific guides

---

**Document Status:** Active
**Maintained by:** Audio Control Team
**Last Validated:** 2025-10-12
**Hardware Tested:** Novation Launch Control XL3 (Serial: LX280935400469)
