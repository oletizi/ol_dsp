/**
 * CLI tools for Ardour MIDI Maps
 *
 * This module provides command-line tools for working with Ardour MIDI map XML files:
 * - Validation of Ardour XML files with MIDI parameter checking
 * - Conversion from canonical format to Ardour XML
 * - Function mapping configuration and validation
 * - Device info generation and customization
 *
 * All tools are designed to work with Ardour's specific XML format requirements
 * and provide comprehensive validation of MIDI parameters and Ardour functions.
 */

// Re-export all CLI tools for programmatic use
export { ArdourMapValidator, main as validateArdourMapsMain } from './validate-ardour-maps.js';
export { CanonicalToArdourConverter, main as convertToArdourMain } from './convert-to-ardour.js';

// CLI tool metadata for discovery
export const CLI_TOOLS = {
  'validate-ardour-maps': {
    description: 'Validate Ardour MIDI map XML files',
    module: './validate-ardour-maps.js',
    examples: [
      'validate-ardour-maps controller.xml',
      'validate-ardour-maps --strict ardour-maps/',
      'validate-ardour-maps --verbose --no-function-check controller.xml'
    ]
  },
  'convert-to-ardour': {
    description: 'Convert canonical MIDI maps to Ardour XML format',
    module: './convert-to-ardour.js',
    examples: [
      'convert-to-ardour controller.yaml',
      'convert-to-ardour --functions custom-functions.json controller.yaml',
      'convert-to-ardour --output ardour-maps/ canonical-maps/'
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