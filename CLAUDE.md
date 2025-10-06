# Claude AI Agent Guidelines for OL_DSP

This document provides guidelines for AI agents (including Claude Code) working on the ol_dsp project. It ensures consistency with the project's architectural principles, quality standards, and development patterns.

## Overview

OL_DSP is an eclectic digital signal processing project focused on:

- C/C++ DSP code for Arduino and microcontrollers
- JavaScript/TypeScript code for hardware sampler interaction
- MIDI communication protocols
- Proprietary sampler data formats (Akai S1000, S5000/S6000)
- Audio effects processing (chorus, pitch shift, etc.)
- RNBO and JUCE-based plugin development
- Real-time audio processing algorithms

## Core Requirements

### Import Pattern

- **ALWAYS use the `@/` import pattern** for internal modules
- Pattern is already configured in `tsconfig.json` with path mapping
- Examples:
  ```typescript
  import { ContentScanner } from '@/utils/scanner';
  import { ContentEntry } from '@/types/content';
  import { AIScorer } from '@/utils/ai';
  ```

### Error Handling

- **Never implement fallbacks or use mock data outside of test code**
- **Throw errors with descriptive messages** instead of fallbacks
- Errors let us know that something isn't implemented
- Fallbacks and mock data are bug factories

```typescript
// ✅ GOOD: Throw descriptive errors
if (!apiKey) {
  throw new Error('API key is required but not configured');
}

// ❌ BAD: Using fallbacks
const apiKey = process.env.API_KEY || 'mock-key-for-testing';
```

### Code Quality Standards

- **TypeScript strict mode required** (already enabled)
- **High code coverage** - aim for 80%+ coverage
- **Unit tests must be deterministic** - use mocking/dependency injection
- **NO module stubbing** - use dependency injection instead
- All code must be unit testable

### File Size Limits

- **Code files should be no larger than 300-500 lines**
- Anything larger should be refactored for readability and modularity
- Split large files into smaller, focused modules

### Repository Hygiene

- **Build artifacts ONLY in `dist/` directory** (already configured)
- NO temporary scripts, logs, or generated files committed to git
- **Never bypass pre-commit or pre-push hooks** - fix issues instead
- Clean repository is mandatory

## Implementation Patterns

### Dependency Injection Pattern

```typescript
// Good: Constructor injection with interfaces
export interface ContentProcessorOptions {
  aiScorer?: AIScorer;
  database?: ContentDatabaseHandler;
  scanner?: ContentScanner;
}

export class ContentProcessor {
  private readonly aiScorer: AIScorer;
  private readonly database: ContentDatabaseHandler;

  constructor(options: ContentProcessorOptions = {}) {
    this.aiScorer = options.aiScorer ?? new AIScorer();
    this.database = options.database ?? new ContentDatabaseHandler();
  }
}

// Provide factory for backward compatibility
export function createContentProcessor(options?: ContentProcessorOptions): ContentProcessor {
  return new ContentProcessor(options);
}
```

### Test Structure (Critical for Coverage)

```typescript
import { ContentProcessor } from '@/services/processor';
import { AIScorer } from '@/utils/ai';

describe('ContentProcessor', () => {
  let mockAIScorer: jest.Mocked<AIScorer>;
  let mockDatabase: jest.Mocked<ContentDatabaseHandler>;
  let processor: ContentProcessor;

  beforeEach(() => {
    mockAIScorer = {
      scoreContent: jest.fn(),
      improveContent: jest.fn(),
    } as jest.Mocked<AIScorer>;

    mockDatabase = {
      load: jest.fn(),
      updateEntry: jest.fn(),
      getData: jest.fn(),
    } as jest.Mocked<ContentDatabaseHandler>;

    processor = new ContentProcessor({
      aiScorer: mockAIScorer,
      database: mockDatabase,
    });
  });

  it('should handle both success and error cases', async () => {
    // Test both happy path and error conditions
    mockAIScorer.scoreContent.mockResolvedValue(mockAnalysis);
    
    await expect(processor.processContent('test.md')).resolves.not.toThrow();
    
    // Test error case
    mockAIScorer.scoreContent.mockRejectedValue(new Error('API failure'));
    await expect(processor.processContent('test.md')).rejects.toThrow('API failure');
  });
});
```

## Project Structure

The project contains multiple components:

```text
ol_dsp/
├── arduino/               # Arduino/microcontroller DSP code
├── effects/              # Audio effect implementations
│   ├── rnbo/            # RNBO-based effects
│   └── juce/            # JUCE plugin host and effects
├── samplers/            # Hardware sampler interaction code
│   ├── akai/            # Akai S1000/S5000/S6000 format handlers
│   └── midi/            # MIDI communication utilities
├── dsp/                 # Core DSP algorithms
├── gen~/                # Max/MSP gen~ patches
└── scripts/             # Build and utility scripts
```

## DSP-Specific Guidelines

### Memory Management
- **Microcontroller constraints**: Be mindful of limited RAM/flash on Arduino
- **Real-time safety**: Avoid dynamic allocation in audio callbacks
- **Buffer management**: Use fixed-size buffers for DSP operations

### Audio Processing
- **Sample rate awareness**: Always consider target sample rates (44.1kHz, 48kHz, etc.)
- **Bit depth**: Handle 16-bit, 24-bit, and float formats appropriately
- **Latency considerations**: Minimize processing delays for real-time applications

### Hardware Integration
- **MIDI timing**: Respect MIDI clock and timing constraints
- **Sampler formats**: Preserve proprietary format specifications exactly
- **Endianness**: Be aware of byte order when dealing with binary formats

## Development Workflow for AI Agents

### Before Making Changes

1. **Read existing code** to understand patterns and conventions
2. **Check dependencies** in package.json to understand available libraries
3. **Review test files** to understand testing patterns
4. **Verify imports use `@/` pattern**

### When Writing Code

1. **Use dependency injection** - pass dependencies via constructor
2. **Follow `@/` import pattern** for all internal imports
3. **Write tests first** or alongside implementation
4. **Ensure high coverage** - test all error paths
5. **Use descriptive error messages** with context
6. **Throw errors instead of fallbacks** outside of test code
7. **Keep files under 300-500 lines** - refactor if larger

### Before Completing Tasks

1. **Run tests**: `npm test`
2. **Check build**: `npm run build`
3. **Verify TypeScript compilation**: `tsc --noEmit`
4. **Ensure all imports use `@/` pattern**

## Common Commands

```bash
# Build commands (vary by component)
make                # Build C/C++ components
npm run build       # Build TypeScript/JavaScript components
arduino-cli compile # Compile Arduino sketches

# Testing
npm test            # Run JavaScript/TypeScript tests
make test           # Run C/C++ tests

# JUCE plugin operations
./juce_host --list  # List available plugins
./juce_host --load  # Load and test plugins
```

## Error Handling Pattern

```typescript
try {
  const result = await someOperation();
  return result;
} catch (error: any) {
  const contextualMessage = `Failed to ${operation} for ${resource}: ${error.message}`;
  console.error(contextualMessage);
  throw new Error(contextualMessage);
}
```

## Critical Don'ts for AI Agents

❌ **NEVER implement fallbacks or mock data** outside of test code - throw descriptive errors instead  
❌ **NEVER stub entire modules** (`jest.mock('fs')`) - use dependency injection  
❌ **NEVER put build artifacts** outside appropriate directories  
❌ **NEVER bypass pre-commit/pre-push checks** - fix issues instead  
❌ **NEVER use relative imports** - use `@/` pattern for TypeScript modules  
❌ **NEVER write environment-dependent tests**  
❌ **NEVER commit temporary files** or scripts  
❌ **NEVER create files larger than 500 lines** - refactor for modularity  
❌ **NEVER use dynamic memory allocation** in real-time audio callbacks  
❌ **NEVER ignore sample rate or bit depth** when processing audio  
❌ **NEVER modify proprietary format specs** without documentation  

## Success Criteria

An AI agent has successfully completed work when:

- ✅ All tests pass (C/C++, TypeScript/JavaScript)
- ✅ Code follows dependency injection patterns (where applicable)
- ✅ TypeScript imports use `@/` pattern
- ✅ Build artifacts in appropriate directories
- ✅ Pre-commit/pre-push hooks pass
- ✅ Compilation succeeds (TypeScript, C/C++, Arduino)
- ✅ No fallbacks or mock data outside test code
- ✅ Files are appropriately sized (under 500 lines)
- ✅ Descriptive error messages for missing functionality
- ✅ Real-time audio code is allocation-free
- ✅ Sample rates and bit depths handled correctly
- ✅ MIDI timing constraints respected
- ✅ Memory usage appropriate for target platform

## When in Doubt

- Look at existing code in the project for patterns
- Check test files for testing approaches
- Follow the dependency injection pattern consistently
- Use `@/` imports for all internal modules
- Prioritize testability over convenience
- Throw errors with context instead of using fallbacks
- don't mock/stub modules.
- don't mock/stub modules; use dependency injection and a mock interface instead.
- always document make targets in the Makefile itself to ensure it's self documenting
- always use make targets instead of running ./scripts/* directly to ensure the make targets work properly. There should only be a single path to achieve a goal.
- Don't mention "production-readiness" in documentation. It's almost certainly not true until proven by actual production testing.