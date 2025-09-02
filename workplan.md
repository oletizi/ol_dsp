# OL_DSP Dependencies Workplan

## Problem Statement

Current Docker CI setup has complexity around copying/excluding source files vs pre-built dependencies. The main issues:
- `libs/dattorro-verb/` is source code (not submodule) but gets excluded to preserve pre-built deps
- CMakeLists.txt expects dependencies in `libs/` but Docker pre-builds them there, causing conflicts
- Complex symlink and copying logic that's error-prone

## New Strategy: `.ol_dsp-deps/` Approach

### Overview
- Clone all submodule dependencies to `../.ol_dsp-deps/<path>`
- Pre-build all dependencies in `.ol_dsp-deps/`
- Update CMakeLists.txt to reference `../.ol_dsp-deps/` paths
- Create make target for dependency management (works Docker + local)
- Eliminate all copying/excluding logic

### Benefits
- Clean separation: source code vs dependencies
- Works identically in Docker and local development
- No more complex copying/symlink logic
- Dependencies pre-built and cached properly
- Source code like `libs/dattorro-verb/` stays in main repo

## Implementation Steps

### Phase 1: Dependency Management System
- [x] 1.1: Create `scripts/setup-deps.sh` script
  - Reads `submodules.json`
  - Clones each dependency to `../.ol_dsp-deps/<path>`
  - Checks out correct commit for each dependency
  - Pre-builds dependencies with CMake where applicable
- [x] 1.2: Add `make setup-deps` target
  - Calls `scripts/setup-deps.sh`
  - Can be used on laptop or in Docker
- [x] 1.3: Test `make setup-deps` locally
  - ✅ JUCE pre-built successfully with juceaide
  - ✅ DaisySP, googletest, FakeIt pre-built
  - ⚠️ Some deps failed build (platform-specific) but are available

### Phase 2: CMake Integration
- [x] 2.1: Update `CMakeLists.txt` to reference `../.ol_dsp-deps/`
  - ✅ Changed `add_subdirectory(libs/JUCE)` → `add_subdirectory(../.ol_dsp-deps/libs/JUCE ${CMAKE_BINARY_DIR}/deps/JUCE)`
  - ✅ Changed `add_subdirectory(libs/rtmidi)` → `add_subdirectory(../.ol_dsp-deps/libs/rtmidi ${CMAKE_BINARY_DIR}/deps/rtmidi)`
  - ✅ Added binary directories for out-of-tree sources
  - ✅ Updated stmlib paths to use `.ol_dsp-deps`
  - ✅ Updated test dependencies (googletest, FakeIt)
  - ✅ Kept `add_subdirectory(libs/dattorro-verb)` (source code, not dependency)
- [x] 2.2: Test local build with new CMake setup
  - ✅ JUCE successfully pre-built and used (juceaide working)
  - ✅ DaisySP, stmlib, ol_corelib, ol_ctllib all building
  - ✅ All CMake dependency errors resolved
  - ⚠️ Build takes long time (includes JUCE examples) but succeeding
- [x] 2.3: Fix any path/build issues
  - ✅ Fixed out-of-tree source binary directory requirements
  - ✅ Fixed test dependency paths

### Phase 3: Docker Integration
- [ ] 3.1: Update `cpp-builder.Dockerfile`
  - Remove current submodule caching logic
  - Add `RUN make setup-deps` to pre-build dependencies
- [ ] 3.2: Simplify CI scripts
  - Remove all copying/excluding logic from `01-clone-repo.sh`
  - Just clone repo normally - no special handling needed
- [ ] 3.3: Update `cache-submodules.py` or remove if no longer needed

### Phase 4: Testing & Cleanup
- [ ] 4.1: Test full Docker CI pipeline
- [ ] 4.2: Test local development workflow
- [ ] 4.3: Remove old submodule files and scripts
- [ ] 4.4: Update documentation

## File Changes Required

### New Files
- `scripts/setup-deps.sh` - Dependency setup script
- `workplan.md` - This file

### Modified Files
- `Makefile` - Add `setup-deps` target
- `CMakeLists.txt` - Update paths to `../.ol_dsp-deps/`
- `.docker/cpp-builder.Dockerfile` - Use new dependency approach
- `scripts/docker-ci-steps/01-clone-repo.sh` - Simplify (no exclusions)

### Potentially Removed Files
- `.docker/cache-submodules.py` - May no longer be needed
- Complex logic in CI scripts

## Directory Structure (After Implementation)

```
ol_dsp/                          # Main repository
├── libs/
│   ├── dattorro-verb/          # Source code (stays here)
│   └── [other source libs]/
├── scripts/setup-deps.sh       # New dependency script
└── [rest of source code]

ol_dsp-deps/                     # Dependencies (separate)
├── libs/
│   ├── JUCE/                   # Cloned submodule
│   │   └── build/              # Pre-built
│   ├── stk/                    # Cloned submodule
│   │   └── build/              # Pre-built
│   └── [other deps]/
└── test/
    ├── googletest/             # Cloned submodule
    └── FakeIt/                 # Cloned submodule
```

## Current Status
- [x] Problem identified and strategy designed  
- [x] **Phase 1 Complete**: Dependency management system working
- [x] **Phase 2 Complete**: CMake integration successful
- [x] **Local builds working**: JUCE pre-built, all dependencies resolved
- [ ] Phase 3: Docker integration
- [ ] Phase 4: Testing & cleanup

## Notes
- This approach eliminates the need for complex Docker copying/excluding logic
- Dependencies are completely separate from source code
- Same workflow works on laptop and in Docker
- CMakeLists.txt becomes more explicit about where dependencies come from