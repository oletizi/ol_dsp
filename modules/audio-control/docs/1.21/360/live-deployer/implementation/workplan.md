# LiveDeployer Implementation Plan: Fixing TypeScript Errors & Architecture Issues

**Issue:** #360
**Module:** `controller-workflow`
**Component:** `LiveDeployer`
**Version:** 1.21
**Status:** Implementation Planning
**Created:** 2025-10-05
**Priority:** High - Blocking one-click deployment workflow

---

## Executive Summary

### Current State

The `LiveDeployer` implementation in `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts` has **7 TypeScript compilation errors** and uses a **fragile string manipulation approach** to update TypeScript files. This blocks the completion of the one-click controller deployment workflow for Ableton Live.

**Errors Summary:**
- 1 error: Unknown property `message` in `DeploymentResult`
- 1 error: `exactOptionalPropertyTypes` mismatch in metadata object
- 2 errors: Potentially undefined string values
- 4 errors: Possibly undefined object access in mapping serialization

### Problems

1. **Type Safety Violations**: `exactOptionalPropertyTypes: true` enforces strict handling of optional properties, exposing latent bugs
2. **Architectural Fragility**: String manipulation to update TypeScript source files (lines 176-215) is error-prone and unmaintainable
3. **Missing Parameter Resolution**: No mechanism to match controller controls to plugin parameters semantically
4. **No Integration Testing**: String manipulation approach cannot be reliably tested

### Proposed Solution

**Phase 1 (Immediate)**: Fix all TypeScript errors to restore compilation
**Phase 2 (Strategic)**: Replace string manipulation with JSON data approach
**Phase 3 (Enhancement)**: Add fuzzy matching and parameter validation

---

## Problem Analysis

### TypeScript Errors Detail

#### Error 1: Unknown Property `message` (Line 99)

```typescript
// ❌ Current (line 99)
return {
  success: true,
  dawName: this.dawName,
  outputPath: canonicalMapsFile,
  installed: true,
  message: `Updated ${canonicalMapsFile}...`, // ❌ Not in DeploymentResult interface
};
```

**Root Cause**: `DeploymentResult` interface doesn't include `message` property

**Impact**: Breaks type contract with `DAWDeployerInterface`

#### Error 2: `exactOptionalPropertyTypes` Violation (Line 151)

```typescript
// ❌ Current (line 151)
metadata: {
  name: canonicalMap.metadata.name,           // string | undefined
  description: canonicalMap.metadata.description,  // string | undefined
  version: canonicalMap.version,              // string (always defined)
},
```

**Root Cause**: When `exactOptionalPropertyTypes: true`, TypeScript requires either:
- All properties must be explicitly `T | undefined`, OR
- Properties must never be `undefined`

**Impact**: Type safety violation - the object has different runtime shape than compile-time declaration

#### Errors 3-4: Potentially Undefined String (Line 167, 221-224)

```typescript
// ❌ Line 167
return parseInt(match[1], 10);  // match can be null

// ❌ Lines 221-224
const controller = canonicalMap.device.model?.toLowerCase()...  // model is optional
const plugin = (canonicalMap.plugin?.name || ...)
  .toLowerCase()
  .replace(/\s+/g, '-');
```

**Root Cause**: Optional chaining returns `string | undefined`, but code treats as `string`

**Impact**: Runtime errors if properties are undefined

#### Errors 5-8: Possibly Undefined Object Access (Lines 256-259)

```typescript
// ❌ Lines 250-260
const m = mapping.mappings[cc];  // Could be undefined due to noUncheckedIndexedAccess
lines.push(`        "deviceIndex": ${m.deviceIndex},`);    // ❌ m possibly undefined
lines.push(`        "parameterIndex": ${m.parameterIndex},`);  // ❌ m possibly undefined
lines.push(`        "parameterName": "${m.parameterName}",`);  // ❌ m possibly undefined
lines.push(`        "curve": "${m.curve}"`);                    // ❌ m possibly undefined
```

**Root Cause**: `noUncheckedIndexedAccess: true` means indexed access always returns `T | undefined`

**Impact**: Potential runtime null reference errors

---

### Architecture Issues

#### Issue 1: String Manipulation of TypeScript Files (Lines 176-215)

**Current Approach:**
```typescript
private async updateCanonicalMaps(
  existingContent: string,
  newMapping: PluginMapping,
  canonicalMap: CanonicalMidiMap
): Promise<string> {
  // Uses regex to find/replace mapping keys
  const keyPattern = new RegExp(`"${mappingKey}"\\s*:\\s*{`, 'g');

  if (keyPattern.test(existingContent)) {
    // Replace with another regex
    const replacePattern = new RegExp(...);
    return existingContent.replace(replacePattern, ...);
  } else {
    // String surgery to insert before closing brace
    const closingBraceIndex = existingContent.lastIndexOf('};');
    // ...
  }
}
```

**Problems:**
1. **Fragile**: Breaks if formatting changes (spaces, line breaks, comments)
2. **No Validation**: Can corrupt TypeScript syntax
3. **Hard to Test**: Requires mock file system and exact string matching
4. **No Error Recovery**: Single regex failure breaks entire deployment
5. **Maintenance Burden**: Every TypeScript formatting change requires regex updates

#### Issue 2: Missing Parameter Resolution

**Current Approach:**
```typescript
private extractParameterIndex(controlId: string): number {
  const match = controlId.match(/(\d+)$/);  // Just extract trailing number
  if (match) {
    return parseInt(match[1], 10);
  }
  return 0;  // ❌ Fallback to 0 - wrong parameter!
}
```

**Problems:**
1. **No Semantic Matching**: `encoder_1` → parameter 1 is arbitrary
2. **No Plugin Awareness**: Doesn't know what plugin parameter names mean
3. **No Validation**: Can't detect incorrect mappings
4. **No Fuzzy Matching**: Can't handle similar but not identical names

---

## Implementation Plan

### Phase 1: Fix TypeScript Errors (Immediate - 2 hours)

**Goal**: Restore compilation, maintain current functionality

#### Step 1.1: Add `message` to `DeploymentResult` Interface

**File**: `modules/controller-workflow/src/types/daw-deployer.ts`

```typescript
export interface DeploymentResult {
  success: boolean;
  dawName?: string;
  outputPath?: string;
  installed?: boolean;
  errors?: string[];
  message?: string;  // ✅ ADD THIS
}
```

**Rationale**: Quick fix to support informational messages without breaking interface

#### Step 1.2: Fix `exactOptionalPropertyTypes` Violation

**File**: `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts` (Line 151)

```typescript
// ✅ Conditional property assignment (PREFERRED)
metadata: {
  ...(canonicalMap.metadata.name && { name: canonicalMap.metadata.name }),
  ...(canonicalMap.metadata.description && { description: canonicalMap.metadata.description }),
  version: canonicalMap.version,
}
```

**Rationale**: Only includes properties that have values, satisfying `exactOptionalPropertyTypes: true`

#### Step 1.3: Fix Potentially Undefined Strings

**File**: `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts`

```typescript
// ✅ Line 167: extractParameterIndex
private extractParameterIndex(controlId: string): number {
  const match = controlId.match(/(\d+)$/);
  if (match && match[1]) {  // ✅ Check match AND match[1]
    return parseInt(match[1], 10);
  }
  return 0;
}

// ✅ Lines 221-224: generateMappingKey
private generateMappingKey(canonicalMap: CanonicalMidiMap): string {
  const controller = (canonicalMap.device.model ?? 'unknown')
    .toLowerCase()
    .replace(/\s+/g, '-');
  const plugin = (canonicalMap.plugin?.name ?? canonicalMap.metadata.name ?? 'unknown')
    .toLowerCase()
    .replace(/\s+/g, '-');
  return `${controller}_${plugin}`;
}
```

**Rationale**: Use nullish coalescing (`??`) to provide safe defaults

#### Step 1.4: Fix Possibly Undefined Object Access

**File**: `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts` (Lines 250-260)

```typescript
// ✅ Add type guard
ccNumbers.forEach((ccStr, index) => {
  const cc = parseInt(ccStr);
  const m = mapping.mappings[cc];

  // ✅ Type guard - skip if undefined
  if (!m) {
    console.warn(`⚠️  Skipping undefined mapping for CC ${cc}`);
    return;
  }

  const comma = index < ccNumbers.length - 1 ? ',' : '';
  lines.push(`      "${cc}": {`);
  lines.push(`        "deviceIndex": ${m.deviceIndex},`);
  lines.push(`        "parameterIndex": ${m.parameterIndex},`);
  lines.push(`        "parameterName": "${m.parameterName}",`);
  lines.push(`        "curve": "${m.curve}"`);
  lines.push(`      }${comma}`);
});
```

**Rationale**: Explicit type guard prevents all 4 errors at once

**Verification Steps:**
```bash
# After changes
cd modules/controller-workflow
pnpm tsc --noEmit  # Should show 0 errors
pnpm test          # Existing tests should still pass
```

---

### Phase 2: Replace String Manipulation with JSON Data Approach (Strategic - 4-6 hours)

**Goal**: Robust, maintainable architecture using data files instead of source code manipulation

#### Architecture Decision

**Option A: Continue String Manipulation** ❌
- Pros: No architectural changes
- Cons: Fragile, untestable, maintenance burden

**Option B: JSON Data File Approach** ✅ RECOMMENDED
- Pros: Type-safe, testable, maintainable, versionable
- Cons: Requires refactoring cc-router module

**Decision**: **Option B** - Long-term correctness outweighs short-term refactoring cost

#### Step 2.1: Create JSON Data Schema

**File**: `modules/live-max-cc-router/src/types/plugin-mappings.ts`

```typescript
/**
 * JSON schema for plugin mappings loaded by cc-router at runtime
 */
export interface PluginMappingRegistry {
  version: string;
  lastUpdated: string;
  mappings: Record<string, PluginMapping>;
}

export interface PluginMapping {
  controller: {
    manufacturer?: string;
    model?: string;
  };
  pluginName: string;
  pluginManufacturer: string;
  mappings: Record<number, ParameterMapping>;
  metadata: {
    name?: string;
    description?: string;
    version?: string;
  };
}

export interface ParameterMapping {
  deviceIndex: number;
  parameterIndex: number;
  parameterName: string;
  curve: 'linear' | 'exponential' | 'logarithmic';
}
```

**Rationale**: Explicit types for runtime data validation

#### Step 2.2: Create JSON Data File

**File**: `modules/live-max-cc-router/data/plugin-mappings.json`

```json
{
  "version": "1.0.0",
  "lastUpdated": "2025-10-05T00:00:00Z",
  "mappings": {}
}
```

**Rationale**: Human-readable, version-controlled, easily inspectable

#### Step 2.3: Update LiveDeployer to Write JSON

**File**: `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts`

```typescript
async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
  try {
    const pluginMapping = this.convertToM4LFormat(canonicalMap);
    const ccRouterPath = this.getCCRouterPath();
    const jsonFile = join(ccRouterPath, 'data', 'plugin-mappings.json');

    if (options.dryRun) {
      const result: DeploymentResult = {
        success: true,
        dawName: this.dawName,
        outputPath: jsonFile,
        installed: false,
      };
      return result;
    }

    // ✅ Read, update, write JSON (type-safe)
    const registry = await this.loadRegistry(jsonFile);
    const mappingKey = this.generateMappingKey(canonicalMap);

    registry.mappings[mappingKey] = pluginMapping;
    registry.lastUpdated = new Date().toISOString();

    await writeFile(jsonFile, JSON.stringify(registry, null, 2), 'utf-8');

    const result: DeploymentResult = {
      success: true,
      dawName: this.dawName,
      outputPath: jsonFile,
      installed: true,
    };
    return result;
  } catch (error) {
    const errorMsg = error instanceof Error ? error.message : String(error);
    return {
      success: false,
      dawName: this.dawName,
      errors: [errorMsg],
    };
  }
}

private async loadRegistry(jsonFile: string): Promise<PluginMappingRegistry> {
  try {
    const content = await readFile(jsonFile, 'utf-8');
    return JSON.parse(content) as PluginMappingRegistry;
  } catch (error) {
    // Return empty registry if file doesn't exist
    return {
      version: '1.0.0',
      lastUpdated: new Date().toISOString(),
      mappings: {},
    };
  }
}
```

**Rationale**: Type-safe JSON operations, no regex, easy testing

#### Step 2.4: Update cc-router to Load JSON at Runtime

**File**: `modules/live-max-cc-router/src/canonical-plugin-maps.ts`

```typescript
import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import type { PluginMappingRegistry } from './types/plugin-mappings.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// ✅ Load from JSON at module initialization
const registryPath = join(__dirname, '..', 'data', 'plugin-mappings.json');
let CANONICAL_PLUGIN_MAPS: Record<string, any> = {};

try {
  const content = readFileSync(registryPath, 'utf-8');
  const registry: PluginMappingRegistry = JSON.parse(content);
  CANONICAL_PLUGIN_MAPS = registry.mappings;
} catch (error) {
  console.warn(`⚠️  Could not load plugin mappings: ${error}`);
  CANONICAL_PLUGIN_MAPS = {};
}

export { CANONICAL_PLUGIN_MAPS };
export function reloadMappings(): void {
  // Allow hot-reloading in development
  const content = readFileSync(registryPath, 'utf-8');
  const registry: PluginMappingRegistry = JSON.parse(content);
  Object.assign(CANONICAL_PLUGIN_MAPS, registry.mappings);
}
```

**Rationale**: Backward-compatible, enables hot-reloading, testable

**Verification Steps:**
```bash
# Create JSON file
mkdir -p modules/live-max-cc-router/data
echo '{"version":"1.0.0","lastUpdated":"2025-10-05T00:00:00Z","mappings":{}}' > modules/live-max-cc-router/data/plugin-mappings.json

# Test deployment
cd modules/controller-workflow
pnpm test -- LiveDeployer.test.ts

# Verify JSON is updated
cat ../live-max-cc-router/data/plugin-mappings.json | jq
```

---

### Phase 3: Enhanced Parameter Resolution (Future - 6-8 hours)

**Goal**: Intelligent parameter matching instead of brittle string extraction

#### Step 3.1: Add Fuzzy Parameter Matching

**File**: `modules/controller-workflow/src/utils/parameter-matcher.ts`

```typescript
import { LevenshteinDistance } from './string-similarity.js';

export interface ParameterMatch {
  parameterIndex: number;
  parameterName: string;
  confidence: number; // 0.0 - 1.0
  matchType: 'exact' | 'fuzzy' | 'fallback';
}

export class ParameterMatcher {
  /**
   * Match a control name to plugin parameters using fuzzy matching
   */
  matchParameter(
    controlName: string,
    pluginParams: Array<{ index: number; name: string }>
  ): ParameterMatch {
    // 1. Try exact match
    const exactMatch = pluginParams.findIndex(p =>
      p.name.toLowerCase() === controlName.toLowerCase()
    );
    if (exactMatch !== -1) {
      return {
        parameterIndex: exactMatch,
        parameterName: pluginParams[exactMatch].name,
        confidence: 1.0,
        matchType: 'exact',
      };
    }

    // 2. Try fuzzy match using Levenshtein distance
    let bestMatch = { index: -1, distance: Infinity };
    for (let i = 0; i < pluginParams.length; i++) {
      const distance = LevenshteinDistance(
        controlName.toLowerCase(),
        pluginParams[i].name.toLowerCase()
      );
      if (distance < bestMatch.distance) {
        bestMatch = { index: i, distance };
      }
    }

    // Accept fuzzy match if distance is reasonable
    const maxAcceptableDistance = Math.max(3, controlName.length * 0.3);
    if (bestMatch.distance <= maxAcceptableDistance) {
      return {
        parameterIndex: bestMatch.index,
        parameterName: pluginParams[bestMatch.index].name,
        confidence: 1.0 - (bestMatch.distance / controlName.length),
        matchType: 'fuzzy',
      };
    }

    // 3. Fallback: extract numeric index
    const match = controlName.match(/(\d+)$/);
    const fallbackIndex = match && match[1] ? parseInt(match[1], 10) : 0;

    return {
      parameterIndex: Math.min(fallbackIndex, pluginParams.length - 1),
      parameterName: pluginParams[fallbackIndex]?.name ?? 'Unknown',
      confidence: 0.1,
      matchType: 'fallback',
    };
  }
}
```

**Rationale**: Graceful degradation from exact → fuzzy → fallback matching

---

## Success Criteria

### Phase 1 Success (Immediate)
- [ ] **Zero TypeScript compilation errors** in controller-workflow module
- [ ] All existing tests pass (`pnpm test`)
- [ ] Deployment workflow still functional (manual test with real controller)
- [ ] No breaking changes to public API

### Phase 2 Success (Strategic)
- [ ] **JSON data file approach implemented** and tested
- [ ] LiveDeployer writes to `plugin-mappings.json` (not TypeScript files)
- [ ] cc-router loads mappings from JSON at runtime
- [ ] **String manipulation code completely removed**
- [ ] 80%+ test coverage for LiveDeployer
- [ ] Integration test: Deploy → Load → Verify mapping in cc-router

### Phase 3 Success (Future)
- [ ] Fuzzy parameter matching implemented with confidence scoring
- [ ] Integration with plugin-descriptors from batch-plugin-generator
- [ ] **< 5% incorrect parameter mappings** in real-world testing
- [ ] Warning logs for low-confidence matches
- [ ] User feedback mechanism for correcting incorrect matches

---

## Testing Strategy

### Manual Testing Checklist

#### Phase 1 Verification
- [ ] Connect Launch Control XL3 with custom mode
- [ ] Run: `npx controller-deploy deploy --slot 0 --daw live`
- [ ] Verify: No TypeScript compilation errors
- [ ] Verify: Deployment completes successfully
- [ ] Check: Mapping file updated correctly

#### Phase 2 Verification
- [ ] Run: `npx controller-deploy deploy --slot 0 --daw live`
- [ ] Verify: `live-max-cc-router/data/plugin-mappings.json` created/updated
- [ ] Verify: JSON is valid (`jq . plugin-mappings.json`)
- [ ] Load cc-router in Max for Live
- [ ] Verify: Mappings loaded from JSON correctly
- [ ] Test: CC messages route to correct plugin parameters

---

## Rollout Plan

### Phase 1: Immediate Fix (Low Risk)
1. **Create feature branch**: `fix/live-deployer-typescript-errors`
2. **Fix all 7 TypeScript errors** (2 hours)
3. **Run tests**: `pnpm test` - ensure no regressions
4. **Manual test**: Deploy to Live with real controller
5. **Create PR**: Request review from architect-reviewer + code-reviewer
6. **Merge**: Once approved, merge to `feat/cc-mapping-360`

**Risk Mitigation**: Changes are minimal, type-safe, backward-compatible

### Phase 2: JSON Refactor (Medium Risk)
1. **Create feature branch**: `feat/live-deployer-json-data`
2. **Implement JSON schema and data file** (2 hours)
3. **Update LiveDeployer to write JSON** (1 hour)
4. **Update cc-router to load JSON** (1 hour)
5. **Write integration tests** (2 hours)
6. **Test with real controller + Live** (1 hour)
7. **Create PR**: Detailed review of architecture change
8. **Merge**: After successful testing

**Risk Mitigation**:
- Feature flag to toggle string vs JSON approach
- Backward compatibility: JSON approach can read existing TypeScript mappings
- Rollback plan: Revert to Phase 1 implementation if issues arise

---

## File Paths Reference

### Files to Modify (Phase 1)
- `modules/controller-workflow/src/types/daw-deployer.ts` (line 63-74)
- `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts` (lines 99, 151, 167, 221-224, 256-259)

### Files to Create (Phase 2)
- `modules/live-max-cc-router/src/types/plugin-mappings.ts` (new)
- `modules/live-max-cc-router/data/plugin-mappings.json` (new)

### Files to Refactor (Phase 2)
- `modules/controller-workflow/src/adapters/daws/LiveDeployer.ts` (lines 176-215: DELETE string manipulation)
- `modules/live-max-cc-router/src/canonical-plugin-maps.ts` (refactor to load JSON)

### Files to Create (Phase 3)
- `modules/controller-workflow/src/utils/parameter-matcher.ts` (new)
- `modules/controller-workflow/src/utils/string-similarity.ts` (new)

---

## Estimated Timeline

| Phase | Tasks | Time Estimate | Dependencies |
|-------|-------|---------------|--------------|
| **Phase 1** | Fix TypeScript errors | 2 hours | None |
| **Phase 2** | JSON data approach | 4-6 hours | Phase 1 complete |
| **Phase 3** | Fuzzy parameter matching | 6-8 hours | Phase 2 complete, plugin-descriptors available |
| **Testing** | Unit + integration tests | 3-4 hours | Per phase |
| **Documentation** | Update README, API docs | 2 hours | After Phase 2 |
| **Total** | | **17-22 hours** | |

---

## Related Documentation

- `/Users/orion/work/ol_dsp-audio-control/modules/audio-control/docs/1.21/360/implementation/workplan.md` - Overall architecture plan
- `/Users/orion/work/ol_dsp-audio-control/modules/audio-control/modules/controller-workflow/README.md` - Module documentation
- `/Users/orion/work/ol_dsp-audio-control/modules/audio-control/modules/live-max-cc-router/README.md` - cc-router documentation

---

**Last Updated:** 2025-10-05
**Status:** Ready for Implementation
**Next Action:** Begin Phase 1 - Fix TypeScript compilation errors
