# Auto-Deploy Guide - Feature 360

**Version:** 1.21
**Last Updated:** 2025-10-12
**Status:** Phase 11 Complete

## Overview

Auto-Deploy is the "one-click" deployment feature for Feature 360. Instead of deploying each controller slot individually, Auto-Deploy:

1. **Scans all 16 slots** in your connected controller
2. **Identifies the plugin** each slot is designed for using Claude AI
3. **Matches parameters** automatically for identified plugins
4. **Deploys to all DAWs** (Ardour + Live) simultaneously

**Command:**
```bash
npx controller-deploy auto-deploy
```

That's it! One command deploys everything.

---

## How It Works

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│              Auto-Deploy Pipeline (Phase 11)                 │
└─────────────────────────────────────────────────────────────┘

1. SCAN                    2. IDENTIFY               3. MATCH
   ↓                          ↓                         ↓
┌──────────────┐       ┌──────────────┐        ┌──────────────┐
│ Read All 16  │────▶  │ PluginID AI  │───────▶│ Parameter    │
│ Controller   │       │ (Claude)     │        │ Matcher      │
│ Slots        │       │ confidence   │        │ (Claude)     │
└──────────────┘       └──────────────┘        └──────────────┘
                              │                        │
                              ↓                        ↓
                       ┌──────────────┐        ┌──────────────┐
                       │ Plugin       │        │ Canonical    │
                       │ Registry     │        │ MIDI Map     │
                       │ (138 plugins)│        │ w/ params    │
                       └──────────────┘        └──────────────┘

4. DEPLOY
   ↓
┌──────────────┐       ┌──────────────┐        ┌──────────────┐
│ Ardour XML   │       │ Live JSON    │        │ More DAWs... │
│ Generation   │       │ Generation   │        │ (Future)     │
└──────────────┘       └──────────────┘        └──────────────┘
```

### Components

#### 1. PluginRegistry
**File:** `src/services/PluginRegistry.ts` (216 lines)

- Scans `canonical-midi-maps/plugin-descriptors/` directory
- Catalogs 138 available plugins
- Provides fast lookup by name, manufacturer, tags

**What it does:**
```typescript
const registry = await PluginRegistry.create();
const plugins = registry.getAllPlugins(); // 138 plugins
const talJ8 = registry.findByName('TAL-J-8');
```

#### 2. PluginIdentifier
**File:** `src/services/PluginIdentifier.ts` (240 lines)

- Uses Claude Code CLI for AI-powered plugin identification
- Analyzes control names to identify which plugin the slot is designed for
- Returns confidence scores (0-1) and reasoning

**What it does:**
```typescript
const identifier = PluginIdentifier.create(registry);
const result = await identifier.identifyPlugin(
  ['VCF Cutoff', 'VCF Resonance', 'Env Attack'],
  'Jupiter 8' // slot name
);
// result: { pluginName: 'TAL-J-8', confidence: 0.95, reasoning: '...' }
```

#### 3. AutoDeployOrchestrator
**File:** `src/orchestrator/AutoDeployOrchestrator.ts` (398 lines)

- Coordinates the entire auto-deploy process
- Processes all 16 slots in sequence
- Aggregates results and provides summary

**What it does:**
```typescript
const orchestrator = AutoDeployOrchestrator.create(
  adapter, converter, pluginIdentifier, parameterMatcher
);

const result = await orchestrator.deploy({
  deployers: [ardourDeployer, liveDeployer],
  outputDir: './output',
  minConfidence: 0.7
});
// result: { totalSlots: 16, slotsDeployed: 12, ... }
```

---

## Prerequisites

### Required

1. **Hardware Controller**
   - Novation Launch Control XL3 connected via USB

2. **Software**
   - Node.js v20 or higher
   - Project built: `pnpm install && pnpm build`

3. **Claude Code CLI** (for AI identification)
   ```bash
   npm install -g @anthropic-ai/claude-cli
   claude auth login
   ```

### Optional

4. **Custom Labels** (recommended for better matches)
   - Set meaningful control labels in Novation Components
   - Example: "VCF Cutoff" instead of "Control 16"

---

## Usage

### Basic Usage

Deploy all populated slots with default settings:

```bash
npx controller-deploy auto-deploy
```

**What happens:**
1. Scans all 16 slots
2. Identifies plugins for each slot (AI)
3. Matches parameters for identified plugins (AI)
4. Deploys to Ardour + Live
5. Saves files to `./output/`

**Expected Output:**
```
🎛️  Auto-Deploy - Deploying All Controller Slots

⏳ Initializing services...

   ✓ Plugin registry loaded: 138 plugins available

🚀 Starting auto-deployment...

╔══════════════════════════════════════════════════════════╗
║          Auto-Deploy Complete                           ║
╚══════════════════════════════════════════════════════════╝

  📊 Summary:
     Total slots: 16
     Populated: 15
     Processed: 12
     Deployed: 12
     Total time: 145.3s

  ✅ Successfully Deployed:
     [0] Jupiter 8
         Plugin: TAL-J-8 (95% confidence)
         Parameters: 24/48 matched
         DAWs: Ardour, Live
         Time: 12.3s

     [1] Mini V4
         Plugin: Arturia Mini V3 (88% confidence)
         Parameters: 16/48 matched
         DAWs: Ardour, Live
         Time: 10.1s

     ... (10 more slots)

  ⚠️  Skipped/Failed:
     [5] Cutoffj
         Reason: Low confidence match (0.45)

     [12] TestMode
         Reason: No meaningful control names

     [15] Empty
         Reason: Slot is empty
```

---

### Advanced Usage

#### Custom Output Directory

```bash
npx controller-deploy auto-deploy --output ./my-mappings
```

#### Deploy to Single DAW

```bash
npx controller-deploy auto-deploy --daw ardour
```

#### Auto-Install to DAW Directories

```bash
npx controller-deploy auto-deploy --install
```

Files automatically copied to:
- **macOS:** `~/Library/Preferences/Ardour8/midi_maps/`
- **Linux:** `~/.config/ardour8/midi_maps/`

#### Adjust Confidence Threshold

```bash
npx controller-deploy auto-deploy --min-confidence 0.8
```

Higher = more selective (default: 0.7)

#### Skip Low Confidence Matches

```bash
npx controller-deploy auto-deploy --skip-low-confidence
```

Only deploy slots with high confidence plugin identification.

#### Dry-Run Mode

```bash
npx controller-deploy auto-deploy --dry-run
```

Preview what would be deployed without writing files.

---

## CLI Reference

### auto-deploy Command

```bash
npx controller-deploy auto-deploy [options]
```

**Options:**

| Option | Description | Default |
|--------|-------------|---------|
| `-d, --daw <daws...>` | Target DAWs (ardour, live) | `ardour live` |
| `-o, --output <dir>` | Output directory | `./output` |
| `--install` | Auto-install to DAW config dirs | `false` |
| `--dry-run` | Preview without writing files | `false` |
| `--min-confidence <num>` | Min confidence (0-1) | `0.7` |
| `--skip-low-confidence` | Skip low confidence matches | `false` |

**Examples:**

```bash
# Deploy all slots (default settings)
npx controller-deploy auto-deploy

# Deploy to Ardour only with 80% confidence
npx controller-deploy auto-deploy --daw ardour --min-confidence 0.8

# Dry-run to see what would be deployed
npx controller-deploy auto-deploy --dry-run

# Deploy and install to DAW directories
npx controller-deploy auto-deploy --install

# Custom output directory
npx controller-deploy auto-deploy --output ./studio-mappings

# Skip uncertain matches
npx controller-deploy auto-deploy --skip-low-confidence
```

---

## How Plugin Identification Works

### Step 1: Control Name Analysis

Auto-Deploy extracts control names from each slot:

```yaml
# Slot 0: "Jupiter 8"
controls:
  - Encoder 1: "VCF Cutoff"
  - Encoder 2: "VCF Resonance"
  - Encoder 3: "VCF Envelope"
  - Slider 1: "Env Attack"
  - Slider 2: "Env Decay"
  - Button 1: "Filter Mode"
  ...
```

### Step 2: AI Identification

Claude Code CLI analyzes the control names:

**Prompt to Claude:**
```
You are analyzing MIDI controller configuration to identify which plugin it was designed for.

Controller Mode: "Jupiter 8"

Control Names:
1. VCF Cutoff
2. VCF Resonance
3. VCF Envelope
4. Env Attack
5. Env Decay
6. Filter Mode

Available Plugins: [138 plugins including TAL-J-8, Arturia Jupiter-8 V3, ...]

Identify which plugin this controller mode was designed for.
```

**Claude Response:**
```json
{
  "pluginName": "TAL-J-8",
  "confidence": 0.95,
  "reasoning": "Control names match Jupiter-8 architecture (VCF, VCO, etc.). Slot name 'Jupiter 8' strongly suggests Roland Jupiter-8 emulation. TAL-J-8 is most likely match.",
  "alternativeMatches": [
    {
      "pluginName": "Arturia Jupiter-8 V3",
      "confidence": 0.85,
      "reasoning": "Also emulates Jupiter-8, good alternative"
    }
  ]
}
```

### Step 3: Confidence Scoring

| Confidence | Meaning | Action |
|------------|---------|--------|
| >= 0.8 | High confidence | Deploy automatically |
| 0.6 - 0.79 | Moderate | Deploy with warning |
| < 0.6 | Low confidence | Skip (or deploy if --skip-low-confidence not set) |

---

## Parameter Matching

After plugin identification, Auto-Deploy matches control names to plugin parameters:

### Example: TAL-J-8 (2,234 parameters)

**Control Names:**
```
VCF Cutoff → Parameter 105: "VCF Cutoff Frequency" (confidence: 0.95)
VCF Resonance → Parameter 106: "VCF Resonance" (confidence: 0.98)
Env Attack → Parameter 87: "Env 1 Attack" (confidence: 0.88)
```

**Result in Canonical YAML:**
```yaml
controls:
  - id: encoder_1
    name: VCF Cutoff
    cc: 13
    plugin_parameter: 105  # Matched!
  - id: encoder_2
    name: VCF Resonance
    cc: 14
    plugin_parameter: 106  # Matched!
```

**Result in Ardour XML:**
```xml
<Binding channel="0" ctl="13" uri="TAL-J-8/param/105"/>
<Binding channel="0" ctl="14" uri="TAL-J-8/param/106"/>
```

---

## Performance

### Timing Benchmarks

**Hardware:** M1 Mac, Launch Control XL3
**Date:** 2025-10-12

| Operation | Time | Notes |
|-----------|------|-------|
| Plugin registry load | ~0.5s | One-time, 138 plugins |
| Plugin identification (per slot) | ~10-20s | Claude AI call |
| Parameter matching (per slot) | ~5-10s | Claude AI call (if plugin identified) |
| Canonical conversion | ~20ms | Fast |
| DAW deployment (per slot) | ~50ms | Fast |
| **Total (15 slots, 12 deployed)** | **~145s** | **2.4 minutes** |

### Optimization Tips

1. **Use Custom Labels**: Better labels → faster AI identification
2. **Cache Results**: Run once, reuse canonical YAML files
3. **Deploy to Single DAW**: `--daw ardour` if you don't use Live
4. **Skip Low Confidence**: `--skip-low-confidence` saves time on uncertain slots

---

## Troubleshooting

### Issue: Claude CLI Not Found

**Symptom:**
```
❌ Error: Claude CLI not found
```

**Solution:**
```bash
npm install -g @anthropic-ai/claude-cli
claude auth login
which claude  # Verify installation
```

---

### Issue: All Slots Skipped (Low Confidence)

**Symptom:**
```
⚠️  Skipped/Failed: 15 slots
     Reason: Low confidence match (<0.70)
```

**Causes:**
- Generic control names ("Control 16")
- No custom labels set
- Plugin not in descriptor database

**Solutions:**

1. **Set Custom Labels:**
   - Open [components.novationmusic.com](https://components.novationmusic.com/)
   - Edit your custom modes
   - Set meaningful labels: "VCF Cutoff", "Env Attack", etc.
   - Upload to hardware

2. **Lower Confidence Threshold:**
   ```bash
   npx controller-deploy auto-deploy --min-confidence 0.5
   ```

3. **Check Available Plugins:**
   ```bash
   ls modules/canonical-midi-maps/plugin-descriptors/ | wc -l
   # Should show 138+
   ```

---

### Issue: Parameter Matching Failed

**Symptom:**
```
⚠ Parameter matching failed for slot 3
```

**Causes:**
- Plugin descriptor not found
- Claude CLI timeout
- Network issues

**Solutions:**

1. **Verify Plugin Descriptor Exists:**
   ```bash
   ls modules/canonical-midi-maps/plugin-descriptors/tal-*
   ```

2. **Check Claude CLI:**
   ```bash
   claude --version
   claude auth status
   ```

3. **Retry:**
   Auto-Deploy continues even if parameter matching fails. Deployment uses generic parameter bindings.

---

### Issue: Deployment Takes Too Long

**Symptom:** Auto-deploy runs for >5 minutes

**Causes:**
- Many populated slots (15-16)
- Slow Claude AI responses
- Network latency

**Solutions:**

1. **Deploy Single Slots:**
   Use regular `deploy` command for specific slots:
   ```bash
   npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
   ```

2. **Skip Low Confidence:**
   ```bash
   npx controller-deploy auto-deploy --skip-low-confidence
   ```

3. **Deploy to Single DAW:**
   ```bash
   npx controller-deploy auto-deploy --daw ardour
   ```

---

## Comparison: deploy vs auto-deploy

### Regular `deploy` Command

**Use when:**
- Deploying a single specific slot
- You know the plugin name
- You want fine control

**Example:**
```bash
npx controller-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour
```

**Time:** ~10-15 seconds per slot

---

### `auto-deploy` Command

**Use when:**
- Deploying multiple/all slots at once
- You don't know which plugin each slot is for
- You want "one-click" deployment

**Example:**
```bash
npx controller-deploy auto-deploy
```

**Time:** ~2-3 minutes for 12-15 slots

---

## Best Practices

### 1. Organize Your Slots

**Good Organization:**
```
Slot 0: Jupiter 8 (TAL-J-8)
Slot 1: Mini V4 (Arturia Mini V3)
Slot 2: Prophet (Arturia Prophet V3)
Slot 3: Juno-60 (TAL-U-NO-LX)
...
```

**Benefits:**
- Easier to identify plugins
- Better AI confidence scores
- Clearer slot management

---

### 2. Use Descriptive Slot Names

**Good:**
```
"Jupiter 8" → TAL-J-8 identified with 95% confidence
"Mini V4" → Arturia Mini V3 identified with 90% confidence
```

**Bad:**
```
"Config 1" → Unknown plugin (45% confidence)
"Test" → No match
```

---

### 3. Set Meaningful Control Labels

**Good Labels:**
```
VCF Cutoff, VCF Resonance, VCO 1 Frequency, Env Attack
```

**Bad Labels:**
```
Control 16, Knob 1, Encoder A
```

**Impact:**
- Good labels: 24/48 controls matched (50%)
- Bad labels: 0/48 controls matched (0%)

---

## Advanced Topics

### Plugin Descriptor Database

Auto-Deploy relies on plugin descriptors in:
```
modules/canonical-midi-maps/plugin-descriptors/
```

**Current Database:**
- 138 plugins available
- Covers: TAL, Arturia, Full Bucket Music, Analog Obsession, more

**Adding New Plugins:**

See [workflow.md](./workflow.md) Phase 1 for plugin interrogation instructions.

---

### Extending Auto-Deploy

#### Add Support for New Controllers

1. Create controller adapter implementing `ControllerAdapterInterface`
2. Create converter implementing `CanonicalConverterInterface`
3. Update `detectController()` in `deploy.ts`

#### Add Support for New DAWs

1. Create deployer implementing `DAWDeployerInterface`
2. Update `createDeployer()` in `deploy.ts`
3. Add file extension mapping to `getFileExtension()`

---

## Next Steps

### After Auto-Deploy

1. **Test in DAW:**
   - Load maps in Ardour/Live
   - Verify controls work correctly
   - Adjust parameters if needed

2. **Refine Mappings:**
   - If AI matching was wrong, manually edit canonical YAML
   - Redeploy using regular `deploy` command

3. **Share Mappings:**
   - Save canonical YAML files to version control
   - Share with team/community

---

## Related Documentation

- **[README.md](./README.md)** - Feature 360 overview
- **[quick-start.md](./quick-start.md)** - 5-minute tutorial
- **[workflow.md](./workflow.md)** - Complete 3-phase workflow
- **[architecture.md](./architecture.md)** - System architecture
- **[troubleshooting.md](./troubleshooting.md)** - Common issues

---

**Last Updated:** 2025-10-12
**Maintained by:** Audio Control Team
**Phase:** 11 (Auto-Deploy) Complete
**Status:** ✅ Operational
