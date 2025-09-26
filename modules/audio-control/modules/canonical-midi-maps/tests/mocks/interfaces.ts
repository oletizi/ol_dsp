/**
 * Mock interfaces for dependency injection testing
 * These interfaces mirror the real implementation interfaces for testing
 */

import { vi } from 'vitest';
import type {
  PluginInfo,
  ProcessResult,
  GeneratorArgs,
  PluginParameter,
  ProcessedParameter,
  PluginSpec
} from '@/tools/plugin-generator/types.js';

/**
 * Mock interface for process manager
 */
export interface MockProcessManager {
  runPlughost: (args: string[], timeoutMs?: number) => Promise<ProcessResult>;
  isPlughostAvailable: () => boolean;
}

/**
 * Mock interface for plugin discovery service
 */
export interface MockPluginDiscovery {
  getPluginList: () => Promise<PluginInfo[]>;
  shouldSkipPlugin: (plugin: PluginInfo) => boolean;
}

/**
 * Mock interface for parameter categorizer
 */
export interface MockParameterCategorizer {
  categorizeParameters: (parameters: PluginParameter[]) => Record<string, ProcessedParameter[]>;
  processParameter: (param: PluginParameter) => ProcessedParameter;
}

/**
 * Mock interface for spec generator
 */
export interface MockSpecGenerator {
  generatePluginSpec: (plugin: PluginInfo) => Promise<PluginSpec | null>;
  savePluginSpec: (plugin: PluginInfo, spec: PluginSpec, outputDir: string) => string;
}

/**
 * Mock interface for CLI argument parser
 */
export interface MockCli {
  parseArgs: () => GeneratorArgs;
  showHelp: (outputDir: string) => void;
}

/**
 * Factory function to create mock implementations
 */
export function createMockComponents() {
  const mockProcessManager: MockProcessManager = {
    runPlughost: vi.fn(),
    isPlughostAvailable: vi.fn(),
  };

  const mockPluginDiscovery: MockPluginDiscovery = {
    getPluginList: vi.fn(),
    shouldSkipPlugin: vi.fn(),
  };

  const mockParameterCategorizer: MockParameterCategorizer = {
    categorizeParameters: vi.fn(),
    processParameter: vi.fn(),
  };

  const mockSpecGenerator: MockSpecGenerator = {
    generatePluginSpec: vi.fn(),
    savePluginSpec: vi.fn(),
  };

  const mockCli: MockCli = {
    parseArgs: vi.fn(),
    showHelp: vi.fn(),
  };

  return {
    mockProcessManager,
    mockPluginDiscovery,
    mockParameterCategorizer,
    mockSpecGenerator,
    mockCli,
  };
}