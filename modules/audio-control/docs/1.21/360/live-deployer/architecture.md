# LiveDeployer Dual-Pipeline Architecture

**Component:** LiveDeployer (Ableton Live deployment)
**Version:** 1.21
**Status:** Phase 2 Complete
**Last Updated:** 2025-10-11

## Overview

The LiveDeployer implements a **dual-pipeline architecture** for deploying MIDI controller mappings to Ableton Live via Max for Live. This design solves the conflict between build-time canonical mappings and runtime device-extracted mappings.

## Architecture Summary

```
┌────────────────────────────────────────────────────────────────────┐
│                  Dual-Pipeline Mapping System                      │
└────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────┐    ┌─────────────────────────────┐
│  Tier 1: Canonical          │    │  Tier 2: Runtime            │
│  (Build-time)               │    │  (Device-extracted)         │
└─────────────────────────────┘    └─────────────────────────────┘

Source: YAML files                  Source: Controller hardware
        (version-controlled)                (custom modes)

Process:                            Process:
  ┌──────────────┐                    ┌──────────────┐
  │ YAML files   │                    │ Hardware     │
  └──────┬───────┘                    │ Custom Mode  │
         │                            └──────┬───────┘
         ↓                                   │
  ┌──────────────┐                           ↓
  │ convert-     │                    ┌──────────────┐
  │ canonical-   │                    │ controller-  │
  │ maps.cjs     │                    │ deploy       │
  └──────┬───────┘                    └──────┬───────┘
         │                                   │
         ↓                                   ↓
  ┌──────────────┐                    ┌──────────────┐
  │ canonical-   │                    │ plugin-      │
  │ plugin-maps  │                    │ mappings.json│
  │ .ts          │                    └──────────────┘
  └──────────────┘

         └───────────────┬───────────────┘
                         │
                         ↓
              ┌──────────────────┐
              │ Runtime Merge    │
              │ { ...canonical,  │
              │   ...runtime }   │
              └──────────────────┘
                         │
                         ↓
              ┌──────────────────┐
              │ Max for Live     │
              │ cc-router        │
              └──────────────────┘
```

## Why Dual-Pipeline?

### The Problem

During Phase 2 implementation, we discovered a conflict:

**Original Plan:** LiveDeployer writes to `canonical-plugin-maps.ts`

**Conflict:** Build script regenerates this file from YAML sources:
```bash
pnpm build
  ↓
convert-canonical-maps.cjs
  ↓
canonical-plugin-maps.ts (OVERWRITTEN)
```

**Issue:** Any runtime updates from hardware get lost on next build.

### The Solution

**Two-tier system** with separate data sources:

1. **Tier 1 (Canonical):** Build-time YAML → TypeScript (curated defaults)
2. **Tier 2 (Runtime):** Hardware extraction → JSON (user customization)
3. **Runtime Merge:** Combined at load time (runtime overrides canonical)

## Tier 1: Canonical Mappings (Build-Time)

### Purpose

Provide curated, version-controlled default mappings for known controller + plugin combinations.

### Source

YAML files in `canonical-midi-maps/maps/`:
```
maps/novation-launch-control-xl-3/
├── roland-jupiter-8/
│   ├── tal-j8.yaml
│   ├── arturia-jup8v.yaml
│   └── jupiter-8-base.yaml
└── moog-minimoog/
    └── arturia-mini-v4.yaml
```

### Build Process

```bash
# Run during build
pnpm build
  ↓
convert-canonical-maps.cjs
  ↓
src/canonical-plugin-maps.ts (AUTO-GENERATED)
```

**Output Example:**
```typescript
// canonical-plugin-maps.ts (GENERATED - DO NOT EDIT)
export const CANONICAL_PLUGIN_MAPS: PluginMappingsDatabase = {
  "launch-control-xl-3_tal-j-8": {
    controller: { manufacturer: "Novation", model: "Launch Control XL 3" },
    pluginName: "TAL-J-8",
    mappings: {
      "13": { parameterIndex: 105, parameterName: "VCF Cutoff", ... },
      "14": { parameterIndex: 107, parameterName: "VCF Resonance", ... }
    },
    metadata: { source: "canonical", ... }
  }
};
```

### Characteristics

- **Curated:** Carefully designed mappings
- **Version-controlled:** Changes tracked in git
- **Build artifact:** Generated during compilation
- **Immutable at runtime:** Cannot be changed without rebuild
- **Default mappings:** Used when no runtime override exists

## Tier 2: Runtime Mappings (Device-Extracted)

### Purpose

Enable "one-click" deployment: extract custom mode from hardware and deploy immediately.

### Source

Controller hardware (custom modes) extracted via `controller-deploy`:
```bash
npx controller-deploy deploy --slot 0 --daw live
```

### Extraction Process

```
1. Read custom mode from controller (SysEx)
   ↓
2. Convert to CanonicalMidiMap
   ↓
3. Transform to PluginMapping format
   ↓
4. Write to data/plugin-mappings.json
```

**Output Location:** `live-max-cc-router/data/plugin-mappings.json`

**Output Example:**
```json
{
  "version": "1.0.0",
  "lastUpdated": "2025-10-11T10:00:00Z",
  "mappings": {
    "launch-control-xl-3_user-custom": {
      "controller": {
        "manufacturer": "Novation",
        "model": "Launch Control XL 3"
      },
      "pluginName": "User Custom",
      "mappings": {
        "13": {
          "deviceIndex": 0,
          "parameterIndex": 42,
          "parameterName": "My Custom Param",
          "curve": "linear"
        }
      },
      "metadata": {
        "name": "User Template",
        "source": "device-extracted",
        "controllerSlot": 0,
        "extractedAt": "2025-10-11T10:00:00Z"
      }
    }
  }
}
```

### Characteristics

- **User-created:** Extracted from hardware
- **Not version-controlled:** User-specific customizations
- **Runtime data:** JSON file (not build artifact)
- **Mutable at runtime:** Can be updated anytime
- **Override canonical:** Takes precedence in merge

## Runtime Merge Strategy

### Loading Process

**Location:** `live-max-cc-router/src/runtime-loader.ts` + `mapping-registry.ts`

```typescript
// 1. Load canonical (compile-time)
import { CANONICAL_PLUGIN_MAPS } from './canonical-plugin-maps.js';

// 2. Load runtime (file read)
const runtimeMappings = loadRuntimeMappings('./data/plugin-mappings.json');

// 3. Merge (runtime wins)
const merged = {
  ...CANONICAL_PLUGIN_MAPS,  // Tier 1: Defaults
  ...runtimeMappings          // Tier 2: Overrides
};
```

### Merge Behavior

**Same mapping key exists:**
```typescript
// Canonical provides default
CANONICAL_PLUGIN_MAPS["launch-control-xl-3_tal-j-8"] = { ... }

// Runtime overrides completely
runtimeMappings["launch-control-xl-3_tal-j-8"] = { ... }  // WINS

// Result: Runtime mapping used
```

**Different mapping keys:**
```typescript
// Canonical provides one mapping
CANONICAL_PLUGIN_MAPS["launch-control-xl-3_tal-j-8"] = { ... }

// Runtime provides different mapping
runtimeMappings["launch-control-xl-3_user-synth"] = { ... }

// Result: Both available
merged = {
  "launch-control-xl-3_tal-j-8": { ...canonical },
  "launch-control-xl-3_user-synth": { ...runtime }
}
```

### Source Tracking

Metadata includes `source` field to distinguish origin:
```typescript
metadata: {
  source: "canonical" | "device-extracted"
}
```

## Implementation Details

### MappingRegistry Class

**Location:** `live-max-cc-router/src/mapping-registry.ts`

```typescript
export class MappingRegistry {
  constructor(
    private canonicalMaps: PluginMappingsDatabase,
    private runtimeMaps: PluginMappingsDatabase
  ) {
    this.mergedMaps = { ...canonicalMaps, ...runtimeMaps };
  }

  getPluginMapping(key: string): PluginMapping | undefined {
    return this.mergedMaps[key];  // Runtime overrides canonical
  }

  getAllKeys(): string[] {
    return Object.keys(this.mergedMaps);
  }

  getSource(key: string): 'canonical' | 'runtime' | 'unknown' {
    if (this.runtimeMaps[key]) return 'runtime';
    if (this.canonicalMaps[key]) return 'canonical';
    return 'unknown';
  }
}
```

### Runtime Loader

**Location:** `live-max-cc-router/src/runtime-loader.ts`

```typescript
export function loadRuntimeMappings(jsonPath: string): PluginMappingsDatabase {
  try {
    const content = readFileSync(jsonPath, 'utf-8');
    const registry: PluginMappingRegistry = JSON.parse(content);
    return registry.mappings;
  } catch (error) {
    // File missing is OK - return empty object
    if (error.code === 'ENOENT') {
      return {};
    }
    throw new Error(`Failed to load runtime mappings: ${error.message}`);
  }
}
```

### Integration with cc-router

**Location:** `live-max-cc-router/src/cc-router.ts`

```typescript
export class CCRouter {
  private registry: MappingRegistry;

  constructor(options: CCRouterOptions = {}) {
    const runtimeMaps = loadRuntimeMappings(
      options.runtimeMappingsPath || './data/plugin-mappings.json'
    );

    this.registry = createMappingRegistry({
      canonicalMaps: CANONICAL_PLUGIN_MAPS,
      runtimeMaps
    });
  }

  autoApplyCanonicalMapping(controllerName: string, pluginName: string): void {
    const key = `${controllerName}_${pluginName}`;
    const mapping = this.registry.getPluginMapping(key);

    if (mapping) {
      const source = this.registry.getSource(key);
      this.post(`📥 Applied ${source} mapping: ${key}`);
      this.applyMapping(mapping);
    }
  }
}
```

## Workflow Examples

### Workflow 1: Use Canonical Mapping (Default)

```
User opens Max for Live device
  ↓
CCRouter initializes
  ↓
Loads canonical mappings (build artifact)
  ↓
Runtime JSON file doesn't exist or is empty
  ↓
Uses canonical mapping only
```

**No custom hardware setup required** - curated defaults work immediately.

### Workflow 2: Override with Device Extraction

```
User creates custom mode on hardware
  ↓
Runs: npx controller-deploy deploy --slot 0 --daw live
  ↓
Writes to data/plugin-mappings.json
  ↓
User reopens Max for Live device
  ↓
CCRouter loads both canonical + runtime
  ↓
Runtime mapping overrides canonical
  ↓
Uses custom hardware configuration
```

**One-click customization** - no manual editing required.

### Workflow 3: Mix Canonical + Custom

```
Canonical provides:
  - launch-control-xl-3_tal-j-8
  - launch-control-xl-3_arturia-jup8v

User extracts from hardware:
  - launch-control-xl-3_my-synth (custom)

Result:
  - All three mappings available
  - User can switch between them
```

**Best of both worlds** - curated defaults + user customization.

### Workflow 4: Remove Runtime Override

```bash
# Remove custom mapping
rm live-max-cc-router/data/plugin-mappings.json

# Or edit JSON to remove specific mapping
vim live-max-cc-router/data/plugin-mappings.json
# Delete the mapping key you want to remove

# Reopen Max for Live device
# Falls back to canonical mapping
```

**Easy rollback** - delete JSON to restore defaults.

## Benefits

### For Users

- **Instant deployment:** Extract from hardware, use immediately
- **No manual editing:** JSON generated automatically
- **Preserve customizations:** Runtime mappings survive builds
- **Rollback anytime:** Delete JSON to restore defaults

### For Developers

- **No build conflicts:** Canonical and runtime use separate files
- **Type safety:** TypeScript for canonical, validated JSON for runtime
- **Easy testing:** Mock runtime loader for unit tests
- **Clear separation:** Build-time vs runtime concerns isolated

### For Maintainers

- **Version control:** Canonical in git, runtime user-specific
- **Clear audit trail:** Metadata tracks source and timestamp
- **Easy debugging:** Check which tier mapping came from
- **Future-proof:** Architecture supports additional tiers if needed

## File Locations

| File | Purpose | Tier | Generated By |
|------|---------|------|--------------|
| `canonical-plugin-maps.ts` | Build artifact | Tier 1 | convert-canonical-maps.cjs |
| `data/plugin-mappings.json` | Runtime data | Tier 2 | controller-deploy |
| `runtime-loader.ts` | JSON loader | Both | Manual (lib code) |
| `mapping-registry.ts` | Merge logic | Both | Manual (lib code) |
| `cc-router.ts` | Integration | Both | Manual (lib code) |

## Deployment Process

### Canonical Mappings

```bash
# 1. Edit YAML
vim canonical-midi-maps/maps/.../my-plugin.yaml

# 2. Build
cd live-max-cc-router
pnpm build

# 3. Canonical maps regenerated automatically
# Result: canonical-plugin-maps.ts updated
```

### Runtime Mappings

```bash
# 1. Create custom mode on hardware (Novation Components)

# 2. Extract and deploy
npx controller-deploy deploy --slot 0 --daw live

# 3. JSON file created/updated
# Result: data/plugin-mappings.json updated

# 4. Reload Max for Live device
# Runtime mapping now active
```

## Troubleshooting

### Runtime mapping not loading

**Check:**
1. File exists: `ls live-max-cc-router/data/plugin-mappings.json`
2. Valid JSON: `jq . data/plugin-mappings.json`
3. Correct path in CCRouter options
4. Max for Live device reloaded after JSON update

### Canonical mapping overriding custom mapping

**Cause:** Runtime mapping key doesn't match canonical key

**Fix:** Check mapping keys match exactly:
```typescript
// Canonical key
"launch-control-xl-3_tal-j-8"

// Runtime key must match exactly
"launch-control-xl-3_tal-j-8"  // ✅ Overrides
"launch-control-xl-3_TAL-J-8"  // ❌ Different key (case sensitive)
```

### JSON file keeps getting overwritten

**Cause:** Running wrong deploy command

**Fix:**
```bash
# Correct (updates JSON)
npx controller-deploy deploy --slot 0 --daw live

# Wrong (would overwrite TypeScript, but prevented by architecture)
# (No longer possible - architecture prevents this)
```

## Performance

### Load Time

- **Canonical:** Instant (imported at compile time)
- **Runtime:** <20ms (JSON parse + validation)
- **Merge:** <1ms (object spread)
- **Total:** <25ms (negligible)

### Memory

- **Canonical:** ~50KB typical mapping database
- **Runtime:** ~10-50KB per custom mapping
- **Total:** <200KB for typical use (10 mappings)

## Future Enhancements

### Tier 3: Session Mappings (Potential)

Could add temporary session-specific mappings:
```
Tier 1: Canonical (build-time, defaults)
Tier 2: Runtime (extracted, user permanent)
Tier 3: Session (temporary, this session only)
```

### Hot Reload

Watch JSON file for changes and auto-reload:
```typescript
fs.watch('data/plugin-mappings.json', () => {
  this.registry.reload();
});
```

### Conflict Resolution UI

If both tiers provide same mapping, let user choose:
```
Canonical: [Use this]
Runtime:   [Use this]
```

## Related Documentation

- **[Implementation Workplan](./implementation/workplan.md)** - Complete implementation details
- **[Module Documentation](../../../../modules/live-max-cc-router/docs/architecture/mapping-sources.md)** - Detailed module architecture
- **[Main Architecture](../architecture.md)** - Overall 360 architecture
- **[Workflow Guide](../workflow.md)** - User workflows

---

**Questions?** See the [main README](../README.md) or module documentation.
