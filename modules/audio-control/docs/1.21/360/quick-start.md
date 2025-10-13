# Feature 360 Quick-Start Guide

**Version:** 1.21
**Last Updated:** 2025-10-12
**Status:** 95% Complete

## What is Feature 360?

Feature 360 is a complete workflow for deploying MIDI controller configurations to multiple DAWs (Digital Audio Workstations). The name "360" reflects the comprehensive, full-circle nature of the workflow:

```
Extract from Hardware → Convert to Universal Format → Deploy to Any DAW
```

**Key Benefits:**
- One-click deployment from hardware to DAW
- AI-powered parameter matching (via Claude Code CLI)
- Multiple DAW support (Ardour complete, Live integration pending)
- No manual XML/JSON editing required
- Preserves your hardware's custom labels

## Prerequisites

### Required

1. **Hardware Controller**
   - Novation Launch Control XL3 (currently supported)
   - Connected via USB

2. **Software**
   - Node.js v20 or higher
   - npm or pnpm package manager
   - Supported DAW: Ardour 8+ (Live support in progress)

3. **Project Setup**
   ```bash
   # Clone and install the audio-control project
   cd modules/audio-control/modules/controller-workflow
   pnpm install
   pnpm build
   ```

### Optional (for AI Parameter Matching)

4. **Claude Code CLI**
   ```bash
   # Install Claude Code CLI
   npm install -g @anthropic-ai/claude-cli

   # Authenticate
   claude auth login

   # Verify installation
   claude --version
   ```

5. **Plugin Descriptors**
   - Auto-generated from installed VST3 plugins
   - Located in: `modules/canonical-midi-maps/plugin-descriptors/`
   - See [workflow.md](./workflow.md) for plugin interrogation details

## 5-Minute Tutorial

### Step 1: Check Your Controller

Verify your Launch Control XL3 is connected and detected:

```bash
npx controller-deploy list
```

**Expected Output:**
```
✅ Device connected: Launch Control XL3
   Serial: LX280935400469
   Firmware: 1.0.10.84

Available Custom Mode Slots:
─────────────────────────────────────────────────────

Slot 0: Jupiter 8 (48 controls)
  • 24 encoders
  •  8 sliders
  • 16 buttons

Slot 1: Cutoffj (48 controls)
  ...

✨ Total: 15 populated slots
```

If you see this, you're ready to proceed!

### Step 2: Deploy to Ardour (Basic)

Deploy a controller slot to Ardour without AI matching:

```bash
npx controller-deploy deploy --slot 0 --daw ardour
```

**What happens:**
1. Reads configuration from slot 0 of your controller
2. Converts to canonical YAML format
3. Generates Ardour MIDI map XML
4. Saves files to `output/` directory

**Expected Output:**
```
✅ Device connected: Launch Control XL3
   Serial: LX280935400469
   Firmware: 1.0.10.84

[1/3] Reading configuration from slot 0...
✅ Configuration read: Jupiter 8
   • 24 encoders
   •  8 sliders
   • 16 buttons

[2/3] Converting to canonical format...
✅ Canonical map created
   📄 Saved: output/jupiter_8.yaml

[3/3] Deploying to DAWs...
   Ardour: ✅ Deployed
   📄 Map file: output/jupiter_8.map

🎉 Deployment complete!

Files Generated:
  • output/jupiter_8.yaml (canonical map)
  • output/jupiter_8.map (Ardour MIDI map)
```

**Typical Time:** ~6 seconds

### Step 3: Deploy with AI Matching (Advanced)

For AI-powered parameter matching, you need:
1. Claude Code CLI installed (see Prerequisites)
2. Custom labels set on your hardware (see "Setting Custom Labels" below)
3. Plugin descriptor available for your target plugin

```bash
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
```

**What happens:**
1. Reads configuration from slot 0
2. **[NEW]** Loads plugin descriptor for TAL-J-8 (2,234 parameters)
3. **[NEW]** Uses Claude AI to match control names to plugin parameters
4. Converts to canonical YAML with `plugin_parameter` indices
5. Generates Ardour XML with plugin-specific URIs

**Expected Output:**
```
✅ Device connected: Launch Control XL3

[1/3] Reading configuration from slot 0...
✅ Configuration read: Jupiter 8

[1.5/3] AI-matching control names to plugin parameters...
     ✓ Loaded plugin descriptor: TAL-J-8 (2234 parameters)
     ✓ Found 12 named controls out of 48 total controls
     ✓ Matched 12/12 controls successfully
     ✓ High confidence matches: 10 (83%)
     ⚠ Low confidence matches: 2 (confidence < 0.7)

[2/3] Converting to canonical format...
✅ Canonical map created with AI-matched parameters

[3/3] Deploying to DAWs...
   Ardour: ✅ Deployed (with plugin-specific bindings)

🎉 Deployment complete!
```

**Typical Time:** ~10-20 seconds (first run with Claude AI)

### Step 4: Install to DAW

To automatically install maps to your DAW configuration directories:

```bash
npx controller-deploy deploy --slot 0 --daw ardour --install
```

**Installation Locations:**
- **macOS:** `~/Library/Preferences/Ardour8/midi_maps/`
- **Linux:** `~/.config/ardour8/midi_maps/`
- **Windows:** `%LOCALAPPDATA%\Ardour8\midi_maps\`

### Step 5: Load Map in Ardour

1. Open Ardour
2. Go to **Edit > Preferences**
3. Select **Control Surfaces** in left sidebar
4. Enable **Generic MIDI**
5. Choose your map from the dropdown: "LCXL3 - Jupiter 8"
6. Select your MIDI input device
7. Click **Apply**

Done! Your Launch Control XL3 now controls your plugin parameters.

## Common Use Cases

### Use Case 1: Quick Deployment (No AI Matching)

**Scenario:** You just want to get your controller working with generic mappings.

```bash
# List available slots
npx controller-deploy list

# Deploy slot 2 to Ardour
npx controller-deploy deploy --slot 2 --daw ardour --install
```

**Result:** Generic parameter bindings (param/0, param/1, etc.)

**Best for:** Quick setup, testing, controllers without custom labels

---

### Use Case 2: AI-Powered Deployment (With Custom Labels)

**Scenario:** You've set custom labels on your hardware and want AI to match them to plugin parameters.

**Prerequisites:**
- Claude Code CLI installed
- Custom labels set in Novation Components (see below)
- Plugin descriptor available

```bash
# Deploy with AI matching
npx controller-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --install
```

**Result:** Plugin-specific bindings (e.g., `TAL-J-8/param/105` for "VCF Cutoff")

**Best for:** Professional setups, synth programming, precise control

---

### Use Case 3: Save for Later Review

**Scenario:** You want to review the generated files before installing.

```bash
# Deploy and save to custom directory
npx controller-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --output ./my-mappings \
  --daw ardour
```

**Result:** Files saved to `./my-mappings/` for review

**Best for:** Testing, version control, batch processing

---

### Use Case 4: Multi-Slot Deployment

**Scenario:** You have multiple plugins mapped to different slots and want to deploy all of them.

```bash
# Deploy multiple slots
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour --install
npx controller-deploy deploy --slot 1 --plugin "Mini V4" --daw ardour --install
npx controller-deploy deploy --slot 2 --plugin "Jup-8 V4" --daw ardour --install
```

**Result:** Three separate map files for three different plugins

**Best for:** Studio sessions, project-specific setups

---

### Use Case 5: Dry-Run (Preview Mode)

**Scenario:** You want to see what will be generated without actually creating files.

```bash
# Preview deployment
npx controller-deploy deploy --slot 0 --daw ardour --dry-run
```

**Result:** Console output shows what would be generated

**Best for:** Verification, debugging, learning

## Setting Custom Labels in Novation Components

For AI parameter matching to work effectively, you need to set meaningful labels on your hardware controls.

### Step 1: Open Novation Components

1. Go to [components.novationmusic.com](https://components.novationmusic.com/)
2. Connect your Launch Control XL3 via USB
3. Browser will detect the device (Chrome/Edge required)

### Step 2: Create or Edit a Custom Mode

1. Click on **Custom Modes** tab
2. Select a slot (0-14) to edit
3. Click **Edit** to enter editor mode

### Step 3: Set Control Labels

For each control you want to map:
1. Click on the encoder/slider/button
2. In the properties panel, set:
   - **CC Number:** (keep default or customize)
   - **Label/Name:** Enter descriptive name
     - ✅ Good: "VCF Cutoff", "VCF Resonance", "Env Attack"
     - ❌ Bad: "Knob1", "Control A", "Encoder 1"

**Example Labels for a Jupiter-8 Template:**

| Control | Label | CC | Parameter |
|---------|-------|----|-----------|
| Encoder 1 | VCF Cutoff | 13 | Filter cutoff frequency |
| Encoder 2 | VCF Resonance | 14 | Filter resonance |
| Encoder 3 | VCF Envelope | 15 | Filter envelope amount |
| Slider 1 | Env Attack | 53 | Envelope attack time |
| Slider 2 | Env Decay | 54 | Envelope decay time |
| Button 1 | Filter Mode | 41 | Filter mode toggle |

### Step 4: Upload to Hardware

1. Click **Upload** button
2. Wait for upload to complete (~5 seconds)
3. Verify labels on hardware display

### Step 5: Test AI Matching

```bash
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
```

AI matching will now use your custom labels to find matching parameters.

## Troubleshooting

### Controller Not Detected

**Symptom:**
```
❌ Error: No MIDI controllers detected
```

**Solutions:**
1. Check USB connection (unplug/replug)
2. Ensure controller is powered on
3. Try different USB port
4. Close other software using MIDI device (DAWs, Components)
5. Restart the controller-deploy command

---

### Slot is Empty or Failed to Read

**Symptom:**
```
Slot 5: Empty or failed to read
```

**Causes:**
- Slot genuinely empty (no custom mode programmed)
- Slot 15 is factory read-only (expected)
- USB communication issue

**Solutions:**
1. Verify slot has configuration in Novation Components
2. If slot 15: This is expected (factory read-only)
3. Try re-reading: `npx controller-deploy list`
4. Check USB cable quality

---

### AI Matching Not Running

**Symptom:**
```
⚠ No named controls found, skipping AI matching
```

**Causes:**
- Controls have no custom labels set
- All controls have generic names (Control 1, Control 2, etc.)
- Plugin descriptor not found

**Solutions:**
1. Set custom labels in Novation Components (see above)
2. Ensure labels are descriptive, not generic
3. Re-upload configuration to hardware
4. Verify plugin descriptor exists:
   ```bash
   ls modules/canonical-midi-maps/plugin-descriptors/tal-togu-audio-line-tal-j-8.json
   ```

---

### Claude Code CLI Not Found

**Symptom:**
```
❌ Failed to spawn Claude CLI: spawn claude ENOENT
```

**Causes:**
- Claude Code CLI not installed
- CLI not in system PATH

**Solutions:**
1. Install Claude Code CLI:
   ```bash
   npm install -g @anthropic-ai/claude-cli
   ```
2. Authenticate:
   ```bash
   claude auth login
   ```
3. Verify installation:
   ```bash
   which claude  # Should show path to CLI
   claude --version
   ```

**Note:** Deployment continues without AI matching if CLI is unavailable.

---

### Low Confidence Matches

**Symptom:**
```
⚠ Low confidence matches: 3 (confidence < 0.7)
```

**Causes:**
- Control names ambiguous or generic
- Multiple plugin parameters with similar names
- Control name doesn't match any parameters well

**Solutions:**
1. Use more specific control labels:
   - Instead of "Cutoff" → "VCF Cutoff"
   - Instead of "Attack" → "Env 1 Attack"
2. Review match results in output
3. Manually override in canonical YAML if needed:
   ```yaml
   controls:
     - id: encoder_1
       name: Cutoff
       cc: 13
       plugin_parameter: 105  # Override AI match
   ```

---

### Ardour Map Not Appearing

**Symptom:** Map doesn't appear in Ardour preferences dropdown.

**Solutions:**
1. Check installation path exists:
   ```bash
   ls ~/Library/Preferences/Ardour8/midi_maps/
   ```
2. Verify XML file is valid:
   ```bash
   xmllint --noout output/jupiter_8.map
   ```
3. Restart Ardour after installing map
4. Manually copy file to Ardour config directory

---

### Deployment Fails or Crashes

**Symptom:** Command exits with error.

**Solutions:**
1. Check TypeScript compilation:
   ```bash
   pnpm build
   ```
2. Verify all dependencies installed:
   ```bash
   pnpm install
   ```
3. Check Node.js version (v20+ required):
   ```bash
   node --version
   ```
4. Run with verbose logging:
   ```bash
   DEBUG=* npx controller-deploy deploy --slot 0 --daw ardour
   ```

## CLI Reference

### controller-deploy list

List all available controller slots and configurations.

```bash
npx controller-deploy list
```

**Options:** None

**Example Output:**
```
✅ Device connected: Launch Control XL3
   Serial: LX280935400469
   Firmware: 1.0.10.84

Available Custom Mode Slots:
Slot 0: Jupiter 8 (48 controls)
Slot 1: Cutoffj (48 controls)
...
```

---

### controller-deploy deploy

Deploy controller configuration to one or more DAWs.

```bash
npx controller-deploy deploy [options]
```

**Required Options:**

| Option | Description | Example |
|--------|-------------|---------|
| `--slot <number>` | Controller slot (0-14) | `--slot 0` |
| `--daw <daws...>` | Target DAW(s) | `--daw ardour` |

**Optional Options:**

| Option | Description | Example |
|--------|-------------|---------|
| `--plugin <name>` | Plugin name (enables AI matching) | `--plugin "TAL-J-8"` |
| `--output <dir>` | Output directory | `--output ./my-maps` |
| `--install` | Auto-install to DAW directories | `--install` |
| `--midi-channel <num>` | Override MIDI channel | `--midi-channel 2` |
| `--dry-run` | Preview without generating files | `--dry-run` |

**Examples:**

```bash
# Basic deployment
npx controller-deploy deploy --slot 0 --daw ardour

# With AI matching
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour

# Multiple DAWs (when Live support complete)
npx controller-deploy deploy --slot 0 --daw ardour live

# Save and install
npx controller-deploy deploy --slot 0 --output ./maps --install

# Preview only
npx controller-deploy deploy --slot 0 --daw ardour --dry-run
```

## Expected Performance

| Operation | Time | Notes |
|-----------|------|-------|
| List all slots | ~5s | 16 slots via MIDI |
| Basic deploy | ~6s | Without AI matching |
| AI-powered deploy | ~10-20s | First run (includes Claude AI) |
| Subsequent AI deploys | ~6s | Cached in canonical YAML |
| File generation | <1s | YAML and XML creation |

## Next Steps

### Learn More

- **[Full Workflow Guide](./workflow.md)** - Complete 3-phase workflow with AI matching
- **[Architecture Overview](./architecture.md)** - System design and component relationships
- **[Hardware Validation Report](./hardware-validation-report.md)** - Physical testing results
- **[AI Matching Validation](./ai-matching-validation-results.md)** - AI matching details

### Advanced Topics

- **Plugin Interrogation:** Generate plugin descriptors from VST3 plugins
- **Manual YAML Creation:** Create canonical maps by hand
- **Batch Deployment:** Script multiple controller deployments
- **Custom Converters:** Add support for new controller types

### Get Help

- **Documentation:** [README.md](./README.md)
- **Issues:** Check module-specific READMEs for known issues
- **Community:** (Project community channels)

## Hardware Requirements

### Supported Controllers

| Controller | Status | Notes |
|------------|--------|-------|
| Novation Launch Control XL3 | ✅ Complete | Reference implementation |
| Future controllers | 📋 Planned | Adapter pattern designed for easy extension |

### Validated Hardware

**Launch Control XL3:**
- Firmware: 1.0.10.84
- Serial tested: LX280935400469
- Connection: USB MIDI
- All 16 slots working
- 48 controls per slot (24 encoders + 8 sliders + 16 buttons)

## Project Status

**Feature 360:** 95% Complete

**Completed:**
- [x] Core workflow (list, deploy)
- [x] Hardware validation (Launch Control XL3)
- [x] Canonical conversion
- [x] Ardour deployment
- [x] AI parameter matching (service + CLI integration)
- [x] Test suite (249 tests, 96.5% coverage)

**In Progress:**
- [ ] Live deployer CLI integration (~3%)
- [ ] AI matching hardware validation with real plugin (~2%)

**Pending:**
- [ ] Troubleshooting guide refinement
- [ ] Performance benchmarks documentation

**Target MVP Release:** v1.0.0-beta (2025-10-15)

---

**Questions?** See [README.md](./README.md) for navigation or [workflow.md](./workflow.md) for detailed process.

**Last Updated:** 2025-10-12
**Maintained by:** Audio Control Team
**Hardware Validated:** ✅ PASSED
