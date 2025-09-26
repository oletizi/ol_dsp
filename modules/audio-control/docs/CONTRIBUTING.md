# Contributing to Audio Control

Welcome to the audio-control project! This guide will help you get started contributing to our TypeScript monorepo for audio device control and MIDI mapping utilities.

## Project Overview

Audio-control is a specialized library project providing:
- **Ardour MIDI Maps**: TypeScript utilities for generating Ardour DAW MIDI configurations
- **Canonical MIDI Maps**: DAW-agnostic MIDI mapping format with YAML/JSON support
- **Plugin Interrogation**: Extract accurate parameter data from VST3 plugins
- **Workflow Orchestration**: 12-script streamlined development workflow

## Quick Start

### Prerequisites

- **Node.js** >= 18.0.0
- **pnpm** >= 8.0.0 (required for monorepo management)
- **TypeScript** knowledge (strict mode enforced)
- **Basic MIDI/Audio** understanding helpful

### Development Environment Setup

1. **Clone and Install**
   ```bash
   git clone [repository-url]
   cd audio-control
   pnpm install
   ```

2. **Verify Installation**
   ```bash
   # Run all tests
   pnpm test

   # Check TypeScript compilation
   pnpm typecheck

   # Verify workflow scripts work
   pnpm workflow:health
   ```

3. **Development Dependencies**
   ```bash
   # Build tools
   pnpm build

   # Development server (where applicable)
   pnpm dev

   # Code formatting
   pnpm format:check
   ```

## Development Workflow

### The 12-Script Architecture

Our streamlined workflow uses 12 commands organized into three phases:

#### Phase 1: Plugin Interrogation
```bash
pnpm plugins:extract              # Extract plugin parameters
pnpm plugins:extract:force        # Force re-extraction (bypass cache)
pnpm plugins:list                 # List available plugins
pnpm plugins:health               # Validate extracted descriptors
```

#### Phase 2: Canonical Mapping
```bash
pnpm maps:validate                # Validate canonical mapping files
pnpm maps:list                    # List available canonical mappings
pnpm maps:check                   # Health check (validate against descriptors)
```

#### Phase 3: DAW Generation
```bash
pnpm daw:generate                 # Generate all DAW formats
pnpm daw:generate:ardour          # Generate Ardour only
pnpm daw:generate:ardour:install  # Generate and install to Ardour
pnpm daw:list                     # List generated DAW files
```

#### Workflow Management
```bash
pnpm workflow:complete            # Run complete workflow (extract → validate → generate)
pnpm workflow:health              # System health check across all phases
```

### Typical Development Session

1. **Start with Health Check**
   ```bash
   pnpm workflow:health
   ```

2. **Extract Plugin Data** (if working with new plugins)
   ```bash
   pnpm plugins:extract
   pnpm plugins:health
   ```

3. **Develop/Modify Mappings**
   ```bash
   # Edit YAML files in maps/ directory
   pnpm maps:validate
   pnpm maps:check
   ```

4. **Generate and Test DAW Output**
   ```bash
   pnpm daw:generate:ardour
   # Test in your DAW
   ```

5. **Run Complete Workflow**
   ```bash
   pnpm workflow:complete
   ```

## Code Quality Standards

### TypeScript Requirements

- **Strict Mode**: All TypeScript must compile in strict mode
- **Interface-First Design**: Define contracts across module boundaries
- **Dependency Injection**: Constructor injection with interface types
- **No `any` Types**: Use `unknown` with proper type guards
- **Import Pattern**: Always use `@/` imports for internal modules

```typescript
// ✅ GOOD: Interface-first design with dependency injection
export interface PluginExtractorOptions {
  scanner?: PluginScanner;
  validator?: ParameterValidator;
}

export class PluginExtractor {
  constructor(private options: PluginExtractorOptions = {}) {
    this.scanner = options.scanner ?? new PluginScanner();
    this.validator = options.validator ?? new ParameterValidator();
  }
}

// ❌ BAD: Concrete dependencies, any types
export class PluginExtractor {
  private scanner: any;
  constructor() {
    this.scanner = new ConcretePluginScanner();
  }
}
```

### Error Handling

- **No Fallbacks**: Throw descriptive errors instead of using fallbacks or mock data
- **Context in Errors**: Include operation context in error messages
- **Early Validation**: Validate inputs at function/method boundaries

```typescript
// ✅ GOOD: Descriptive error with context
if (!pluginDescriptor) {
  throw new Error(`Plugin descriptor not found for ${pluginName}. Run 'pnpm plugins:extract' first.`);
}

// ❌ BAD: Fallback with mock data
const pluginDescriptor = loadDescriptor(pluginName) || {
  parameters: [] // Mock data - bug factory!
};
```

### File Organization

- **File Size Limit**: Keep files under 300-500 lines
- **Module Boundaries**: Clear separation between plugins/, maps/, daw/, workflow/
- **Shared Types**: Common interfaces in tools/types/
- **Single Responsibility**: One primary concern per file

### Performance Requirements

- **Script Startup**: < 50ms for individual commands
- **Validation Overhead**: < 10ms for dependency checking
- **End-to-End Workflow**: < 2s for complete workflow
- **Memory Usage**: < 50MB for typical operations

## Testing Requirements

### Test Structure

```typescript
// ✅ GOOD: Comprehensive test with mocks
describe('PluginExtractor', () => {
  let mockScanner: jest.Mocked<PluginScanner>;
  let extractor: PluginExtractor;

  beforeEach(() => {
    mockScanner = {
      scanPlugins: jest.fn(),
      validatePlugin: jest.fn(),
    } as jest.Mocked<PluginScanner>;

    extractor = new PluginExtractor({
      scanner: mockScanner
    });
  });

  it('should extract plugin parameters successfully', async () => {
    mockScanner.scanPlugins.mockResolvedValue(mockPluginData);

    const result = await extractor.extract();

    expect(result.success).toBe(true);
    expect(result.plugins).toHaveLength(1);
  });

  it('should handle extraction errors gracefully', async () => {
    mockScanner.scanPlugins.mockRejectedValue(new Error('Plugin not found'));

    await expect(extractor.extract()).rejects.toThrow('Plugin not found');
  });
});
```

### Test Requirements

- **Unit Tests**: All public functions and methods
- **Integration Tests**: Cross-module interactions
- **Performance Tests**: Validate timing requirements
- **Error Path Testing**: Test all failure modes
- **80% Coverage**: Minimum test coverage target

### Running Tests

```bash
# All tests
pnpm test

# Tools-specific tests
pnpm test:tools

# Watch mode during development
pnpm test:watch

# Coverage report
pnpm test:coverage
```

## Contributing Guidelines

### Adding New Scripts or Tools

1. **Follow the Phase Structure**
   - Phase 1: Plugin interrogation (`tools/plugins/`)
   - Phase 2: Canonical mapping (`tools/maps/`)
   - Phase 3: DAW generation (`tools/daw/`)
   - Workflow: Orchestration (`tools/workflow/`)

2. **Create TypeScript Interfaces**
   ```typescript
   // In tools/types/
   export interface NewToolOptions {
     input: string;
     output?: string;
     validate?: boolean;
   }

   export interface NewToolResult {
     success: boolean;
     data?: any;
     errors: string[];
   }
   ```

3. **Implement with Dependency Injection**
   ```typescript
   // tools/phase/new-tool.ts
   import type { NewToolOptions, NewToolResult } from '@/types/new-tool';

   export class NewTool {
     constructor(private options: NewToolOptions) {}

     async execute(): Promise<NewToolResult> {
       // Implementation
     }
   }
   ```

4. **Add Script Entry Point**
   ```typescript
   // CLI entry point
   #!/usr/bin/env tsx
   import { NewTool } from './new-tool.js';
   import { parseArgs } from '@/utils/cli';

   const options = parseArgs(process.argv);
   const tool = new NewTool(options);
   const result = await tool.execute();

   if (!result.success) {
     console.error(result.errors.join('\n'));
     process.exit(1);
   }
   ```

5. **Update package.json**
   ```json
   {
     "scripts": {
       "phase:new-tool": "tsx tools/phase/new-tool.ts"
     }
   }
   ```

### Adding New MIDI Mappings

1. **Create Controller Directory Structure**
   ```
   maps/
   └── manufacturer-controller-model/
       └── plugin-category/
           └── specific-plugin.yaml
   ```

2. **Follow Canonical Format**
   ```yaml
   version: 1.0.0
   device:
     manufacturer: ControllerMfg
     model: Controller Model

   plugin:
     manufacturer: PluginMfg
     name: Plugin Name
     descriptor: plugin-descriptors/manufacturer-plugin.json

   controls:
     - id: control_1
       name: Parameter Name
       type: encoder|slider|button
       cc: 1-127
       channel: global|1-16
       plugin_parameter: 42  # Real parameter index from descriptor
   ```

3. **Validate the Mapping**
   ```bash
   pnpm maps:validate
   pnpm maps:check
   ```

### Adding New DAW Support

1. **Create DAW-Specific Types**
   ```typescript
   // tools/types/daw.ts
   export interface NewDAWMapping {
     device: DeviceInfo;
     plugin: PluginInfo;
     bindings: NewDAWBinding[];
   }
   ```

2. **Implement Generator**
   ```typescript
   // tools/daw/generators/new-daw.ts
   export class NewDAWGenerator {
     generate(canonicalMap: CanonicalMapping): NewDAWMapping {
       // Convert canonical to DAW-specific format
     }
   }
   ```

3. **Add to Main Generator**
   ```typescript
   // tools/daw/generate.ts
   if (options.target === 'new-daw' || options.target === 'all') {
     const generator = new NewDAWGenerator();
     // Generate and save
   }
   ```

## Pull Request Process

### Before Submitting

1. **Code Quality Checks**
   ```bash
   # TypeScript compilation
   pnpm typecheck

   # Linting
   pnpm lint

   # Formatting
   pnpm format:check

   # All tests pass
   pnpm test

   # Workflow health check
   pnpm workflow:health
   ```

2. **Performance Validation**
   ```bash
   # Verify performance targets
   pnpm test:coverage

   # Manual timing tests
   time pnpm plugins:extract
   time pnpm workflow:complete
   ```

### PR Requirements

- [ ] **All tests pass** with 80%+ coverage
- [ ] **TypeScript compiles** without errors
- [ ] **Performance targets met** (< 50ms startup, < 2s workflow)
- [ ] **Documentation updated** for new features
- [ ] **Error handling tested** for all failure modes
- [ ] **Integration tested** with existing workflow

### PR Description Template

```markdown
## Summary
Brief description of changes and motivation.

## Changes Made
- [ ] New feature: X
- [ ] Bug fix: Y
- [ ] Performance improvement: Z

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests pass
- [ ] Performance benchmarks meet targets

## Documentation
- [ ] README updated (if applicable)
- [ ] API documentation updated
- [ ] Breaking changes documented

## Workflow Impact
- [ ] All 12 scripts still functional
- [ ] End-to-end workflow tested
- [ ] No regression in performance
```

## Performance Benchmarking Guidelines

### Measurement Standards

1. **Script Startup Time**
   ```bash
   # Should be < 50ms
   time pnpm plugins:list
   time pnpm maps:list
   time pnpm daw:list
   ```

2. **Validation Overhead**
   ```bash
   # Should be < 10ms additional overhead
   time pnpm maps:check
   time pnpm plugins:health
   ```

3. **End-to-End Workflow**
   ```bash
   # Should be < 2s total
   time pnpm workflow:complete
   ```

4. **Memory Usage**
   ```bash
   # Monitor with activity monitor or htop
   # Should stay < 50MB for typical operations
   ```

### Performance Testing

```typescript
// Example performance test
describe('Performance Tests', () => {
  it('should start script in < 50ms', async () => {
    const start = performance.now();
    await import('./plugin-extractor.js');
    const end = performance.now();

    expect(end - start).toBeLessThan(50);
  });

  it('should complete workflow in < 2000ms', async () => {
    const start = performance.now();
    await runCompleteWorkflow();
    const end = performance.now();

    expect(end - start).toBeLessThan(2000);
  });
});
```

### Benchmarking Tools

- **Vitest**: For automated performance tests
- **Node.js built-in**: `performance.now()` for timing
- **System tools**: `time` command for script execution
- **Memory profiling**: Node.js `--inspect` for memory analysis

## Common Pitfalls

### ❌ Things to Avoid

1. **Relative Imports**
   ```typescript
   // BAD
   import { Plugin } from '../types/plugin';

   // GOOD
   import { Plugin } from '@/types/plugin';
   ```

2. **Fallback Data**
   ```typescript
   // BAD
   const params = getParameters() || [];

   // GOOD
   const params = getParameters();
   if (!params) {
     throw new Error('Parameters not found. Run extraction first.');
   }
   ```

3. **Large Files**
   ```typescript
   // If your file is > 500 lines, break it into smaller modules
   // Use composition instead of large monolithic files
   ```

4. **Module Stubbing**
   ```typescript
   // BAD
   jest.mock('fs');

   // GOOD
   const mockFileSystem: FileSystem = {
     readFile: jest.fn(),
     writeFile: jest.fn()
   };
   ```

### ✅ Best Practices

1. **Interface-First Design**
   ```typescript
   // Define contracts first, implement later
   export interface MIDIController {
     extractControls(): Promise<ControlMap>;
     validateMapping(map: CanonicalMapping): ValidationResult;
   }
   ```

2. **Clear Error Messages**
   ```typescript
   throw new Error(
     `MIDI mapping validation failed for ${device.name}: ` +
     `Parameter ${paramIndex} not found in plugin descriptor. ` +
     `Available parameters: ${availableParams.join(', ')}`
   );
   ```

3. **Performance Monitoring**
   ```typescript
   const start = performance.now();
   await extractPlugins();
   const duration = performance.now() - start;
   console.log(`Plugin extraction completed in ${duration.toFixed(2)}ms`);
   ```

## Getting Help

### Resources

- **Documentation**: Check `/docs` directory for comprehensive guides
- **Examples**: Look at existing implementations in `tools/` directories
- **Test Cases**: Review test files for usage patterns
- **Architecture**: Read `ARCHITECTURE.md` for system design

### Communication

- **Issues**: GitHub issues for bugs and feature requests
- **Discussions**: GitHub discussions for general questions
- **Code Review**: PRs for collaborative code improvement

### Debugging

1. **Enable Debug Logging**
   ```bash
   DEBUG=audio-control:* pnpm workflow:complete
   ```

2. **TypeScript Diagnostics**
   ```bash
   pnpm typecheck --listFiles
   ```

3. **Performance Profiling**
   ```bash
   node --inspect tools/workflow/complete.ts
   ```

## Project-Specific Guidelines

### MIDI/Audio Domain Knowledge

- **MIDI CC Range**: Controllers use CC 0-127
- **Parameter Indexing**: Plugin parameters use 0-based indexing
- **Real-time Constraints**: Audio operations must be low-latency
- **DAW Integration**: Each DAW has specific format requirements

### Audio Plugin Considerations

- **VST3 Format**: Primary target for plugin interrogation
- **Parameter Types**: Continuous, discrete, boolean, choice
- **Automation Flags**: Not all parameters are automatable
- **Plugin Loading**: Avoid loading plugins during mapping creation

### Hardware Controller Patterns

- **Physical Layout**: Map controls to logical parameter groups
- **CC Assignment**: Avoid conflicts with standard MIDI CCs
- **Device Memory**: Consider limited MIDI controller memory
- **User Experience**: Intuitive control mapping for musicians

---

## Summary

Contributing to audio-control requires:

1. **Understanding the 12-script workflow** architecture
2. **Following TypeScript strict mode** and interface-first design
3. **Maintaining performance targets** (< 50ms startup, < 2s workflow)
4. **Writing comprehensive tests** with 80%+ coverage
5. **Using dependency injection** and avoiding fallbacks
6. **Creating clear documentation** for new features

The project prioritizes correctness, type safety, and real-time performance suitable for professional audio software. Welcome to the community!