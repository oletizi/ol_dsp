# Audio Control Tooling Review & Streamlining Recommendations

## Executive Summary

The audio-control monorepo currently suffers from **significant tooling proliferation** with 20+ npm scripts scattered across modules, many of which are redundant, deprecated, or poorly aligned with the documented workflow. This review recommends consolidating to **12 focused, workflow-aligned scripts** that eliminate confusion and provide clear entry points for each phase of the process.

## Current State Analysis

### Discovered Issues

#### 1. **Workflow Misalignment**
The documented 3-phase workflow (Plugin Interrogation → Canonical Mapping → DAW Generation) is poorly reflected in the current tooling:
- Phase 1 has 8+ different plugin interrogation variants
- Phase 2 has no dedicated tooling support
- Phase 3 has inconsistent script names

#### 2. **Script Proliferation**
**Total Scripts Found: 23 user-facing scripts**
- `canonical-midi-maps`: 11 scripts
- `ardour-midi-maps`: 6 scripts
- Duplicated functionality across modules
- Multiple deprecated variants still present

#### 3. **Naming Inconsistencies**
Current naming follows no clear pattern:
- `plugin:generate-batch` vs `generate:plugin-specs`
- `generate:install` vs `generate:ardour:install`
- Mixed use of colons, hyphens, and prefixes

#### 4. **Broken/Deprecated Tools**
Several scripts are non-functional or obsolete:
- `validate-maps` fails due to missing schema
- Multiple deprecated `generate-plugin-specs` variants
- Inconsistent parameter handling

## Detailed Script Inventory

### canonical-midi-maps Module
```
✅ Keep: plugin:generate-batch (core functionality)
🔄 Rename: validate-maps → maps:validate
❌ Remove: generate:plugin-specs* (4 variants - all deprecated)
❌ Remove: plugin:interrogate* (6 variants - duplicates batch)
```

### ardour-midi-maps Module
```
✅ Keep: generate (core functionality)
🔄 Rename: generate:install → generate:ardour:install
❌ Remove: Duplicate generation scripts
```

### Missing Tools
Critical gaps in the tooling:
- No validation for canonical mappings
- No health check for plugin descriptors
- No end-to-end workflow command
- No cleanup/maintenance tools

## Streamlining Recommendations

### Proposed Workflow-Aligned Structure

#### **Phase 1: Plugin Interrogation**
```bash
# Core plugin parameter extraction
pnpm plugins:extract              # Main batch extraction command
pnpm plugins:extract --force      # Force re-extraction (bypass cache)
pnpm plugins:list                 # List available plugins
pnpm plugins:health               # Validate extracted descriptors
```

#### **Phase 2: Canonical Mapping**
```bash
# Mapping validation and management
pnpm maps:validate                # Validate canonical mapping files
pnpm maps:list                    # List available canonical mappings
pnpm maps:check                   # Health check (validate against descriptors)
```

#### **Phase 3: DAW Generation**
```bash
# DAW-specific generation
pnpm daw:generate                 # Generate all DAW formats
pnpm daw:generate:ardour          # Generate Ardour only
pnpm daw:generate:ardour --install # Generate and install to Ardour
pnpm daw:list                     # List generated DAW files
```

#### **Workflow Management**
```bash
# End-to-end commands
pnpm workflow:complete            # Run complete workflow (extract → validate → generate)
pnpm workflow:health              # System health check across all phases
```

### Implementation Strategy

#### **1. Script Consolidation**
- **Remove 11 redundant/deprecated scripts**
- **Consolidate to 12 workflow-focused commands**
- **Implement consistent parameter handling**

#### **2. Naming Convention**
Adopt `{phase}:{action}:{modifier}` pattern:
- `plugins:extract` (Phase 1, primary action)
- `maps:validate` (Phase 2, primary action)
- `daw:generate:ardour` (Phase 3, action, target)

#### **3. Missing Functionality**
Add critical missing tools:
- **Health checks**: Validate descriptor integrity
- **Cross-validation**: Ensure mappings match descriptors
- **Installation helpers**: Automated DAW configuration
- **Cleanup tools**: Remove stale/invalid files

### Migration Plan

#### **Phase 1: Immediate (Week 1)**
1. **Remove deprecated scripts**: Clean up 11 obsolete commands
2. **Implement core renames**: Update to new naming convention
3. **Add missing validation**: Implement `maps:validate` and `plugins:health`

#### **Phase 2: Enhancement (Week 2)**
1. **Add workflow commands**: Implement `workflow:complete` and `workflow:health`
2. **Improve parameter handling**: Consistent flags across all scripts
3. **Add installation helpers**: Automated DAW config deployment

#### **Phase 3: Polish (Week 3)**
1. **Update documentation**: Align PROCESS.md with new tooling
2. **Add completion scripts**: Shell auto-completion support
3. **Performance optimization**: Caching and parallel execution

### Expected Benefits

#### **1. Reduced Cognitive Load**
- **70% fewer scripts** to remember (23 → 12)
- **Clear workflow mapping** (each phase has obvious entry points)
- **Consistent naming** reduces guesswork

#### **2. Improved Reliability**
- **Remove broken tools** that cause user confusion
- **Add missing validation** to catch errors early
- **Health checks** ensure system integrity

#### **3. Better User Experience**
- **Single workflow command** for common use case
- **Granular control** when needed
- **Clear feedback** on system state

### Risk Mitigation

#### **Breaking Changes**
- **Deprecation notices**: Warn before removing old scripts
- **Alias support**: Temporary backwards compatibility
- **Clear migration guide**: Step-by-step transition instructions

#### **Testing Strategy**
- **Comprehensive testing** of all new scripts
- **User acceptance testing** with existing workflows
- **Documentation validation** against actual behavior

## Conclusion

The current tooling landscape creates unnecessary complexity and user confusion. The proposed streamlining reduces script count by 70% while adding essential missing functionality. The workflow-aligned structure makes the system more discoverable and maintainable.

**Next Steps:**
1. Review and approve recommendations
2. Begin Phase 1 implementation (remove deprecated scripts)
3. Implement missing validation tools
4. Update documentation to reflect new structure

This consolidation effort will significantly improve developer experience while maintaining all essential functionality needed for the plugin-to-DAW mapping workflow.