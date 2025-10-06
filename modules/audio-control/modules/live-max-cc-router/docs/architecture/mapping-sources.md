# Dual-Pipeline Mapping Architecture

**Module:** `live-max-cc-router`
**Version:** 1.0
**Last Updated:** 2025-10-06

## Overview

The `live-max-cc-router` uses a **dual-pipeline mapping architecture** that combines:

1. **Tier 1: Canonical Mappings** - Build-time TypeScript constants from curated YAML sources
2. **Tier 2: Runtime Mappings** - Runtime JSON data extracted from Live devices

This architecture provides both curated defaults (Tier 1) and user-customized mappings (Tier 2) with a clear merge strategy where runtime mappings override canonical defaults.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    DUAL-PIPELINE MAPPING SYSTEM                          │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────┐  ┌──────────────────────────────────┐
│   TIER 1: CANONICAL MAPPINGS    │  │   TIER 2: RUNTIME MAPPINGS       │
│         (Build-Time)            │  │        (Runtime)                 │
└─────────────────────────────────┘  └──────────────────────────────────┘

YAML Files                           Live Device Configuration
(canonical-midi-maps/maps/)          (stored in Live project)
    │                                    │
    │ npm run convert-maps               │ LiveDeployer.deploy()
    ↓                                    ↓
src/canonical-plugin-maps.ts ──→    data/plugin-mappings.json
(Auto-generated, DO NOT EDIT)        (User-extracted, editable)
    │                                    │
    │ import at compile-time             │ fs.readFileSync() at runtime
    ↓                                    ↓
CANONICAL_PLUGIN_MAPS                Runtime Mappings Object
(TypeScript constant)                (Loaded from JSON)
    │                                    │
    └────────────────┬───────────────────┘
                     ↓
            { ...canonical, ...runtime }
                     ↓
                 cc-router
              (merged mappings)
                     ↓
            Max for Live Device
         (MIDI CC → Plugin Parameters)
```

## Tier 1: Canonical Mappings (Build-Time)

### Source

- **Location:** `@oletizi/canonical-midi-maps` package
- **Path:** `canonical-midi-maps/maps/*.yaml`
- **Format:** YAML with strict schema validation
- **Version Control:** Git-tracked, PR workflow

### Build Process

1. **Developer creates/updates YAML mapping:**
   ```bash
   cd modules/canonical-midi-maps
   # Edit or create: maps/launch-control-xl-3/channev.yaml
   ```

2. **Build pipeline converts to TypeScript:**
   ```bash
   cd modules/live-max-cc-router
   npm run convert-maps
   # Runs: scripts/convert-canonical-maps.cjs
   ```

3. **Generated output:**
   ```typescript
   // src/canonical-plugin-maps.ts (AUTO-GENERATED)
   export const CANONICAL_PLUGIN_MAPS = {
     "launch-control-xl-3_channev": {
       controller: { ... },
       pluginName: "CHANNEV",
       mappings: { ... }
     }
   };
   ```

4. **Bundled into distribution:**
   ```bash
   npm run build
   # TypeScript → rollup → dist/cc-router.js
   ```

### Purpose

- **Curated defaults** - High-quality mappings for popular plugins
- **Version controlled** - Changes go through PR review
- **Type-safe** - Compile-time validation
- **Distribution** - Included in built package

### Update Workflow

```
Developer → YAML edit → PR → Review → Merge → npm run build → Deploy
```

**Timeline:** Hours to days (PR review process)

## Tier 2: Runtime Mappings (Runtime)

### Source

- **Location:** `live-max-cc-router/data/plugin-mappings.json`
- **Format:** JSON matching `PluginMappingsDatabase` interface
- **Version Control:** Git-ignored (user-specific)
- **Tool:** `LiveDeployer` from `controller-workflow` module

### Extraction Process

1. **User configures device in Live:**
   - Opens Launch Control XL3 in Novation Components
   - Maps CC controls to desired plugin parameters
   - Saves configuration to device slot

2. **User runs deployment workflow:**
   ```bash
   cd modules/controller-workflow
   npx controller-deploy deploy --slot 0 --daw live
   ```

3. **LiveDeployer extracts configuration:**
   ```typescript
   // LiveDeployer reads device SysEx
   const deviceConfig = await adapter.readConfig(slot);

   // Converts to canonical format
   const canonicalMap = converter.toCanonical(deviceConfig);

   // Writes to JSON file
   await fs.writeFile('data/plugin-mappings.json', JSON.stringify({
     "launch-control-xl-3_my-plugin": {
       controller: { ... },
       pluginName: "My Plugin",
       mappings: { ... }
     }
   }));
   ```

4. **cc-router loads at runtime:**
   ```typescript
   // src/runtime-loader.ts
   const runtimeMaps = loadRuntimeMappings(jsonPath);
   // Returns: { "launch-control-xl-3_my-plugin": { ... } }
   ```

### Purpose

- **User customization** - Extract actual device configuration
- **One-click workflow** - No manual YAML editing
- **Live-specific** - Uses actual parameter assignments from Live
- **Immediate updates** - No rebuild required

### Update Workflow

```
User configures → controller-deploy → JSON written → Reload cc-router → Active
```

**Timeline:** Seconds to minutes (immediate)

## Merge Strategy

### Runtime Overrides Canonical

The cc-router merges mappings with **runtime taking precedence**:

```typescript
// Conceptual merge (actual implementation in cc-router.ts)
const MERGED_MAPPINGS = {
  ...CANONICAL_PLUGIN_MAPS,  // Tier 1: Canonical defaults
  ...runtimeMappings          // Tier 2: Runtime overrides
};
```

### Example Scenarios

#### Scenario 1: No Runtime Mapping

```typescript
// CANONICAL_PLUGIN_MAPS contains:
{
  "launch-control-xl-3_channev": { /* mapping */ }
}

// plugin-mappings.json is empty:
{}

// Result: Uses canonical default
ccRouter.setupfor("channev"); // ✅ Uses canonical
```

#### Scenario 2: Runtime Overrides Canonical

```typescript
// CANONICAL_PLUGIN_MAPS contains:
{
  "launch-control-xl-3_channev": {
    mappings: {
      13: { parameterIndex: 0, parameterName: "Gain" }
    }
  }
}

// plugin-mappings.json contains:
{
  "launch-control-xl-3_channev": {
    mappings: {
      13: { parameterIndex: 5, parameterName: "Custom Param" }
    }
  }
}

// Result: Runtime override wins
ccRouter.setupfor("channev"); // ✅ Uses runtime (parameterIndex: 5)
```

#### Scenario 3: New Runtime-Only Mapping

```typescript
// CANONICAL_PLUGIN_MAPS contains:
{
  "launch-control-xl-3_channev": { /* mapping */ }
}

// plugin-mappings.json contains:
{
  "launch-control-xl-3_my-custom-plugin": { /* new mapping */ }
}

// Result: Both mappings available
ccRouter.setupfor("my-custom-plugin"); // ✅ Uses runtime
ccRouter.setupfor("channev");          // ✅ Uses canonical
```

## File Locations

### Build-Time Files (Tier 1)

| File | Purpose | Edit? | Version Control |
|------|---------|-------|-----------------|
| `canonical-midi-maps/maps/*.yaml` | Source YAML mappings | ✅ Yes | ✅ Git tracked |
| `src/canonical-plugin-maps.ts` | Generated TypeScript | ❌ No (auto-gen) | ✅ Git tracked |
| `dist/cc-router.js` | Bundled output | ❌ No (build artifact) | ❌ Git ignored |

### Runtime Files (Tier 2)

| File | Purpose | Edit? | Version Control |
|------|---------|-------|-----------------|
| `data/plugin-mappings.json` | Runtime user mappings | ✅ Yes (manual or tool) | ❌ Git ignored |
| `src/runtime-loader.ts` | JSON loading logic | ✅ Yes | ✅ Git tracked |

### Workflow Integration Files

| File | Purpose | Module |
|------|---------|--------|
| `controller-workflow/src/adapters/daws/LiveDeployer.ts` | Writes JSON | `controller-workflow` |
| `launch-control-xl3/src/device/DeviceManager.ts` | Reads device config | `launch-control-xl3` |

## Responsibilities

### Canonical Mappings (Tier 1)

**Maintained by:** Module developers and contributors
**Quality bar:** High - requires PR review
**Update frequency:** Infrequent (weeks/months)

**Responsibilities:**
- Provide sensible defaults for popular plugins
- Ensure mappings are well-tested
- Document parameter choices
- Maintain backward compatibility

### Runtime Mappings (Tier 2)

**Maintained by:** End users
**Quality bar:** User-defined
**Update frequency:** Frequent (as needed)

**Responsibilities:**
- User-specific customizations
- Live project configurations
- Experimental mappings
- Plugin-specific tweaks

## Common Workflows

### Workflow 1: Add a New Canonical Mapping

**Use Case:** Add curated default for popular plugin

**Steps:**

1. Create YAML file:
   ```bash
   cd modules/canonical-midi-maps/maps/launch-control-xl-3
   touch my-plugin.yaml
   ```

2. Define mapping:
   ```yaml
   # my-plugin.yaml
   version: "1.0.0"
   metadata:
     name: "My Plugin Mapping"
   device:
     manufacturer: "Novation"
     model: "Launch Control XL 3"
   plugin:
     name: "My Plugin"
   controls:
     - id: encoder_1
       cc: 13
       parameter:
         index: 0
         name: "Cutoff"
   ```

3. Rebuild module:
   ```bash
   cd ../../live-max-cc-router
   npm run convert-maps
   npm run build
   ```

4. Commit and PR:
   ```bash
   git add canonical-midi-maps/maps/launch-control-xl-3/my-plugin.yaml
   git add src/canonical-plugin-maps.ts
   git commit -m "feat: add My Plugin canonical mapping"
   ```

**Timeline:** Hours to days (PR review)

### Workflow 2: Override a Canonical Mapping

**Use Case:** User wants different parameters than canonical default

**Steps:**

1. Configure device in Live:
   - Open Novation Components web editor
   - Map CC 13 to different parameter
   - Save to device slot 0

2. Extract to runtime JSON:
   ```bash
   cd modules/controller-workflow
   npx controller-deploy deploy --slot 0 --daw live --plugin "My Plugin"
   ```

3. Verify JSON created:
   ```bash
   cat ../live-max-cc-router/data/plugin-mappings.json | jq
   ```

4. Reload cc-router in Max for Live:
   - Click "reload" in Max device
   - Test CC controls

**Timeline:** Minutes

### Workflow 3: Remove a Runtime Override

**Use Case:** Return to canonical default

**Steps:**

1. Edit JSON file:
   ```bash
   cd modules/live-max-cc-router/data
   # Remove entry from plugin-mappings.json
   ```

2. Or delete entire file:
   ```bash
   rm plugin-mappings.json
   ```

3. Reload cc-router in Max for Live

**Timeline:** Seconds

### Workflow 4: Create New Runtime-Only Mapping

**Use Case:** Map a plugin with no canonical default

**Steps:**

1. Configure device:
   - Set up CC mappings in Novation Components
   - Save to slot

2. Extract configuration:
   ```bash
   npx controller-deploy deploy --slot 0 --daw live --plugin "New Plugin"
   ```

3. Verify and test:
   ```bash
   cat data/plugin-mappings.json
   # Reload cc-router
   ```

**Timeline:** Minutes

## Implementation Details

### Runtime Loading (src/runtime-loader.ts)

```typescript
/**
 * Loads runtime mappings from JSON file.
 * Returns empty object if file doesn't exist (not an error).
 * Throws if file exists but is malformed.
 */
export function loadRuntimeMappings(jsonPath: string): PluginMappingsDatabase {
  if (!fs.existsSync(jsonPath)) {
    return {}; // No runtime mappings - use canonical only
  }

  const content = fs.readFileSync(jsonPath, 'utf-8');
  const parsed = JSON.parse(content);

  // Validation logic...

  return parsed as PluginMappingsDatabase;
}
```

### Merge in cc-router (Future Implementation)

```typescript
// Planned implementation in cc-router.ts
import { CANONICAL_PLUGIN_MAPS } from '@/canonical-plugin-maps';
import { loadRuntimeMappings } from '@/runtime-loader';

// Load runtime at initialization
const runtimeMaps = loadRuntimeMappings(
  join(__dirname, '..', 'data', 'plugin-mappings.json')
);

// Merge: runtime overrides canonical
const MERGED_MAPS = {
  ...CANONICAL_PLUGIN_MAPS,
  ...runtimeMaps
};

// Use merged mappings
export function getPluginMapping(controller: string, plugin: string) {
  const key = `${controller}_${plugin}`;
  return MERGED_MAPS[key];
}
```

## Benefits of Dual-Pipeline Architecture

### For Module Developers

✅ **Canonical defaults preserved** - YAML workflow unchanged
✅ **No breaking changes** - Existing build pipeline intact
✅ **Quality control** - Canonical mappings go through PR review
✅ **Type safety** - TypeScript validation at build time

### For End Users

✅ **One-click extraction** - No manual YAML editing
✅ **Immediate updates** - No rebuild required
✅ **Live integration** - Uses actual Live parameter assignments
✅ **Easy overrides** - Runtime mappings take precedence

### For System Architecture

✅ **Clear separation** - Canonical (curated) vs Runtime (user)
✅ **Flexible deployment** - Ship with defaults, customize at runtime
✅ **Testability** - Both tiers independently testable
✅ **Maintainability** - Simple merge strategy

## Future Enhancements

### Planned Features

- **Hot reload** - Watch JSON file for changes, reload without restart
- **Validation UI** - Show conflicts between canonical and runtime
- **Backup/restore** - Save/load runtime mapping configurations
- **Sync to Live project** - Store mappings in Live set

### Potential Improvements

- **Multi-device support** - Handle multiple controllers simultaneously
- **Mapping presets** - User-defined named configurations
- **Cloud sync** - Share runtime mappings across machines
- **Visual editor** - GUI for editing JSON mappings

## Troubleshooting

### Issue: Runtime mapping not taking effect

**Check:**
1. Verify JSON file exists: `ls data/plugin-mappings.json`
2. Validate JSON syntax: `cat data/plugin-mappings.json | jq`
3. Check mapping key format: `"controller-model_plugin-name"`
4. Reload cc-router device in Max for Live

### Issue: Canonical mapping missing

**Check:**
1. Verify YAML file exists in `canonical-midi-maps/maps/`
2. Run build: `npm run convert-maps && npm run build`
3. Check generated file: `cat src/canonical-plugin-maps.ts`
4. Verify key format matches

### Issue: Wrong parameters mapped

**Debug:**
1. Enable debug logging in cc-router
2. Check which tier is being used: `logger.debug('Using canonical vs runtime')`
3. Verify parameter indices in JSON/TypeScript
4. Test with `testcc` command in Max console

## Related Documentation

- [LiveDeployer Implementation Plan](/Users/orion/work/ol_dsp-audio-control/modules/audio-control/docs/1.21/360/live-deployer/implementation/workplan.md)
- [Controller Workflow README](/Users/orion/work/ol_dsp-audio-control/modules/audio-control/modules/controller-workflow/README.md)
- [Canonical MIDI Maps README](/Users/orion/work/ol_dsp-audio-control/modules/audio-control/modules/canonical-midi-maps/README.md)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-10-06 | Initial dual-pipeline architecture documentation |

---

**Maintained by:** Audio Control Monorepo Team
**Questions:** See controller-workflow module documentation
