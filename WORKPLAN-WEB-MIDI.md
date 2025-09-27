# Web MIDI API Support Implementation Workplan

**Project**: @oletizi/launch-control-xl3
**Feature**: Browser Environment Support via Web MIDI API
**Issue**: [#11](https://github.com/oletizi/ol_dsp/issues/11)
**Date**: 2025-09-27
**Estimated Effort**: 65-85 hours

## Executive Summary

Implement Web MIDI API support to enable the `@oletizi/launch-control-xl3` library to function in browser environments while maintaining 100% backward compatibility with existing Node.js usage. The current architecture's `MidiBackendInterface` abstraction provides an excellent foundation for this enhancement.

### Key Objectives

1. Enable browser-based applications to use the library
2. Maintain zero breaking changes for existing Node.js users
3. Support full SysEx communication for custom modes
4. Auto-detect environment and select appropriate backend
5. Minimize bundle size impact (<5KB increase)

## Current State Analysis

### Strengths
- ✅ Well-designed `MidiBackendInterface` abstraction
- ✅ Dependency injection patterns throughout
- ✅ Comprehensive SysEx implementation
- ✅ Vite + TypeScript build system ready
- ✅ Good test coverage foundation

### Blockers
- ❌ Node.js `EventEmitter` dependency prevents browser usage
- ❌ No Web MIDI backend implementation
- ❌ Build system not configured for browser outputs
- ❌ Missing browser-specific tests

## Recommended Implementation Approach

**Hybrid Solution** combining Options 1 + 2 from the GitHub issue:

1. **Replace EventEmitter**: Use `eventemitter3` (browser-compatible)
2. **Web MIDI Backend**: Implement new backend following existing patterns
3. **Auto-Detection**: Automatically select backend based on environment
4. **Dual Builds**: Configure separate Node.js and browser builds

### Why This Approach?

- Leverages existing architecture (no refactoring needed)
- Minimal code changes = lower risk
- Full backward compatibility guaranteed
- Tree-shaking eliminates unused backends
- Clean separation of concerns

## Implementation Phases

### Phase 1: Event Emitter Compatibility

**Goal**: Replace Node.js EventEmitter with browser-compatible alternative

**Estimated Effort**: 8-12 hours

#### Tasks

1. **Add eventemitter3 dependency** (1 hour)
   - Add to package.json dependencies
   - Install and verify
   - Check bundle size impact

2. **Update LaunchControlXL3 class** (2-3 hours)
   - Replace `import { EventEmitter } from 'events'`
   - Update to `import { EventEmitter } from 'eventemitter3'`
   - Verify type compatibility
   - Update any EventEmitter-specific code

3. **Test existing functionality** (3-4 hours)
   - Run full test suite
   - Verify all events still work
   - Test with real LCXL3 hardware
   - Ensure no breaking changes

4. **Update TypeScript types** (1-2 hours)
   - Update type definitions if needed
   - Verify exports are correct
   - Check documentation generation

5. **Commit and verify** (1-2 hours)
   - Run pre-commit hooks
   - Verify build passes
   - Check bundle size delta

**Dependencies**: None

**Deliverables**:
- Updated package.json with eventemitter3
- Modified LaunchControlXL3.ts
- All existing tests passing
- Documentation of any API changes (should be none)

**Success Criteria**:
- ✅ All existing tests pass
- ✅ No breaking changes to public API
- ✅ Bundle size increase < 2KB
- ✅ Types still work correctly

---

### Phase 2: Web MIDI Backend Implementation

**Goal**: Create WebMidiBackend class implementing MidiBackendInterface

**Estimated Effort**: 20-25 hours

#### Tasks

1. **Create WebMidiBackend class skeleton** (2-3 hours)
   - Create `src/core/backends/WebMidiBackend.ts`
   - Implement MidiBackendInterface
   - Add environment detection
   - Basic error handling structure

2. **Implement core MIDI operations** (6-8 hours)
   - `initialize()`: Request MIDI access with SysEx
   - `getInputPorts()`: Map Web MIDI inputs
   - `getOutputPorts()`: Map Web MIDI outputs
   - `openInput()`: Create input port wrapper
   - `openOutput()`: Create output port wrapper
   - `closePort()`: Handle port cleanup
   - `cleanup()`: Close all ports

3. **Implement message handling** (4-5 hours)
   - `sendMessage()`: Send MIDI via Web MIDI API
   - Message event listeners
   - Convert between Web MIDI and internal formats
   - Handle message timing/scheduling

4. **SysEx support implementation** (4-5 hours)
   - Ensure SysEx permission requested
   - Handle SysEx message sending
   - Handle SysEx message receiving
   - Test with actual LCXL3 SysEx protocol

5. **Connection state management** (2-3 hours)
   - Handle device connect events
   - Handle device disconnect events
   - Port state monitoring
   - Reconnection logic

6. **Error handling** (2-3 hours)
   - Permission denied errors
   - Port not found errors
   - Connection errors
   - Descriptive error messages

**Dependencies**: Phase 1 complete

**Deliverables**:
- `WebMidiBackend.ts` fully implementing MidiBackendInterface
- SysEx communication working
- Connection management working
- Comprehensive error handling

**Success Criteria**:
- ✅ Implements all MidiBackendInterface methods
- ✅ SysEx messages send/receive correctly
- ✅ Handles all Web MIDI API edge cases
- ✅ Clear error messages for all failure modes
- ✅ Works with Chrome/Edge browsers

**Technical Specifications**:

```typescript
// src/core/backends/WebMidiBackend.ts
import type { MidiBackendInterface, MidiMessage, MidiPort, MidiPortInfo } from '../types';

export class WebMidiBackend implements MidiBackendInterface {
  private midiAccess: MIDIAccess | null = null;
  private inputPorts: Map<string, MIDIInput> = new Map();
  private outputPorts: Map<string, MIDIOutput> = new Map();
  private messageHandlers: Map<string, (message: MidiMessage) => void> = new Map();

  async initialize(): Promise<void> {
    if (typeof navigator === 'undefined' || !navigator.requestMIDIAccess) {
      throw new Error('Web MIDI API not available in this environment');
    }

    try {
      this.midiAccess = await navigator.requestMIDIAccess({ sysex: true });
      this.setupEventListeners();
    } catch (error: any) {
      if (error.name === 'SecurityError') {
        throw new Error('MIDI access denied. Please grant permission and ensure HTTPS.');
      }
      throw new Error(`Failed to initialize Web MIDI: ${error.message}`);
    }
  }

  async getInputPorts(): Promise<MidiPortInfo[]> {
    if (!this.midiAccess) {
      throw new Error('Web MIDI backend not initialized');
    }

    return Array.from(this.midiAccess.inputs.values()).map(input => ({
      id: input.id,
      name: input.name || 'Unknown Input',
      manufacturer: input.manufacturer || 'Unknown',
      state: input.state,
      type: 'input' as const
    }));
  }

  async getOutputPorts(): Promise<MidiPortInfo[]> {
    if (!this.midiAccess) {
      throw new Error('Web MIDI backend not initialized');
    }

    return Array.from(this.midiAccess.outputs.values()).map(output => ({
      id: output.id,
      name: output.name || 'Unknown Output',
      manufacturer: output.manufacturer || 'Unknown',
      state: output.state,
      type: 'output' as const
    }));
  }

  async openInput(portId: string, onMessage: (message: MidiMessage) => void): Promise<MidiPort> {
    if (!this.midiAccess) {
      throw new Error('Web MIDI backend not initialized');
    }

    const input = this.midiAccess.inputs.get(portId);
    if (!input) {
      throw new Error(`MIDI input port ${portId} not found`);
    }

    await input.open();

    input.onmidimessage = (event: MIDIMessageEvent) => {
      const message: MidiMessage = {
        data: Array.from(event.data),
        timestamp: event.timeStamp
      };
      onMessage(message);
    };

    this.inputPorts.set(portId, input);
    this.messageHandlers.set(portId, onMessage);

    return {
      id: input.id,
      name: input.name || 'Unknown',
      type: 'input'
    };
  }

  async openOutput(portId: string): Promise<MidiPort> {
    if (!this.midiAccess) {
      throw new Error('Web MIDI backend not initialized');
    }

    const output = this.midiAccess.outputs.get(portId);
    if (!output) {
      throw new Error(`MIDI output port ${portId} not found`);
    }

    await output.open();
    this.outputPorts.set(portId, output);

    return {
      id: output.id,
      name: output.name || 'Unknown',
      type: 'output'
    };
  }

  async sendMessage(portId: string, message: MidiMessage): Promise<void> {
    const output = this.outputPorts.get(portId);
    if (!output) {
      throw new Error(`MIDI output port ${portId} not open`);
    }

    if (!message.data || message.data.length === 0) {
      throw new Error('Cannot send empty MIDI message');
    }

    try {
      output.send(message.data, message.timestamp);
    } catch (error: any) {
      throw new Error(`Failed to send MIDI message: ${error.message}`);
    }
  }

  async closePort(portId: string): Promise<void> {
    const input = this.inputPorts.get(portId);
    if (input) {
      await input.close();
      this.inputPorts.delete(portId);
      this.messageHandlers.delete(portId);
    }

    const output = this.outputPorts.get(portId);
    if (output) {
      await output.close();
      this.outputPorts.delete(portId);
    }
  }

  async cleanup(): Promise<void> {
    for (const [portId] of this.inputPorts) {
      await this.closePort(portId);
    }
    for (const [portId] of this.outputPorts) {
      await this.closePort(portId);
    }
    this.midiAccess = null;
  }

  private setupEventListeners(): void {
    if (!this.midiAccess) return;

    this.midiAccess.onstatechange = (event: MIDIConnectionEvent) => {
      // Handle port connection/disconnection
      // This can be used to emit events for device monitoring
    };
  }
}
```

---

### Phase 3: Environment Detection & Auto-Selection

**Goal**: Automatically detect environment and select appropriate backend

**Estimated Effort**: 8-10 hours

#### Tasks

1. **Create backend factory** (3-4 hours)
   - Detect Node.js vs Browser environment
   - Select WebMidiBackend or NodeMidiBackend
   - Handle missing backends gracefully
   - Allow explicit backend override

2. **Update LaunchControlXL3 constructor** (2-3 hours)
   - Accept optional `midiBackend` parameter
   - Use factory if no backend provided
   - Maintain backward compatibility
   - Update TypeScript types

3. **Environment detection utilities** (2-3 hours)
   - Reliable browser detection
   - Node.js detection
   - Web MIDI API availability check
   - Feature detection helpers

4. **Testing** (1-2 hours)
   - Test auto-detection in Node.js
   - Test auto-detection in browser
   - Test explicit backend selection
   - Test error cases

**Dependencies**: Phase 2 complete

**Deliverables**:
- Backend factory implementation
- Updated LaunchControlXL3 constructor
- Environment detection utilities
- Tests for all scenarios

**Success Criteria**:
- ✅ Correctly detects Node.js environment
- ✅ Correctly detects browser environment
- ✅ Falls back gracefully when Web MIDI unavailable
- ✅ Allows explicit backend override
- ✅ Zero breaking changes

**Technical Specifications**:

```typescript
// src/core/backends/BackendFactory.ts
import type { MidiBackendInterface } from '../types';

export async function createMidiBackend(
  explicitBackend?: MidiBackendInterface
): Promise<MidiBackendInterface> {
  if (explicitBackend) {
    return explicitBackend;
  }

  if (isWebMidiAvailable()) {
    const { WebMidiBackend } = await import('./WebMidiBackend');
    return new WebMidiBackend();
  }

  if (isNodeEnvironment()) {
    const { NodeMidiBackend } = await import('./NodeMidiBackend');
    return new NodeMidiBackend();
  }

  throw new Error(
    'No MIDI backend available. Please provide an explicit backend or ensure ' +
    'you are running in a supported environment (Node.js or browser with Web MIDI API).'
  );
}

export function isWebMidiAvailable(): boolean {
  return (
    typeof navigator !== 'undefined' &&
    typeof navigator.requestMIDIAccess === 'function'
  );
}

export function isNodeEnvironment(): boolean {
  return (
    typeof process !== 'undefined' &&
    process.versions != null &&
    process.versions.node != null
  );
}
```

```typescript
// Update LaunchControlXL3 constructor
export interface LaunchControlXL3Options {
  autoConnect?: boolean;
  enableCustomModes?: boolean;
  enableLedControl?: boolean;
  midiBackend?: MidiBackendInterface; // New optional parameter
}

export class LaunchControlXL3 extends EventEmitter {
  private backend: MidiBackendInterface | null = null;

  constructor(private options: LaunchControlXL3Options = {}) {
    super();
  }

  async initialize(): Promise<void> {
    this.backend = await createMidiBackend(this.options.midiBackend);
    await this.backend.initialize();
    // ... rest of initialization
  }
}
```

---

### Phase 4: Build System Configuration

**Goal**: Configure dual builds for Node.js and browser environments

**Estimated Effort**: 12-15 hours

#### Tasks

1. **Configure Vite for dual builds** (4-5 hours)
   - ESM build for browsers
   - CJS build for Node.js
   - Proper externalization
   - Source maps
   - Type declaration generation

2. **Update package.json** (2-3 hours)
   - Configure `exports` field
   - Set `main`, `module`, `browser` fields
   - Update build scripts
   - Configure tree-shaking

3. **Bundle optimization** (3-4 hours)
   - Minimize browser bundle
   - Tree-shake unused backends
   - Code splitting if beneficial
   - Verify bundle sizes

4. **TypeScript configuration** (2-3 hours)
   - Update tsconfig for dual targets
   - Generate proper type declarations
   - Handle environment-specific types
   - Web MIDI API types

5. **Testing build outputs** (1-2 hours)
   - Test Node.js build
   - Test browser build
   - Verify imports work correctly
   - Check bundle sizes

**Dependencies**: Phases 1-3 complete

**Deliverables**:
- Updated Vite configuration
- Updated package.json with proper exports
- Optimized bundles for both environments
- Type declarations for both targets

**Success Criteria**:
- ✅ Browser bundle works in Chrome/Edge
- ✅ Node.js build works unchanged
- ✅ Bundle size increase < 5KB
- ✅ Tree-shaking eliminates unused code
- ✅ Type declarations accurate

**Technical Specifications**:

```json
// package.json updates
{
  "name": "@oletizi/launch-control-xl3",
  "version": "2.0.0",
  "type": "module",
  "main": "./dist/index.cjs",
  "module": "./dist/index.js",
  "browser": "./dist/browser.js",
  "types": "./dist/index.d.ts",
  "exports": {
    ".": {
      "browser": {
        "import": "./dist/browser.js",
        "types": "./dist/index.d.ts"
      },
      "node": {
        "import": "./dist/index.js",
        "require": "./dist/index.cjs",
        "types": "./dist/index.d.ts"
      },
      "default": {
        "import": "./dist/index.js",
        "require": "./dist/index.cjs",
        "types": "./dist/index.d.ts"
      }
    }
  },
  "files": [
    "dist"
  ],
  "dependencies": {
    "eventemitter3": "^5.0.1"
  },
  "peerDependencies": {
    "easymidi": "^3.0.0"
  },
  "peerDependenciesMeta": {
    "easymidi": {
      "optional": true
    }
  }
}
```

```typescript
// vite.config.ts
import { defineConfig } from 'vite';
import { resolve } from 'path';

export default defineConfig({
  build: {
    lib: {
      entry: {
        index: resolve(__dirname, 'src/index.ts'),
        browser: resolve(__dirname, 'src/browser.ts')
      },
      formats: ['es', 'cjs']
    },
    rollupOptions: {
      external: [
        'easymidi' // Node.js only, exclude from browser bundle
      ],
      output: {
        exports: 'named',
        globals: {}
      }
    },
    sourcemap: true,
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: false
      }
    }
  },
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src')
    }
  }
});
```

```typescript
// tsconfig.json updates
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "ESNext",
    "lib": ["ES2020", "DOM", "DOM.Iterable"],
    "types": ["webmidi"],
    "moduleResolution": "bundler",
    "declaration": true,
    "declarationMap": true,
    "sourceMap": true,
    "outDir": "./dist",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "resolveJsonModule": true,
    "paths": {
      "@/*": ["./src/*"]
    }
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist", "**/*.test.ts"]
}
```

---

### Phase 5: Testing & Documentation

**Goal**: Comprehensive testing and documentation for both environments

**Estimated Effort**: 17-23 hours

#### Tasks - Testing

1. **Unit tests for WebMidiBackend** (5-6 hours)
   - Mock Web MIDI API
   - Test all interface methods
   - Test error conditions
   - Test SysEx handling

2. **Integration tests** (5-6 hours)
   - Test with mock LCXL3 device
   - Test custom mode operations
   - Test connection lifecycle
   - Test message flow

3. **Browser testing** (4-5 hours)
   - Manual testing in Chrome/Edge
   - Test with real LCXL3 hardware
   - Playwright E2E tests
   - Cross-browser compatibility

4. **Regression testing** (2-3 hours)
   - Verify all existing Node.js tests pass
   - Test with existing applications
   - Performance testing
   - Memory leak testing

#### Tasks - Documentation

1. **Update README** (3-4 hours)
   - Browser usage examples
   - Web MIDI API requirements
   - Browser compatibility table
   - Troubleshooting guide

2. **API documentation** (2-3 hours)
   - Document WebMidiBackend
   - Document backend factory
   - Update TypeScript docs
   - Add code examples

3. **Migration guide** (1-2 hours)
   - Explain (zero) breaking changes
   - Browser usage patterns
   - Common issues and solutions

4. **Example applications** (2-3 hours)
   - Simple React example
   - Vanilla JS example
   - Node.js example (existing)

**Dependencies**: Phases 1-4 complete

**Deliverables**:
- Comprehensive test suite
- Browser E2E tests
- Updated documentation
- Example applications
- Migration guide

**Success Criteria**:
- ✅ Test coverage > 80%
- ✅ All tests pass in Node.js
- ✅ All tests pass in browser
- ✅ Real hardware tests successful
- ✅ Documentation complete and accurate

**Testing Specifications**:

```typescript
// __tests__/WebMidiBackend.test.ts
import { WebMidiBackend } from '@/core/backends/WebMidiBackend';

// Mock Web MIDI API
global.navigator = {
  requestMIDIAccess: jest.fn()
} as any;

describe('WebMidiBackend', () => {
  let backend: WebMidiBackend;
  let mockMIDIAccess: any;

  beforeEach(() => {
    mockMIDIAccess = {
      inputs: new Map(),
      outputs: new Map(),
      onstatechange: null
    };

    (global.navigator.requestMIDIAccess as jest.Mock).mockResolvedValue(mockMIDIAccess);
    backend = new WebMidiBackend();
  });

  describe('initialize', () => {
    it('should request MIDI access with SysEx permission', async () => {
      await backend.initialize();

      expect(global.navigator.requestMIDIAccess).toHaveBeenCalledWith({ sysex: true });
    });

    it('should throw descriptive error if Web MIDI not available', async () => {
      delete (global as any).navigator;

      await expect(backend.initialize()).rejects.toThrow(
        'Web MIDI API not available in this environment'
      );
    });

    it('should throw descriptive error if permission denied', async () => {
      const securityError = new Error('Permission denied');
      securityError.name = 'SecurityError';
      (global.navigator.requestMIDIAccess as jest.Mock).mockRejectedValue(securityError);

      await expect(backend.initialize()).rejects.toThrow(
        'MIDI access denied. Please grant permission and ensure HTTPS.'
      );
    });
  });

  describe('getInputPorts', () => {
    it('should return all available input ports', async () => {
      const mockInput = {
        id: 'input-1',
        name: 'Launch Control XL3',
        manufacturer: 'Novation',
        state: 'connected'
      };

      mockMIDIAccess.inputs.set('input-1', mockInput);
      await backend.initialize();

      const ports = await backend.getInputPorts();

      expect(ports).toHaveLength(1);
      expect(ports[0]).toMatchObject({
        id: 'input-1',
        name: 'Launch Control XL3',
        manufacturer: 'Novation',
        type: 'input'
      });
    });

    it('should throw error if not initialized', async () => {
      await expect(backend.getInputPorts()).rejects.toThrow(
        'Web MIDI backend not initialized'
      );
    });
  });

  describe('sendMessage', () => {
    it('should send SysEx messages correctly', async () => {
      const mockOutput = {
        id: 'output-1',
        name: 'Launch Control XL3',
        send: jest.fn(),
        open: jest.fn().mockResolvedValue(undefined)
      };

      mockMIDIAccess.outputs.set('output-1', mockOutput);
      await backend.initialize();
      await backend.openOutput('output-1');

      const sysexMessage = {
        data: [0xF0, 0x00, 0x20, 0x29, 0x02, 0x11, 0x77, 0x00, 0xF7]
      };

      await backend.sendMessage('output-1', sysexMessage);

      expect(mockOutput.send).toHaveBeenCalledWith(sysexMessage.data, undefined);
    });
  });
});
```

---

## Testing Strategy

### Unit Testing
- Mock Web MIDI API for deterministic tests
- Test all backend methods independently
- Test error conditions thoroughly
- Achieve >80% code coverage

### Integration Testing
- Test LaunchControlXL3 with WebMidiBackend
- Test custom mode operations end-to-end
- Test connection lifecycle
- Test with mock MIDI device

### Browser Testing
- Chrome/Edge with real LCXL3 hardware
- Test SysEx communication
- Test mode loading/saving
- Test connection state management

### Node.js Regression Testing
- All existing tests must pass
- No performance degradation
- No breaking changes
- Verify backward compatibility

### E2E Testing
- Playwright tests in browser
- Test full user workflows
- Test error scenarios
- Test browser compatibility

## Risk Assessment

### High Risk
- **SysEx timing in browsers**: Web MIDI handles timing differently
  - Mitigation: Extensive testing with real hardware

- **Browser compatibility**: Safari has no Web MIDI support
  - Mitigation: Clear documentation of supported browsers

### Medium Risk
- **Bundle size increase**: Could impact load times
  - Mitigation: Aggressive tree-shaking and optimization

- **Type compatibility**: eventemitter3 types may differ
  - Mitigation: Thorough type checking and tests

### Low Risk
- **Breaking changes**: Well-designed abstraction prevents this
  - Mitigation: Comprehensive regression testing

- **Performance**: Web MIDI should be comparable to Node.js
  - Mitigation: Performance benchmarks

## Success Criteria

### Functional Requirements
- ✅ Works in Chrome/Edge browsers with Web MIDI API
- ✅ All existing Node.js functionality unchanged
- ✅ SysEx communication works (loadCustomMode, saveCustomMode)
- ✅ Connection state management works
- ✅ Auto-detects environment correctly
- ✅ Allows explicit backend selection

### Non-Functional Requirements
- ✅ Zero breaking changes to public API
- ✅ Bundle size increase < 5KB
- ✅ Test coverage > 80%
- ✅ All existing tests pass
- ✅ Documentation complete and accurate
- ✅ Build succeeds without errors

### Quality Requirements
- ✅ Code follows project guidelines (DI, interfaces, @/ imports)
- ✅ All errors have descriptive messages
- ✅ No mock data or fallbacks outside tests
- ✅ Files under 500 lines
- ✅ Pre-commit hooks pass

## Timeline Estimate

### Sequential Development (1 developer)
- Phase 1: 1.5-2 weeks
- Phase 2: 2.5-3 weeks
- Phase 3: 1-1.5 weeks
- Phase 4: 1.5-2 weeks
- Phase 5: 2-3 weeks

**Total: 8-11 weeks** (65-85 hours)

### Parallel Development (2-3 developers)
- Phases 1-2 can run in parallel (after architecture agreement)
- Phase 3 depends on Phase 2
- Phase 4 can start after Phase 1
- Phase 5 can be parallelized

**Total: 5-7 weeks** with proper coordination

## Dependencies

### External Dependencies
- `eventemitter3`: ^5.0.1 (browser-compatible EventEmitter)
- `@types/webmidi`: Latest (TypeScript types for Web MIDI API)

### Browser Requirements
- Chrome 43+ (Web MIDI API)
- Edge 79+ (Chromium-based)
- Opera 30+ (Web MIDI API)
- HTTPS required for Web MIDI API
- User permission for MIDI access

### Development Dependencies
- Vite 5+ for building
- TypeScript 5.8+ with strict mode
- Jest for testing
- Playwright for E2E tests

## Post-Implementation Tasks

1. **Release Planning**
   - Major version bump (2.0.0) due to new features
   - Release notes highlighting browser support
   - Migration guide (though no changes needed)

2. **Community Outreach**
   - Blog post announcing browser support
   - Update npm package description
   - Example applications in separate repos

3. **Monitoring**
   - Track bundle size in CI
   - Monitor browser compatibility issues
   - Collect user feedback

4. **Future Enhancements**
   - Firefox support when Web MIDI stabilizes
   - Safari support if/when Web MIDI added
   - React/Vue component wrappers
   - WebAssembly optimizations if needed

## Questions for Product Owner

1. **Priority**: Is this feature high priority for 2.0 release?
2. **Timeline**: What is the target release date?
3. **Resources**: How many developers can work on this?
4. **Support**: Will we officially support Safari with polyfills?
5. **Examples**: Should we create official React/Vue examples?
6. **Documentation**: Host documentation on separate site?

## References

- [Web MIDI API Specification](https://www.w3.org/TR/webmidi/)
- [MDN: Web MIDI API](https://developer.mozilla.org/en-US/docs/Web/API/Web_MIDI_API)
- [Browser Support: Can I Use WebMIDI](https://caniuse.com/midi)
- [eventemitter3 Documentation](https://github.com/primus/eventemitter3)
- [GitHub Issue #11](https://github.com/oletizi/ol_dsp/issues/11)

## Appendix: Code Examples

### Example: Browser Usage (React)

```typescript
import { LaunchControlXL3 } from '@oletizi/launch-control-xl3';
import { useEffect, useState } from 'react';

export function useLCXL3() {
  const [device, setDevice] = useState<LaunchControlXL3 | null>(null);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const lcxl3 = new LaunchControlXL3({
      autoConnect: true,
      enableCustomModes: true
    });

    lcxl3.on('device:connected', () => setConnected(true));
    lcxl3.on('device:disconnected', () => setConnected(false));

    lcxl3.initialize().catch(console.error);
    setDevice(lcxl3);

    return () => {
      lcxl3.cleanup();
    };
  }, []);

  return { device, connected };
}

// Usage in component
function MIDIEditor() {
  const { device, connected } = useLCXL3();

  const loadMode = async () => {
    if (!device) return;
    const mode = await device.loadCustomMode(0);
    console.log('Loaded mode:', mode);
  };

  return (
    <div>
      <p>Status: {connected ? 'Connected' : 'Disconnected'}</p>
      <button onClick={loadMode} disabled={!connected}>
        Load Current Mode
      </button>
    </div>
  );
}
```

### Example: Vanilla JavaScript (Browser)

```html
<!DOCTYPE html>
<html>
<head>
  <title>LCXL3 Web Editor</title>
</head>
<body>
  <h1>Launch Control XL3 Editor</h1>
  <div id="status">Disconnected</div>
  <button id="connect">Connect</button>
  <button id="loadMode" disabled>Load Mode</button>

  <script type="module">
    import { LaunchControlXL3 } from '@oletizi/launch-control-xl3';

    const statusEl = document.getElementById('status');
    const connectBtn = document.getElementById('connect');
    const loadModeBtn = document.getElementById('loadMode');

    const device = new LaunchControlXL3({
      autoConnect: false,
      enableCustomModes: true
    });

    device.on('device:connected', () => {
      statusEl.textContent = 'Connected';
      loadModeBtn.disabled = false;
    });

    device.on('device:disconnected', () => {
      statusEl.textContent = 'Disconnected';
      loadModeBtn.disabled = true;
    });

    connectBtn.addEventListener('click', async () => {
      try {
        await device.initialize();
        await device.connect();
      } catch (error) {
        alert(`Error: ${error.message}`);
      }
    });

    loadModeBtn.addEventListener('click', async () => {
      try {
        const mode = await device.loadCustomMode(0);
        console.log('Mode:', mode);
        alert(`Loaded mode: ${mode.name}`);
      } catch (error) {
        alert(`Error: ${error.message}`);
      }
    });
  </script>
</body>
</html>
```

### Example: Node.js Usage (Unchanged)

```typescript
import { LaunchControlXL3 } from '@oletizi/launch-control-xl3';

// Existing code works exactly the same
const device = new LaunchControlXL3({
  autoConnect: true,
  enableCustomModes: true
});

device.on('device:connected', (info) => {
  console.log('Connected:', info.name);
});

await device.initialize();
const mode = await device.loadCustomMode(0);
console.log('Current mode:', mode);
```

---

**Document Version**: 1.0
**Last Updated**: 2025-09-27
**Status**: Ready for Review