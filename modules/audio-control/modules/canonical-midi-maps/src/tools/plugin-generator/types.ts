/**
 * Core type definitions for plugin specification generation
 */

/**
 * Basic plugin information from scanning
 */
export interface PluginInfo {
  manufacturer: string;
  name: string;
  version: string;
  format: string;
  uid: string;
  category: string;
}

/**
 * Raw plugin parameter data from plughost
 */
export interface PluginParameter {
  index: number;
  name: string;
  label: string;
  text: string;
  default_value: number;
  current_value: number;
  automatable: boolean;
  meta_parameter: boolean;
  discrete: boolean;
}

/**
 * Processed parameter data for spec output
 */
export interface ProcessedParameter {
  index: number;
  name: string;
  min: number;
  max: number;
  default: number;
  group: string;
  type: 'continuous' | 'discrete' | 'boolean';
  automatable: boolean;
  label?: string;
}

/**
 * Parameter group definition
 */
export interface ParameterGroup {
  name: string;
  parameters: number[];
}

/**
 * Complete plugin specification output format
 */
export interface PluginSpec {
  plugin: {
    manufacturer: string;
    name: string;
    version: string;
    format: string;
    uid: string;
  };
  metadata: {
    version: string;
    created: string;
    author: string;
    description: string;
    parameter_count: number;
    tags: string[];
  };
  parameters: ProcessedParameter[];
  groups: Record<string, ParameterGroup>;
}

/**
 * Command line arguments for the generator
 */
export interface GeneratorArgs {
  format: string | undefined;
  quick: boolean;
  help: boolean;
}

/**
 * Configuration for plugin process execution
 */
export interface ProcessConfig {
  timeoutMs?: number;
  env?: Record<string, string>;
  args?: string[];
}

/**
 * Result from plugin process execution
 */
export interface ProcessResult {
  stdout: string;
  stderr: string;
  exitCode: number;
}