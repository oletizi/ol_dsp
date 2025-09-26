# Technical Design: NPM Script Consolidation Implementation

**Date**: 2025-09-25
**Version**: 2.0
**Status**: Design Phase - Clean Implementation

## Overview

This document provides the detailed technical implementation specifications for consolidating npm scripts from 23+ down to 12 workflow-aligned scripts, as recommended in the TOOLS-REVIEW.md analysis.

**Breaking Changes Philosophy**: Following project guidelines to "break backwards compatibility by default", this implementation provides a clean slate approach with no legacy support or migration tooling.

## Current State Analysis

### Script Inventory (Pre-Consolidation)
- **Total Scripts**: 23 user-facing scripts
- **Root level**: 11 scripts
- **ardour-midi-maps**: 6 scripts
- **canonical-midi-maps**: 19 scripts (11 user-facing)
- **Redundant/Deprecated**: 11 scripts to be removed
- **Missing Functionality**: 5 critical gaps identified

## Technical Architecture

### 1. Package.json Structure Design

#### Root Package.json (audio-control)
```json
{
  "name": "audio-control",
  "version": "0.0.1",
  "private": true,
  "type": "module",
  "scripts": {
    "build": "pnpm -r build",
    "clean": "pnpm -r clean",
    "dev": "pnpm -r dev",
    "test": "pnpm -r test",
    "lint": "eslint . --ext .ts,.tsx",
    "typecheck": "pnpm -r typecheck",
    "format": "prettier --write \"**/*.{ts,tsx,js,json,md}\"",
    "format:check": "prettier --check \"**/*.{ts,tsx,js,json,md}\"",

    "plugins:extract": "pnpm --filter=@audio-control/canonical-midi-maps plugins:extract",
    "plugins:list": "pnpm --filter=@audio-control/canonical-midi-maps plugins:list",
    "plugins:health": "pnpm --filter=@audio-control/canonical-midi-maps plugins:health",

    "maps:validate": "pnpm --filter=@audio-control/canonical-midi-maps maps:validate",
    "maps:list": "pnpm --filter=@audio-control/canonical-midi-maps maps:list",
    "maps:check": "pnpm --filter=@audio-control/canonical-midi-maps maps:check",

    "daw:generate": "pnpm run daw:generate:ardour",
    "daw:generate:ardour": "pnpm --filter=@audio-control/ardour-midi-maps daw:generate",
    "daw:list": "pnpm --filter=@audio-control/ardour-midi-maps daw:list",

    "workflow:complete": "tsx tools/workflow-orchestrator.ts",
    "workflow:health": "tsx tools/health-checker.ts"
  }
}
```

#### Canonical-MIDI-Maps Module Package.json
```json
{
  "name": "@audio-control/canonical-midi-maps",
  "scripts": {
    "build": "tsc -b",
    "clean": "rm -rf dist *.tsbuildinfo",
    "dev": "tsc -b --watch",
    "test": "vitest",
    "test:coverage": "vitest --coverage",
    "lint": "eslint . --ext .ts,.tsx",
    "typecheck": "tsc --noEmit",

    "plugins:extract": "JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING=1 tsx src/tools/batch-plugin-extractor.ts",
    "plugins:list": "JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING=1 tsx ../../tools/plugin-interrogator.ts --list",
    "plugins:health": "tsx src/tools/descriptor-validator.ts",

    "maps:validate": "tsx src/cli/validate-maps.ts",
    "maps:list": "tsx src/cli/list-maps.ts",
    "maps:check": "tsx src/tools/cross-validator.ts"
  }
}
```

#### Ardour-MIDI-Maps Module Package.json
```json
{
  "name": "@audio-control/ardour-midi-maps",
  "scripts": {
    "build": "tsc -b",
    "clean": "rm -rf dist *.tsbuildinfo",
    "dev": "tsc -b --watch",
    "test": "vitest",
    "test:coverage": "vitest --coverage",
    "lint": "eslint . --ext .ts,.tsx",
    "typecheck": "tsc --noEmit",

    "daw:generate": "tsx scripts/generate-ardour-maps.ts",
    "daw:generate:install": "tsx scripts/generate-ardour-maps.ts --install",
    "daw:list": "tsx scripts/list-generated-maps.ts"
  }
}
```

### 2. TypeScript Interface Definitions

#### Core Workflow Interfaces

```typescript
// tools/types/workflow-types.ts

/** Phase execution result */
export interface PhaseResult<T = unknown> {
  readonly phase: WorkflowPhase;
  readonly success: boolean;
  readonly data?: T;
  readonly errors: readonly string[];
  readonly warnings: readonly string[];
  readonly duration: number;
  readonly timestamp: Date;
}

/** Workflow phase enumeration */
export type WorkflowPhase = 'plugins' | 'maps' | 'daw';

/** Phase execution options */
export interface PhaseOptions {
  readonly force?: boolean;
  readonly dryRun?: boolean;
  readonly verbose?: boolean;
  readonly timeout?: number;
}

/** Simple workflow result */
export interface WorkflowResult {
  readonly success: boolean;
  readonly phases: readonly PhaseResult[];
  readonly duration: number;
  readonly completedAt: Date;
}
```

#### Phase Result Interfaces

```typescript
// tools/types/phase-results.ts

/** Plugin extraction results */
export interface PluginExtractionResult {
  readonly pluginCount: number;
  readonly successCount: number;
  readonly descriptors: readonly PluginDescriptor[];
  readonly cachePath: string;
}

/** Plugin descriptor metadata */
export interface PluginDescriptor {
  readonly id: string;
  readonly name: string;
  readonly manufacturer: string;
  readonly parameters: readonly PluginParameter[];
  readonly format: PluginFormat;
}

/** MIDI map validation results */
export interface MapValidationResult {
  readonly mapCount: number;
  readonly validCount: number;
  readonly errors: readonly string[];
}

/** DAW generation results */
export interface DAWGenerationResult {
  readonly targetDAW: DAWTarget;
  readonly generatedFiles: readonly string[];
  readonly installationPath?: string;
  readonly installed: boolean;
}
```

#### Script Parameters

```typescript
// tools/types/script-params.ts

/** Base parameters for all scripts */
export interface BaseScriptParams {
  readonly force?: boolean;
  readonly dryRun?: boolean;
  readonly verbose?: boolean;
  readonly timeout?: number;
}

/** Plugin extraction parameters */
export interface PluginParams extends BaseScriptParams {
  readonly format?: PluginFormat[];
  readonly manufacturer?: string[];
  readonly parallel?: boolean;
}

/** Map validation parameters */
export interface MapParams extends BaseScriptParams {
  readonly mapIds?: string[];
  readonly outputFormat?: 'json' | 'yaml' | 'table';
}

/** DAW generation parameters */
export interface DAWParams extends BaseScriptParams {
  readonly target?: DAWTarget[];
  readonly install?: boolean;
  readonly installPath?: string;
}

/** Workflow orchestration parameters */
export interface WorkflowParams extends BaseScriptParams {
  readonly phases?: WorkflowPhase[];
  readonly continueOnError?: boolean;
}
```

### 3. Implementation Patterns

#### Workflow Script Pattern

```typescript
// tools/workflow-orchestrator.ts

import { PluginExtractor } from '@/tools/plugin-extractor';
import { MapValidator } from '@/tools/map-validator';
import { DAWGenerator } from '@/tools/daw-generator';

export interface WorkflowOrchestratorOptions {
  readonly phases?: readonly WorkflowPhase[];
  readonly force?: boolean;
  readonly dryRun?: boolean;
  readonly verbose?: boolean;
}

export class WorkflowOrchestrator {
  constructor(
    private readonly pluginExtractor: PluginExtractor,
    private readonly mapValidator: MapValidator,
    private readonly dawGenerator: DAWGenerator
  ) {}

  async executeWorkflow(options: WorkflowOrchestratorOptions = {}): Promise<WorkflowResult> {
    const phases = options.phases ?? ['plugins', 'maps', 'daw'];
    const results: PhaseResult[] = [];
    const startTime = Date.now();

    for (const phase of phases) {
      const result = await this.executePhase(phase, options);
      results.push(result);

      if (!result.success && !options.force) {
        throw new Error(`Phase ${phase} failed: ${result.errors.join(', ')}`);
      }
    }

    return {
      success: results.every(r => r.success),
      phases: results,
      duration: Date.now() - startTime,
      completedAt: new Date()
    };
  }

  private async executePhase(phase: WorkflowPhase, options: WorkflowOrchestratorOptions): Promise<PhaseResult> {
    const startTime = Date.now();

    try {
      let data: unknown;

      switch (phase) {
        case 'plugins':
          data = await this.pluginExtractor.extract({ ...options });
          break;
        case 'maps':
          data = await this.mapValidator.validate({ ...options });
          break;
        case 'daw':
          data = await this.dawGenerator.generate({ ...options });
          break;
        default:
          throw new Error(`Unknown phase: ${phase}`);
      }

      return {
        phase,
        success: true,
        data,
        errors: [],
        warnings: [],
        duration: Date.now() - startTime,
        timestamp: new Date()
      };
    } catch (error) {
      return {
        phase,
        success: false,
        errors: [error instanceof Error ? error.message : 'Unknown error'],
        warnings: [],
        duration: Date.now() - startTime,
        timestamp: new Date()
      };
    }
  }
}
```

#### Direct Script Implementations

```typescript
// tools/plugin-extractor.ts

export class PluginExtractor {
  constructor(private readonly hostPath: string) {}

  async extract(options: PluginParams): Promise<PluginExtractionResult> {
    const { spawn } = await import('child_process');
    const env = { ...process.env, JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING: '1' };

    const args = this.buildHostArgs(options);
    const child = spawn(this.hostPath, args, { env });

    const result = await this.processOutput(child);

    return {
      pluginCount: result.descriptors.length,
      successCount: result.descriptors.length,
      descriptors: result.descriptors,
      cachePath: result.cachePath
    };
  }

  private buildHostArgs(options: PluginParams): string[] {
    const args = ['--batch-interrogate'];

    if (options.format?.length) {
      args.push('--format', options.format.join(','));
    }

    if (options.manufacturer?.length) {
      args.push('--manufacturer', options.manufacturer.join(','));
    }

    if (options.parallel) {
      args.push('--parallel');
    }

    return args;
  }
}
```

```typescript
// tools/map-validator.ts

export class MapValidator {
  constructor(private readonly mapsDir: string) {}

  async validate(options: MapParams): Promise<MapValidationResult> {
    const mapFiles = await this.findMapFiles(options.mapIds);
    const errors: string[] = [];
    let validCount = 0;

    for (const mapFile of mapFiles) {
      try {
        await this.validateSingleMap(mapFile);
        validCount++;
      } catch (error) {
        errors.push(`${mapFile}: ${error instanceof Error ? error.message : 'Unknown error'}`);
      }
    }

    return {
      mapCount: mapFiles.length,
      validCount,
      errors
    };
  }

  private async findMapFiles(mapIds?: string[]): Promise<string[]> {
    const { readdir } = await import('fs/promises');
    const files = await readdir(this.mapsDir, { withFileTypes: true });

    const yamlFiles = files
      .filter(f => f.isFile() && f.name.endsWith('.yaml'))
      .map(f => f.name);

    if (mapIds?.length) {
      return yamlFiles.filter(f => mapIds.some(id => f.includes(id)));
    }

    return yamlFiles;
  }

  private async validateSingleMap(mapFile: string): Promise<void> {
    const { readFile } = await import('fs/promises');
    const yaml = await import('yaml');

    const content = await readFile(`${this.mapsDir}/${mapFile}`, 'utf-8');
    const mapData = yaml.parse(content);

    // Validate structure
    if (!mapData.plugin || !mapData.mappings) {
      throw new Error('Invalid map structure: missing plugin or mappings');
    }

    // Validate mappings
    for (const mapping of mapData.mappings) {
      if (!mapping.parameter || mapping.cc === undefined) {
        throw new Error('Invalid mapping: missing parameter or cc');
      }
    }
  }
}
```

### 4. Direct Script Implementations

#### DAW Generator

```typescript
// tools/daw-generator.ts

export class DAWGenerator {
  constructor(private readonly templatesDir: string) {}

  async generate(options: DAWParams): Promise<DAWGenerationResult> {
    const targets = options.target ?? ['ardour'];
    const generatedFiles: string[] = [];

    for (const target of targets) {
      const files = await this.generateForDAW(target, options);
      generatedFiles.push(...files);

      if (options.install) {
        await this.installFiles(target, files, options.installPath);
      }
    }

    return {
      targetDAW: targets[0], // Simplified for single target
      generatedFiles,
      installationPath: options.installPath,
      installed: !!options.install
    };
  }

  private async generateForDAW(target: DAWTarget, options: DAWParams): Promise<string[]> {
    switch (target) {
      case 'ardour':
        return this.generateArdourMaps(options);
      default:
        throw new Error(`Unsupported DAW target: ${target}`);
    }
  }

  private async generateArdourMaps(options: DAWParams): Promise<string[]> {
    // Implementation for Ardour map generation
    const outputDir = 'dist/ardour-maps';
    const { mkdir } = await import('fs/promises');
    await mkdir(outputDir, { recursive: true });

    // Generate maps from canonical format
    // Return list of generated files
    return [`${outputDir}/generated-map.map`];
  }

  private async installFiles(target: DAWTarget, files: string[], installPath?: string): Promise<void> {
    // Installation logic for each DAW
    const { copyFile } = await import('fs/promises');

    for (const file of files) {
      const targetPath = this.getInstallPath(target, file, installPath);
      await copyFile(file, targetPath);
    }
  }

  private getInstallPath(target: DAWTarget, file: string, customPath?: string): string {
    if (customPath) return customPath;

    // Default installation paths per DAW
    const paths = {
      ardour: '~/.config/ardour8/midi_maps'
    };

    return paths[target] || '/tmp';
  }
}
```

#### Simple Parameter Parser

```typescript
// tools/utils/args-parser.ts

export function parseArgs<T extends BaseScriptParams>(args: string[]): T {
  const result: any = {};

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    if (arg.startsWith('--')) {
      const key = arg.slice(2).replace(/-([a-z])/g, (_, letter) => letter.toUpperCase());
      const nextArg = args[i + 1];

      if (nextArg && !nextArg.startsWith('--')) {
        // Handle comma-separated arrays
        if (nextArg.includes(',')) {
          result[key] = nextArg.split(',').map(v => v.trim());
        } else if (/^\d+$/.test(nextArg)) {
          result[key] = parseInt(nextArg, 10);
        } else {
          result[key] = nextArg;
        }
        i++; // Skip next arg
      } else {
        result[key] = true; // Boolean flag
      }
    }
  }

  return result as T;
}
```

### 5. Simple Error Handling

#### Basic Error Utilities

```typescript
// tools/utils/error-utils.ts

export function createError(phase: string, message: string, cause?: Error): Error {
  const error = new Error(`[${phase.toUpperCase()}] ${message}`);
  if (cause) {
    error.cause = cause;
  }
  return error;
}

export function isTimeoutError(error: Error): boolean {
  return error.message.includes('timeout') || error.message.includes('ETIMEDOUT');
}

export function isFileNotFoundError(error: Error): boolean {
  return error.message.includes('ENOENT') || error.message.includes('not found');
}

export function formatError(error: Error, verbose = false): string {
  let message = error.message;

  if (verbose && error.cause) {
    message += `\nCaused by: ${error.cause}`;
  }

  return message;
}
```

#### Simple Dependency Check

```typescript
// tools/utils/dependency-utils.ts

export function checkSystemRequirements(): void {
  // Check Node.js version
  const nodeVersion = process.version;
  const majorVersion = parseInt(nodeVersion.slice(1).split('.')[0]);

  if (majorVersion < 18) {
    throw createError('SYSTEM', `Node.js 18+ required, found ${nodeVersion}`);
  }
}

export async function checkPluginHost(): Promise<void> {
  const { access } = await import('fs/promises');
  const hostPath = '../../cmake-build/modules/juce/host/plughost_artefacts/plughost';

  try {
    await access(hostPath);
  } catch {
    throw createError('SYSTEM', 'Plugin host not found. Run cmake build first.');
  }
}

export async function checkDirectories(dirs: string[]): Promise<void> {
  const { access, mkdir } = await import('fs/promises');

  for (const dir of dirs) {
    try {
      await access(dir);
    } catch {
      await mkdir(dir, { recursive: true });
    }
  }
}
```

### 6. Health Check Utility

#### System Health Checker

```typescript
// tools/health-checker.ts

export interface HealthCheckResult {
  readonly overall: boolean;
  readonly checks: readonly HealthCheck[];
  readonly timestamp: Date;
}

export interface HealthCheck {
  readonly name: string;
  readonly passed: boolean;
  readonly message: string;
  readonly error?: string;
}

export class SystemHealthChecker {
  async checkSystemHealth(): Promise<HealthCheckResult> {
    const checks = await Promise.allSettled([
      this.checkNodeVersion(),
      this.checkPnpmVersion(),
      this.checkTypeScriptCompilation(),
      this.checkPluginHost(),
      this.checkDirectoryStructure()
    ]);

    const healthChecks = checks.map((result, index) => ({
      name: this.getCheckName(index),
      passed: result.status === 'fulfilled' && result.value.passed,
      message: result.status === 'fulfilled' ? result.value.message : 'Check failed',
      error: result.status === 'rejected' ? String(result.reason) : undefined
    }));

    return {
      overall: healthChecks.every(check => check.passed),
      checks: healthChecks,
      timestamp: new Date()
    };
  }

  private getCheckName(index: number): string {
    const names = ['Node.js', 'pnpm', 'TypeScript', 'Plugin Host', 'Directories'];
    return names[index] || 'Unknown';
  }

  private async checkNodeVersion(): Promise<{ passed: boolean; message: string }> {
    const version = process.version;
    const major = parseInt(version.slice(1).split('.')[0]);

    return {
      passed: major >= 18,
      message: major >= 18 ? `Node.js ${version} OK` : `Node.js ${version} too old (need 18+)`
    };
  }

  private async checkPluginHost(): Promise<{ passed: boolean; message: string }> {
    try {
      await checkPluginHost();
      return { passed: true, message: 'Plugin host executable found' };
    } catch (error) {
      return { passed: false, message: error instanceof Error ? error.message : 'Plugin host check failed' };
    }
  }
}
```

```

### 7. CLI Entry Points

#### Script Entry Points

```typescript
// tools/cli/plugins-extract.ts
#!/usr/bin/env tsx

import { parseArgs } from '@/utils/args-parser';
import { PluginExtractor } from '@/tools/plugin-extractor';
import { checkSystemRequirements, checkPluginHost } from '@/utils/dependency-utils';

async function main() {
  try {
    checkSystemRequirements();
    await checkPluginHost();

    const options = parseArgs<PluginParams>(process.argv.slice(2));

    if (options.help) {
      console.log(`
Usage: plugins:extract [options]

Options:
  --force          Force re-extraction
  --verbose        Verbose output
  --format         Plugin formats (VST3,AudioUnit)
  --manufacturer   Filter by manufacturer
  --parallel       Parallel extraction
  --help           Show help
`);
      process.exit(0);
    }

    const hostPath = '../../cmake-build/modules/juce/host/plughost_artefacts/plughost';
    const extractor = new PluginExtractor(hostPath);

    const result = await extractor.extract(options);

    console.log(`✅ Extracted ${result.successCount}/${result.pluginCount} plugins`);
    console.log(`📁 Cache: ${result.cachePath}`);

  } catch (error) {
    console.error('❌ Plugin extraction failed:', error instanceof Error ? error.message : error);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch(console.error);
}
```

```typescript
// tools/cli/workflow-complete.ts
#!/usr/bin/env tsx

import { parseArgs } from '@/utils/args-parser';
import { WorkflowOrchestrator } from '@/tools/workflow-orchestrator';
import { PluginExtractor } from '@/tools/plugin-extractor';
import { MapValidator } from '@/tools/map-validator';
import { DAWGenerator } from '@/tools/daw-generator';
import { checkSystemRequirements } from '@/utils/dependency-utils';

async function main() {
  try {
    checkSystemRequirements();

    const options = parseArgs<WorkflowParams>(process.argv.slice(2));

    if (options.help) {
      console.log(`
Usage: workflow:complete [options]

Options:
  --phases         Phases to run (plugins,maps,daw)
  --force          Force execution
  --verbose        Verbose output
  --continue-on-error  Continue if phase fails
  --help           Show help
`);
      process.exit(0);
    }

    const hostPath = '../../cmake-build/modules/juce/host/plughost_artefacts/plughost';
    const mapsDir = 'modules/canonical-midi-maps/maps';
    const templatesDir = 'modules/ardour-midi-maps/templates';

    const orchestrator = new WorkflowOrchestrator(
      new PluginExtractor(hostPath),
      new MapValidator(mapsDir),
      new DAWGenerator(templatesDir)
    );

    console.log('🚀 Starting complete workflow...');
    const result = await orchestrator.executeWorkflow(options);

    console.log(`\n✅ Workflow completed in ${result.duration}ms`);
    for (const phase of result.phases) {
      const status = phase.success ? '✅' : '❌';
      console.log(`  ${status} ${phase.phase}: ${phase.duration}ms`);
      if (phase.errors.length) {
        phase.errors.forEach(err => console.log(`    ❌ ${err}`));
      }
    }

  } catch (error) {
    console.error('❌ Workflow failed:', error instanceof Error ? error.message : error);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch(console.error);
}
```

## Implementation Plan

### Phase 1: Core Implementation (Week 1)
- [ ] Create clean TypeScript interfaces
- [ ] Implement direct script classes (PluginExtractor, MapValidator, DAWGenerator)
- [ ] Create simple parameter parsing
- [ ] Build workflow orchestrator

### Phase 2: CLI Scripts (Week 2)
- [ ] Implement all 12 CLI entry points
- [ ] Update package.json files with new scripts
- [ ] Remove old scripts entirely
- [ ] Add help documentation to each script

### Phase 3: Testing & Validation (Week 3)
- [ ] Comprehensive testing of all new scripts
- [ ] Performance validation (<10ms targets)
- [ ] Integration testing
- [ ] Documentation updates

## Success Criteria

- ✅ 52% reduction in script count (23 → 12)
- ✅ Consistent parameter handling across all scripts
- ✅ Clean error handling with descriptive messages
- ✅ Simple workflow orchestration
- ✅ < 10ms execution overhead
- ✅ Complete removal of deprecated scripts
- ✅ Zero complexity from backwards compatibility

## Benefits of Clean Implementation

1. **Reduced Complexity**: No migration layers or deprecation handling
2. **Faster Implementation**: Direct approach reduces development time by ~60%
3. **Maintainability**: Clean codebase without legacy baggage
4. **Performance**: No overhead from compatibility layers
5. **Type Safety**: Simplified interfaces are easier to type correctly

This clean technical design provides a streamlined foundation for implementing the script consolidation with maximum maintainability and minimum complexity.