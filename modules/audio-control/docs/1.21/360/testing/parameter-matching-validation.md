# End-to-End Test Plan: Fuzzy Parameter Matching Feature

**Feature:** AI-powered parameter matching between hardware control names and plugin parameters
**Version:** 1.21
**Module:** controller-workflow
**Last Updated:** 2025-10-12

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Test Scenarios](#test-scenarios)
4. [Implementation Checklist](#implementation-checklist)
5. [Test Commands](#test-commands)
6. [Expected vs Actual Results](#expected-vs-actual-results)
7. [Troubleshooting Guide](#troubleshooting-guide)
8. [Success Criteria](#success-criteria)

---

## Overview

This document provides comprehensive end-to-end testing procedures for the fuzzy parameter matching feature, which uses Claude AI to intelligently map hardware controller control names to audio plugin parameters.

### Feature Description

The parameter matching feature enables:
- **Semantic matching** of control names to plugin parameters (e.g., "Filter Cutoff" → "Cutoff Frequency")
- **Abbreviation recognition** (e.g., "Res" → "Resonance", "Env" → "Envelope Attack")
- **Confidence scoring** for each match (0.0 to 1.0 scale)
- **Graceful degradation** when Claude CLI is unavailable or matching fails
- **Plugin descriptor integration** using canonical-midi-maps registry

### Testing Objectives

1. **Validate core matching logic** with real hardware and plugin data
2. **Verify error handling** for edge cases and failure modes
3. **Measure performance** and ensure acceptable response times
4. **Confirm output quality** in canonical YAML and DAW-specific formats

---

## Prerequisites

### Required Hardware

- ✅ **Novation Launch Control XL 3** connected via USB
  - Firmware: 1.0.10.84 or later
  - Custom mode configured with named controls
  - Recommended slot: Slot 0 with "Jupiter 8" configuration

### Required Software

- ✅ **Node.js** 18.x or later
- ✅ **pnpm** package manager (v8.x or later)
- ✅ **Claude Code CLI** installed and authenticated
  - Verify: `claude --version` should return version info
  - Verify: `echo "test" | claude` should respond without errors
- ✅ **Audio plugin** installed (for testing)
  - Recommended: TAL-J-8 (TAL Software)
  - Alternative: Any VST/AU with known parameter list

### Required Data Files

- ✅ **Plugin descriptor** in canonical-midi-maps registry
  - Location: `modules/canonical-midi-maps/plugin-descriptors/`
  - Format: `{manufacturer}-{plugin-name}.json`
  - Example: `tal-togu-audio-line-tal-j-8.json`
  - Structure:
    ```json
    {
      "plugin": {
        "manufacturer": "TAL Software",
        "name": "TAL-J-8",
        "format": "VST3"
      },
      "parameters": [
        {
          "index": 0,
          "name": "Cutoff",
          "group": "Filter",
          "min": 0,
          "max": 1
        }
      ]
    }
    ```

### Environment Setup

```bash
# Navigate to controller-workflow
cd modules/audio-control/modules/controller-workflow

# Install dependencies
pnpm install

# Build project
pnpm build

# Verify CLI is available
pnpm cli-deploy --help

# Verify Claude CLI
claude --version
```

---

## Test Scenarios

### Scenario 1: Basic Parameter Matching

**Objective:** Validate core AI matching functionality with real hardware configuration.

**Hardware Setup:**
- Device: Launch Control XL 3
- Slot: 0 (Jupiter 8 custom mode)
- Controls: 48 total (24 encoders, 8 sliders, 16 buttons)
- Named controls: All controls have descriptive names

**Plugin Setup:**
- Plugin: TAL-J-8
- Parameters: 2234 total parameters
- Descriptor: Available in canonical-midi-maps

**Expected Results:**
- ✅ 48 controls read from hardware
- ✅ Claude AI successfully invoked
- ✅ Matches found with confidence > 0.6
- ✅ Parameter indices added to canonical YAML
- ✅ Ardour XML includes plugin-specific URIs
- ✅ Total time: < 10 seconds

**Test Procedure:**
```bash
# Step 1: List slots to verify hardware connection
pnpm cli-deploy list

# Step 2: Deploy with AI matching
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --output ./test-output

# Step 3: Verify output files
ls -la ./test-output
cat ./test-output/jupiter_8.yaml | head -50
cat ./test-output/jupiter_8.map | head -50
```

**Success Criteria:**
- [ ] Hardware detected and connected
- [ ] All 48 controls read successfully
- [ ] Plugin descriptor loaded (2234 parameters)
- [ ] AI matching completed without errors
- [ ] Match rate > 70% (at least 34/48 controls matched)
- [ ] Average confidence > 0.7
- [ ] Canonical YAML contains `plugin_parameter` fields
- [ ] Ardour XML contains plugin-specific URIs

---

### Scenario 2: Error Handling - Non-Existent Plugin

**Objective:** Verify graceful degradation when plugin descriptor is not found.

**Test Configuration:**
- Plugin: "NonExistentPlugin123"
- Expected: Error message, continue deployment without matching

**Test Procedure:**
```bash
# Deploy with non-existent plugin
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "NonExistentPlugin123" \
  --daw ardour \
  --output ./test-output-error1
```

**Expected Console Output:**
```
[1/3] Reading controller configuration...
     ✓ Controller: Novation Launch Control XL 3
     ✓ Configuration: "Jupiter 8" from slot 0

[1.5/3] AI-matching control names to plugin parameters...
     ⚠ AI matching failed: Plugin descriptor not found for "NonExistentPlugin123"
     ℹ Continuing without parameter matching...

[2/3] Converting to canonical format...
     ✓ Saved canonical: ./test-output-error1/jupiter_8.yaml

[3/3] Deploying to DAWs...
     ✓ Ardour: ./test-output-error1/jupiter_8.map

✅ Deployment complete!
```

**Success Criteria:**
- [ ] Warning displayed for missing plugin descriptor
- [ ] Deployment continues without AI matching
- [ ] Canonical YAML generated (without plugin_parameter fields)
- [ ] Ardour XML generated successfully
- [ ] Exit code: 0 (success)

---

### Scenario 3: Error Handling - Claude CLI Unavailable

**Objective:** Verify graceful degradation when Claude CLI is not available.

**Test Configuration:**
- Temporarily rename or remove `claude` from PATH
- Expected: Error message, continue deployment without matching

**Test Procedure:**
```bash
# Temporarily disable Claude CLI (macOS/Linux)
export PATH_BACKUP="$PATH"
export PATH="/usr/bin:/bin"  # Minimal PATH without claude

# Run deployment
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --output ./test-output-error2

# Restore PATH
export PATH="$PATH_BACKUP"
```

**Expected Console Output:**
```
[1.5/3] AI-matching control names to plugin parameters...
     ✓ Loaded plugin descriptor: TAL-J-8 (2234 parameters)
     ⚠ AI matching failed: Failed to spawn Claude CLI: spawn claude ENOENT
     ℹ Continuing without parameter matching...
```

**Success Criteria:**
- [ ] Plugin descriptor loaded successfully
- [ ] Error detected when spawning Claude CLI
- [ ] Warning displayed with descriptive error
- [ ] Deployment continues without AI matching
- [ ] Exit code: 0 (success)

---

### Scenario 4: Error Handling - Malformed Claude Response

**Objective:** Verify error handling when Claude returns unparseable response.

**Test Configuration:**
- This scenario requires mocking or instrumentation
- Simulate malformed JSON response from Claude

**Expected Behavior:**
- Error message: "Could not find JSON array in Claude response"
- Deployment continues without matching
- Exit code: 0 (success)

**Success Criteria:**
- [ ] JSON parsing error detected
- [ ] Descriptive error message displayed
- [ ] Deployment completes successfully
- [ ] No crash or unhandled exceptions

---

### Scenario 5: Performance Benchmarking

**Objective:** Measure and validate performance of AI matching workflow.

**Test Configuration:**
- Plugin: TAL-J-8 (2234 parameters)
- Controls: 48 named controls
- Target: < 10 seconds total workflow time

**Test Procedure:**
```bash
# Run with time measurement
time pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --output ./test-output-perf
```

**Performance Targets:**

| Operation | Target | Measurement |
|-----------|--------|-------------|
| Hardware connection | < 2s | __________ |
| Slot read | < 1s | __________ |
| Plugin descriptor load | < 100ms | __________ |
| Claude AI matching | < 5s | __________ |
| Canonical conversion | < 100ms | __________ |
| Ardour XML generation | < 50ms | __________ |
| **Total workflow** | **< 10s** | __________ |

**Success Criteria:**
- [ ] Total workflow time < 10 seconds
- [ ] Claude AI response time < 5 seconds
- [ ] All operations complete within target times
- [ ] No performance regressions vs baseline

---

### Scenario 6: Output Validation - Canonical YAML

**Objective:** Verify canonical YAML output includes plugin parameter mappings.

**Test Procedure:**
```bash
# Deploy with plugin matching
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --output ./test-output-validation

# Inspect canonical YAML
cat ./test-output-validation/jupiter_8.yaml
```

**Expected YAML Structure (with AI matching):**
```yaml
version: "1.0"
controller:
  manufacturer: Novation
  model: Launch Control XL 3
plugin:
  manufacturer: TAL Software
  name: TAL-J-8
midi:
  channel: 15
controls:
  - id: encoder_1
    type: encoder
    label: "Filter Cutoff"
    midi_cc: 13
    plugin_parameter: 42  # ← AI-matched parameter index
    behavior: absolute

  - id: encoder_2
    type: encoder
    label: "Resonance"
    midi_cc: 29
    plugin_parameter: 43  # ← AI-matched parameter index
    behavior: absolute
```

**Expected YAML Structure (without AI matching):**
```yaml
controls:
  - id: encoder_1
    type: encoder
    label: "Filter Cutoff"
    midi_cc: 13
    # No plugin_parameter field
    behavior: absolute
```

**Success Criteria:**
- [ ] YAML is valid and well-formed
- [ ] Plugin info included in metadata
- [ ] Matched controls have `plugin_parameter` field
- [ ] Unmatched controls omit `plugin_parameter` field
- [ ] All required fields present (id, type, label, midi_cc)

---

### Scenario 7: Output Validation - Ardour XML

**Objective:** Verify Ardour XML includes plugin-specific parameter URIs.

**Test Procedure:**
```bash
# Inspect Ardour XML
cat ./test-output-validation/jupiter_8.map
```

**Expected XML Structure (with AI matching):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Jupiter 8">
  <DeviceInfo bank-size="0"/>

  <!-- Matched control with plugin URI -->
  <Binding channel="15" ctl="13"
           uri="/route/plugin/parameter B1 42"
           note="Filter Cutoff"/>

  <!-- Matched control with plugin URI -->
  <Binding channel="15" ctl="29"
           uri="/route/plugin/parameter B1 43"
           note="Resonance"/>
</ArdourMIDIBindings>
```

**Expected XML Structure (without AI matching):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<ArdourMIDIBindings version="1.0.0" name="Jupiter 8">
  <DeviceInfo bank-size="0"/>

  <!-- Unmatched control with generic URI -->
  <Binding channel="15" ctl="13"
           uri="/route/send/gain B1 0"
           note="Filter Cutoff"/>
</ArdourMIDIBindings>
```

**Success Criteria:**
- [ ] XML is valid and well-formed
- [ ] Matched controls use plugin-specific URIs
- [ ] URI format: `/route/plugin/parameter B1 {paramIndex}`
- [ ] Note attribute contains control label
- [ ] All MIDI CC bindings present

---

### Scenario 8: Comparison - With vs Without Plugin Flag

**Objective:** Compare deployment outputs with and without AI matching.

**Test Procedure:**
```bash
# Deploy WITHOUT plugin matching (baseline)
pnpm cli-deploy deploy \
  --slot 0 \
  --daw ardour \
  --output ./test-output-baseline

# Deploy WITH plugin matching
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --output ./test-output-matched

# Compare outputs
diff ./test-output-baseline/jupiter_8.yaml ./test-output-matched/jupiter_8.yaml
diff ./test-output-baseline/jupiter_8.map ./test-output-matched/jupiter_8.map
```

**Expected Differences:**

**YAML diff:**
```diff
+ plugin:
+   manufacturer: TAL Software
+   name: TAL-J-8

  controls:
    - id: encoder_1
      type: encoder
      label: "Filter Cutoff"
      midi_cc: 13
+     plugin_parameter: 42
```

**XML diff:**
```diff
- <Binding channel="15" ctl="13" uri="/route/send/gain B1 0" note="Filter Cutoff"/>
+ <Binding channel="15" ctl="13" uri="/route/plugin/parameter B1 42" note="Filter Cutoff"/>
```

**Success Criteria:**
- [ ] Baseline output has no plugin metadata
- [ ] Matched output includes plugin info
- [ ] Plugin parameter indices added to matched controls
- [ ] Ardour URIs changed from generic to plugin-specific
- [ ] Both files are valid and functional

---

## Implementation Checklist

### Phase 1: Environment Setup
- [ ] Hardware connected and detected
- [ ] Node.js and pnpm installed
- [ ] controller-workflow built successfully
- [ ] Claude CLI installed and authenticated
- [ ] Plugin descriptor available

### Phase 2: Core Functionality Testing
- [ ] Scenario 1: Basic parameter matching ✅
- [ ] Validate match rate > 70%
- [ ] Validate confidence scores > 0.6
- [ ] Validate output file structure
- [ ] Measure baseline performance

### Phase 3: Error Handling Testing
- [ ] Scenario 2: Non-existent plugin descriptor
- [ ] Scenario 3: Claude CLI unavailable
- [ ] Scenario 4: Malformed Claude response
- [ ] Verify graceful degradation for all error cases
- [ ] Confirm exit codes and error messages

### Phase 4: Output Validation
- [ ] Scenario 6: Canonical YAML structure
- [ ] Scenario 7: Ardour XML structure
- [ ] Scenario 8: Comparison with/without plugin flag
- [ ] Validate all output formats
- [ ] Verify plugin-specific URIs

### Phase 5: Performance Validation
- [ ] Scenario 5: Performance benchmarking
- [ ] Measure all operation timings
- [ ] Validate total workflow < 10s
- [ ] Identify optimization opportunities

### Phase 6: Integration Testing
- [ ] End-to-end workflow validation
- [ ] Multi-DAW deployment (Ardour + Live)
- [ ] Multiple plugin descriptors
- [ ] Various controller configurations

### Phase 7: Documentation
- [ ] Update user guide with --plugin flag usage
- [ ] Document plugin descriptor creation process
- [ ] Add troubleshooting section
- [ ] Create example workflows

---

## Test Commands

### Quick Reference

```bash
# List available slots
pnpm cli-deploy list

# Basic deployment (no AI matching)
pnpm cli-deploy deploy --slot 0 --daw ardour --output ./output

# Deployment WITH AI matching
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour --output ./output

# Multi-DAW deployment
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour live --output ./output

# Dry-run mode (no file writes)
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour --dry-run

# With auto-install to DAW directories
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour --install

# Custom MIDI channel override
pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --midi-channel 0 --daw ardour

# Full options example
pnpm cli-deploy deploy \
  --slot 0 \
  --plugin "TAL-J-8" \
  --daw ardour \
  --midi-channel 15 \
  --output ./custom-output \
  --install
```

### Test Data Collection Commands

```bash
# Verify Claude CLI availability
which claude
claude --version
echo "test" | claude

# Check plugin descriptors
ls -la modules/canonical-midi-maps/plugin-descriptors/

# Inspect plugin descriptor
cat modules/canonical-midi-maps/plugin-descriptors/tal-togu-audio-line-tal-j-8.json | jq .

# Verify hardware connection
pnpm cli-deploy list | grep "Launch Control XL"

# Performance measurement
time pnpm cli-deploy deploy --slot 0 --plugin "TAL-J-8" --daw ardour

# File size comparison
wc -l output/*.yaml output/*.map
```

---

## Expected vs Actual Results

### Test Execution Log

| Scenario | Expected Result | Actual Result | Status | Notes |
|----------|-----------------|---------------|--------|-------|
| **Scenario 1: Basic Matching** |
| Hardware detection | ✅ LCXL3 detected | ___________ | ⬜ | |
| Plugin descriptor load | ✅ 2234 params | ___________ | ⬜ | |
| AI matching | ✅ >70% match rate | ___________ | ⬜ | |
| Confidence scores | ✅ Avg >0.7 | ___________ | ⬜ | |
| Total time | ✅ <10 seconds | ___________ | ⬜ | |
| **Scenario 2: Non-Existent Plugin** |
| Error detection | ✅ Warning shown | ___________ | ⬜ | |
| Graceful fallback | ✅ Deployment continues | ___________ | ⬜ | |
| Exit code | ✅ 0 (success) | ___________ | ⬜ | |
| **Scenario 3: Claude CLI Unavailable** |
| Error detection | ✅ Spawn error detected | ___________ | ⬜ | |
| Graceful fallback | ✅ Deployment continues | ___________ | ⬜ | |
| Exit code | ✅ 0 (success) | ___________ | ⬜ | |
| **Scenario 5: Performance** |
| Hardware connect | ✅ <2s | ___________ | ⬜ | |
| Slot read | ✅ <1s | ___________ | ⬜ | |
| Plugin load | ✅ <100ms | ___________ | ⬜ | |
| Claude AI | ✅ <5s | ___________ | ⬜ | |
| Total workflow | ✅ <10s | ___________ | ⬜ | |
| **Scenario 6: YAML Output** |
| Plugin metadata | ✅ Present | ___________ | ⬜ | |
| plugin_parameter | ✅ In matched controls | ___________ | ⬜ | |
| Valid YAML | ✅ Parses correctly | ___________ | ⬜ | |
| **Scenario 7: Ardour XML** |
| Plugin URIs | ✅ `/route/plugin/parameter` | ___________ | ⬜ | |
| Valid XML | ✅ Parses correctly | ___________ | ⬜ | |
| **Scenario 8: Comparison** |
| YAML differences | ✅ plugin_parameter added | ___________ | ⬜ | |
| XML differences | ✅ URIs changed | ___________ | ⬜ | |

### Match Quality Analysis

**Control Name → Parameter Name Mappings**

| Control Name | Matched Parameter | Index | Confidence | Correct? |
|--------------|-------------------|-------|------------|----------|
| ___________ | ___________ | ___ | ____ | ⬜ |
| ___________ | ___________ | ___ | ____ | ⬜ |
| ___________ | ___________ | ___ | ____ | ⬜ |
| ___________ | ___________ | ___ | ____ | ⬜ |

**Low Confidence Matches (< 0.7)**

| Control Name | Matched Parameter | Confidence | Notes |
|--------------|-------------------|------------|-------|
| ___________ | ___________ | ____ | ___________ |
| ___________ | ___________ | ___ | ___________ |

---

## Troubleshooting Guide

### Issue: Claude CLI Not Found

**Symptoms:**
```
⚠ AI matching failed: Failed to spawn Claude CLI: spawn claude ENOENT
```

**Solutions:**
1. Verify Claude CLI installation: `which claude`
2. Install Claude CLI if missing: Follow installation guide at https://claude.ai/cli
3. Verify authentication: `claude --version`
4. Check PATH: `echo $PATH` should include Claude CLI directory

### Issue: Plugin Descriptor Not Found

**Symptoms:**
```
⚠ AI matching failed: Plugin descriptor not found for "PluginName"
```

**Solutions:**
1. List available descriptors:
   ```bash
   ls -la modules/canonical-midi-maps/plugin-descriptors/
   ```
2. Create missing descriptor:
   ```bash
   # Create descriptor file at:
   # modules/canonical-midi-maps/plugin-descriptors/{manufacturer}-{plugin}.json
   ```
3. Verify descriptor structure matches schema
4. Use exact plugin name or normalized name (lowercase, hyphens)

### Issue: Low Match Rate

**Symptoms:**
- Match rate < 50%
- Many controls with confidence < 0.6

**Possible Causes:**
1. Control names are too generic (e.g., "Knob 1", "Encoder A")
2. Plugin parameter names don't match control names
3. Plugin descriptor has incorrect parameter names

**Solutions:**
1. Rename controls on hardware to be more descriptive
2. Update plugin descriptor with accurate parameter names
3. Lower confidence threshold (use at your own risk):
   ```typescript
   // In ParameterMatcher constructor
   this.minConfidence = 0.4; // Lower threshold
   ```

### Issue: Slow AI Matching

**Symptoms:**
- Claude AI step takes > 10 seconds
- Timeout errors

**Solutions:**
1. Check internet connectivity (Claude API requires network)
2. Increase timeout in ParameterMatcher:
   ```typescript
   this.timeout = 60000; // 60 second timeout
   ```
3. Reduce number of parameters in plugin descriptor
4. Use caching for repeated matches (future enhancement)

### Issue: Malformed JSON Response

**Symptoms:**
```
⚠ AI matching failed: Could not find JSON array in Claude response
```

**Possible Causes:**
1. Claude returned explanation text instead of JSON
2. Network issues caused truncated response
3. Prompt was unclear or ambiguous

**Solutions:**
1. Check Claude CLI is up to date: `claude --version`
2. Retry the operation (transient network issues)
3. Review Claude response in debug logs
4. Adjust system prompt for clearer instructions

### Issue: Hardware Not Detected

**Symptoms:**
```
Error: No supported controller detected
```

**Solutions:**
1. Check USB connection
2. Verify device appears in system:
   ```bash
   # macOS
   ls /dev/cu.usbmodem*

   # Linux
   ls /dev/midi*
   ```
3. Restart device (power cycle)
4. Check device is in correct mode (not in bootloader/DFU mode)

---

## Success Criteria

### Functional Requirements

- [ ] **FR-1:** AI matching completes successfully with real hardware
- [ ] **FR-2:** Match rate > 70% for well-named controls
- [ ] **FR-3:** Confidence scores accurate and meaningful
- [ ] **FR-4:** Graceful degradation when Claude CLI unavailable
- [ ] **FR-5:** Graceful degradation when plugin descriptor missing
- [ ] **FR-6:** Canonical YAML includes plugin_parameter fields
- [ ] **FR-7:** Ardour XML uses plugin-specific URIs
- [ ] **FR-8:** Comparison test shows clear differences with/without matching

### Performance Requirements

- [ ] **PR-1:** Total workflow time < 10 seconds
- [ ] **PR-2:** Claude AI matching < 5 seconds
- [ ] **PR-3:** Plugin descriptor load < 100ms
- [ ] **PR-4:** Canonical conversion < 100ms
- [ ] **PR-5:** Ardour XML generation < 50ms

### Quality Requirements

- [ ] **QR-1:** All error cases handled gracefully (no crashes)
- [ ] **QR-2:** Descriptive error messages for all failures
- [ ] **QR-3:** Exit codes correct (0 for success, 1 for critical failure)
- [ ] **QR-4:** Output files valid and well-formed
- [ ] **QR-5:** No data loss during error conditions

### Documentation Requirements

- [ ] **DR-1:** User guide updated with --plugin flag
- [ ] **DR-2:** Plugin descriptor creation documented
- [ ] **DR-3:** Troubleshooting guide complete
- [ ] **DR-4:** Example workflows provided
- [ ] **DR-5:** Performance benchmarks documented

---

## Test Report Template

### Executive Summary

**Test Date:** __________
**Tester:** __________
**Environment:** __________
**Overall Result:** ⬜ PASS / ⬜ FAIL

### Test Results Summary

| Category | Tests Run | Passed | Failed | Pass Rate |
|----------|-----------|--------|--------|-----------|
| Core Functionality | ___ | ___ | ___ | ___% |
| Error Handling | ___ | ___ | ___ | ___% |
| Output Validation | ___ | ___ | ___ | ___% |
| Performance | ___ | ___ | ___ | ___% |
| **Total** | ___ | ___ | ___ | ___% |

### Issues Found

| Issue ID | Severity | Description | Status | Notes |
|----------|----------|-------------|--------|-------|
| ___ | ___ | ___________ | ___ | ___________ |

### Performance Results

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| Total workflow | <10s | ___s | ⬜ |
| Claude AI | <5s | ___s | ⬜ |
| Match rate | >70% | ___% | ⬜ |
| Avg confidence | >0.7 | ___ | ⬜ |

### Recommendations

1. ___________
2. ___________
3. ___________

---

## Appendix

### A. Example Plugin Descriptor

```json
{
  "plugin": {
    "manufacturer": "TAL Software",
    "name": "TAL-J-8",
    "format": "VST3",
    "version": "1.0.0"
  },
  "parameters": [
    {
      "index": 0,
      "name": "Master Volume",
      "group": "Output",
      "min": 0,
      "max": 1,
      "default": 0.8
    },
    {
      "index": 42,
      "name": "Filter Cutoff",
      "group": "Filter",
      "min": 0,
      "max": 1,
      "default": 0.5
    },
    {
      "index": 43,
      "name": "Filter Resonance",
      "group": "Filter",
      "min": 0,
      "max": 1,
      "default": 0.0
    }
  ],
  "metadata": {
    "created": "2025-10-12",
    "author": "Test Team",
    "notes": "Generated from plugin introspection"
  }
}
```

### B. Example Control Names (Jupiter 8 Mode)

**Good control names (high match probability):**
- "Filter Cutoff" → High confidence match to "Filter Cutoff" or "Cutoff Frequency"
- "Resonance" → High confidence match to "Filter Resonance" or "Res"
- "Attack" → High confidence match to "Envelope Attack" or "VCA Attack"
- "Decay" → High confidence match to "Envelope Decay"
- "OSC 1 Tune" → High confidence match to "Oscillator 1 Tune" or "OSC1 Pitch"

**Poor control names (low match probability):**
- "Knob 1" → Ambiguous, likely no match
- "A" → Too generic, likely no match
- "Send 1" → May match send controls but not plugin parameters

### C. Related Documentation

- [Feature 360 Goal](../goal.md)
- [Main Workplan](../implementation/workplan.md)
- [Hardware Validation Report](../hardware-validation-report.md)
- [Architecture Documentation](../architecture.md)
- [User Workflow Guide](../workflow.md)
- [controller-workflow README](../../../modules/controller-workflow/README.md)
- [canonical-midi-maps Plugin Descriptors](../../../modules/canonical-midi-maps/plugin-descriptors/README.md)

---

**Document Version:** 1.0
**Last Updated:** 2025-10-12
**Next Review:** 2025-10-19
