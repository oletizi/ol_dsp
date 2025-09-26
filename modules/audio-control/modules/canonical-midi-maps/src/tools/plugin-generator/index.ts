/**
 * Factory functions and main exports for plugin spec generator
 */

export * from './types.js';
export * from './cli.js';
export * from './parameter-categorizer.js';
export * from './process-manager.js';
export * from './plugin-discovery.js';
export * from './spec-generator.js';

import { createCli } from './cli.js';
import { createParameterCategorizer } from './parameter-categorizer.js';
import { createPlughostProcessManager } from './process-manager.js';
import { createPluginDiscovery } from './plugin-discovery.js';
import { createSpecGenerator } from './spec-generator.js';
import type { GeneratorArgs } from './types.js';

/**
 * Configuration for plugin spec generator
 */
export interface PluginSpecGeneratorConfig {
  plughostPath: string;
  outputDir: string;
  args?: GeneratorArgs;
}

/**
 * Factory function to create a fully configured plugin spec generator components
 */
export function createPluginSpecGeneratorComponents(config: PluginSpecGeneratorConfig) {
  const cli = createCli();
  const args = config.args || cli.parseArgs();
  const parameterCategorizer = createParameterCategorizer();
  const processManager = createPlughostProcessManager(config.plughostPath);

  const pluginDiscovery = createPluginDiscovery({
    processManager,
    args
  });

  const specGenerator = createSpecGenerator({
    processManager,
    parameterCategorizer,
    args
  });

  return {
    cli,
    args,
    parameterCategorizer,
    processManager,
    pluginDiscovery,
    specGenerator
  };
}