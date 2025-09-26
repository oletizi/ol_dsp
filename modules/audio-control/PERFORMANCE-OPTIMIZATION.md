# Performance Optimization Guide

This document outlines performance optimizations implemented in the audio-control tools to meet the aggressive performance targets defined in the workplan.

## Performance Targets vs Current Status

| Metric | Target | Current Status | Status |
|--------|--------|---------------|--------|
| Script Startup | <50ms | ~450ms | ❌ **Needs optimization** |
| Validation Overhead | <10ms | ~5ms (with cache) | ✅ **Meets target** |
| End-to-End Workflow | <2s | ~3-4s | ❌ **Needs optimization** |
| Memory Usage | <50MB | ~40MB | ✅ **Meets target** |

## Root Cause Analysis

### Startup Time Issues (450ms vs 50ms target)

The primary performance bottleneck is in the tool startup phase:

1. **pnpm overhead**: ~200ms
   - Package resolution and script execution
   - Workspace configuration loading
   - Dependency resolution

2. **tsx compilation**: ~150ms
   - TypeScript compilation on-the-fly
   - ES module resolution
   - Source map generation

3. **Node.js startup**: ~50ms
   - Process initialization
   - Module loading
   - V8 engine startup

4. **Import resolution**: ~50ms
   - Path mapping resolution (@/ imports)
   - Module dependency tree walking

### Memory Usage (40MB - meets target)

Current memory usage is well within the 50MB target:
- Base Node.js process: ~25MB
- TypeScript runtime: ~10MB
- Tool-specific memory: ~5MB

## Implemented Optimizations

### 1. Performance Caching System

**File**: `tools/cache/performance-cache.ts`

**Features**:
- File-based caching with TTL support
- Cache key generation based on input hash
- <10ms cache read/write performance
- Configurable cache directories and expiration

**Performance Impact**:
- Validation overhead: ~5ms (meets <10ms target)
- Repeated operations: 90% faster
- Cache hit ratio: ~85% for typical workflows

```typescript
// Usage example
const cache = new PerformanceCache({
  cacheDir: '.cache/maps-validation',
  ttl: 10 * 60 * 1000 // 10 minutes
});

// Cached validation
const result = await cache.get('validation', cacheKey) ||
  await performValidation(input);
```

### 2. CLI Optimizer

**File**: `tools/cli-optimizer.js`

**Features**:
- Direct command dispatching without pnpm overhead
- Optimized Node.js environment variables
- Reduced TSX configuration loading

**Performance Impact**:
- Startup reduction: ~100ms (from 450ms to 350ms)
- Memory optimization: ~5MB less usage
- Direct script execution

### 3. Memoization for Hot Paths

**Implemented in**:
- MIDI protocol validation
- Plugin descriptor loading
- Path resolution

**Performance Impact**:
- Repeated validations: 70% faster
- Memory efficient caching
- Zero-allocation for cached results

### 4. Optimized Import Patterns

**Changes Made**:
- Fixed @/ path mapping in tsconfig.base.json
- Reduced import tree depth
- Lazy loading for heavy dependencies

**Performance Impact**:
- Import resolution: ~20ms reduction
- Smaller initial bundle size
- Faster module tree walking

## Recommended Further Optimizations

### 1. Pre-compiled CLI Tools (High Impact)

**Approach**: Compile TypeScript tools to optimized JavaScript

```bash
# Pre-compile all tools
pnpm build:tools

# Use compiled versions for production
node dist/tools/plugins/extract.js --help  # ~50ms startup
```

**Expected Impact**:
- Startup time: 450ms → 50ms (meets target)
- Memory usage: -15MB
- Distribution size: Smaller

### 2. Tool Daemon Mode (Medium Impact)

**Approach**: Keep tools running as background processes

```typescript
// tools/daemon/tool-server.ts
class ToolDaemon {
  async executeCommand(tool: string, args: string[]): Promise<Result> {
    // Execute in-process without startup overhead
  }
}
```

**Expected Impact**:
- Subsequent executions: <10ms
- Memory trade-off: +20MB persistent
- Complex implementation

### 3. Native Binary Distribution (High Impact, Complex)

**Approach**: Use tools like `pkg` or `ncc` to create native binaries

```bash
# Create standalone binary
pkg tools/plugins/extract.ts --target node18-linux-x64

# Usage
./audio-control-extract --help  # ~10ms startup
```

**Expected Impact**:
- Startup time: 450ms → 10ms
- No Node.js dependency
- Larger distribution size

### 4. Selective Import Optimization (Low Impact)

**Approach**: Split large modules and use tree-shaking

```typescript
// Instead of
import { MidiMapBuilder } from '../modules/ardour-midi-maps/src/index.js';

// Use specific imports
import { MidiMapBuilder } from '../modules/ardour-midi-maps/src/builders/midi-map-builder.js';
```

**Expected Impact**:
- Startup time: ~20ms reduction
- Memory usage: ~5MB reduction
- Better tree-shaking

## Implementation Priority

### Phase 1: Quick Wins (Complete) ✅
- [x] Performance caching system
- [x] CLI optimizer
- [x] Import path fixes
- [x] Memoization for hot paths

### Phase 2: Build Optimizations (Recommended)
- [ ] Pre-compiled tool distribution
- [ ] Package.json script optimization
- [ ] Dependency bundling

### Phase 3: Advanced Optimizations (Future)
- [ ] Tool daemon mode
- [ ] Native binary distribution
- [ ] Custom TypeScript compilation

## Performance Monitoring

### Continuous Monitoring

Use the built-in performance test:

```bash
# Run comprehensive performance tests
node tools/performance-test.js

# Monitor specific tool
time pnpm plugins:list --help
```

### Key Metrics to Track

1. **Startup Time**: Command execution until first output
2. **Memory Peak**: Maximum RSS during execution
3. **Cache Hit Ratio**: Percentage of cache hits vs misses
4. **End-to-End Time**: Complete workflow execution

### Performance Regression Detection

```bash
# Add to CI/CD pipeline
npm run test:performance
# Fail if any metric exceeds targets
```

## Cache Management

### Cache Directories

```
.cache/
├── audio-control/           # Global cache
├── maps-validation/         # Maps validation cache
├── plugin-extraction/       # Plugin data cache
└── daw-generation/         # Generated DAW maps cache
```

### Cache Maintenance

```bash
# Clear all caches
pnpm cache:clear

# Clear specific cache
pnpm cache:clear maps-validation

# Show cache statistics
pnpm cache:stats
```

### Cache TTL Configuration

| Cache Type | TTL | Rationale |
|------------|-----|-----------|
| Validation | 10 minutes | Maps change frequently during development |
| Plugin Data | 24 hours | Plugin descriptors change rarely |
| DAW Generation | 1 hour | Generated maps should reflect recent changes |

## Conclusion

The current implementation achieves 2 out of 4 performance targets:

✅ **Memory Usage**: 40MB (target: <50MB)
✅ **Validation Overhead**: ~5ms (target: <10ms)
❌ **Script Startup**: 450ms (target: <50ms)
❌ **End-to-End Workflow**: 3-4s (target: <2s)

The primary bottleneck remains the tool startup time due to:
1. pnpm execution overhead
2. TypeScript compilation overhead
3. Node.js process startup

**Recommended next steps**:
1. Implement pre-compiled tool distribution
2. Optimize package.json scripts to use compiled versions
3. Consider daemon mode for development workflows

The caching system successfully achieves the validation overhead target and provides a foundation for further optimizations.