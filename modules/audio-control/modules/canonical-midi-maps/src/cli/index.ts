/**
 * CLI tools for Canonical MIDI Maps
 *
 * This module provides command-line tools for working with canonical MIDI map files:
 * - Validation of YAML/JSON files
 * - Format conversion between YAML, JSON, and XML
 * - Batch processing with parallel execution
 * - Template generation for common controllers and plugins
 *
 * All CLI tools support both individual files and batch directory operations,
 * with comprehensive error reporting and validation.
 */

// Re-export all CLI tools for programmatic use
export { MapValidator, main as validateMapsMain } from './validate-maps.js';
export { MapConverter, main as convertMapsMain } from './convert-maps.js';
export { BatchProcessor, main as batchProcessMain } from './batch-process.js';
export { TemplateGenerator, main as generateTemplateMain } from './generate-template.js';

// CLI tool metadata for discovery
export const CLI_TOOLS = {
  'validate-maps': {
    description: 'Validate canonical MIDI map files',
    module: './validate-maps.js',
    examples: [
      'validate-maps controller.yaml',
      'validate-maps --strict maps/',
      'validate-maps --verbose controller.yaml'
    ]
  },
  'convert-maps': {
    description: 'Convert MIDI maps between formats',
    module: './convert-maps.js',
    examples: [
      'convert-maps --to json controller.yaml',
      'convert-maps --from yaml --to json maps/',
      'convert-maps --backup --overwrite controller.yaml --to json'
    ]
  },
  'batch-process': {
    description: 'Batch process multiple MIDI map files',
    module: './batch-process.js',
    examples: [
      'batch-process validate maps/',
      'batch-process convert --format json --parallel 4 maps/',
      'batch-process analyze --verbose large-collection/'
    ]
  },
  'generate-template': {
    description: 'Generate MIDI map templates and scaffolding',
    module: './generate-template.js',
    examples: [
      'generate-template --device "Novation Launchkey MK3"',
      'generate-template --plugin serum --output serum-map.yaml',
      'generate-template --type test --name my-controller'
    ]
  }
} as const;

/**
 * Get CLI tool information
 */
export function getToolInfo(toolName: string) {
  return CLI_TOOLS[toolName as keyof typeof CLI_TOOLS];
}

/**
 * List all available CLI tools
 */
export function listTools() {
  return Object.entries(CLI_TOOLS).map(([name, info]) => ({
    name,
    description: info.description,
    examples: info.examples
  }));
}