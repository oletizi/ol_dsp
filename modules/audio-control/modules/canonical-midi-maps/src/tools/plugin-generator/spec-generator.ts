/**
 * Plugin specification generation service
 */

import { writeFileSync } from 'fs';
import { join } from 'path';
import type {
  PluginInfo,
  PluginSpec,
  PluginParameter,
  ProcessedParameter,
  ParameterGroup,
  GeneratorArgs
} from './types.js';
import type { PlughostProcessManager } from './process-manager.js';
import type { IParameterCategorizer } from './parameter-categorizer.js';

/**
 * Interface for plugin specification generation
 */
export interface ISpecGenerator {
  generatePluginSpec(plugin: PluginInfo): Promise<PluginSpec | null>;
  savePluginSpec(plugin: PluginInfo, spec: PluginSpec, outputDir: string): string;
}

/**
 * Plugin specification generator options
 */
export interface SpecGeneratorOptions {
  processManager: PlughostProcessManager;
  parameterCategorizer: IParameterCategorizer;
  args: GeneratorArgs;
}

/**
 * Implementation of plugin specification generation
 */
export class SpecGenerator implements ISpecGenerator {
  constructor(private readonly options: SpecGeneratorOptions) {}

  /**
   * Generate a complete plugin specification
   */
  async generatePluginSpec(plugin: PluginInfo): Promise<PluginSpec | null> {
    const { processManager, parameterCategorizer, args } = this.options;

    console.log(`📋 Generating spec: ${plugin.name} (${plugin.manufacturer})`);

    // Skip known problematic plugins unless explicitly requested
    if (args.quick && processManager.isProblematicPlugin(plugin.name)) {
      console.log(`⏭️  Skipping problematic plugin in quick mode: ${plugin.name}`);
      return null;
    }

    try {
      const paramData = await this.getPluginParameters(plugin);
      if (!paramData) {
        return null;
      }

      const parameters = this.processParameters(paramData, parameterCategorizer);
      const groups = this.groupParameters(parameters);

      const spec: PluginSpec = {
        plugin: {
          manufacturer: plugin.manufacturer,
          name: plugin.name,
          version: plugin.version,
          format: plugin.format,
          uid: plugin.uid
        },
        metadata: {
          version: '1.0.0',
          created: new Date().toISOString(),
          author: 'plughost-generator',
          description: `Auto-generated parameter descriptor for ${plugin.name}`,
          parameter_count: parameters.length,
          tags: [
            'auto-generated',
            plugin.category.toLowerCase(),
            this.createSafeIdentifier(plugin.manufacturer)
          ]
        },
        parameters,
        groups
      };

      return spec;

    } catch (error) {
      console.error(`❌ Failed to generate spec for ${plugin.name}:`, error instanceof Error ? error.message : error);
      return null;
    }
  }

  /**
   * Get plugin parameters using plughost interrogation
   */
  private async getPluginParameters(plugin: PluginInfo): Promise<PluginParameter[] | null> {
    const { processManager, args } = this.options;

    const interrogateArgs = [
      '--interrogate', plugin.name,
      '--json',
      ...(args.quick ? ['--quick-scan'] : [])
    ];

    const timeout = processManager.isProblematicPlugin(plugin.name) ? 20000 : 45000; // Shorter timeout for problematic plugins
    const output = await processManager.runPlughost(interrogateArgs, timeout);

    // Extract JSON from output
    const jsonMatch = output.match(/\{[\s\S]*\}/);
    if (!jsonMatch) {
      console.log(`⚠️  No parameter data for ${plugin.name}`);
      return null;
    }

    const paramData = JSON.parse(jsonMatch[0]);

    if (!paramData.parameters || !Array.isArray(paramData.parameters)) {
      console.log(`⚠️  No parameters found for ${plugin.name}`);
      return null;
    }

    return paramData.parameters;
  }

  /**
   * Process raw parameters into spec format
   */
  private processParameters(
    parameters: PluginParameter[],
    categorizer: IParameterCategorizer
  ): ProcessedParameter[] {
    return parameters.map((param: PluginParameter) => ({
      index: param.index,
      name: param.name,
      min: param.discrete ? 0 : 0.0,
      max: param.discrete ? 1 : 1.0,
      default: param.default_value,
      group: categorizer.categorizeParameter(param.name),
      type: param.discrete ? 'discrete' : 'continuous' as const,
      automatable: param.automatable,
      ...(param.label && param.label !== param.name ? { label: param.label } : {})
    }));
  }

  /**
   * Group parameters by category
   */
  private groupParameters(parameters: ProcessedParameter[]): Record<string, ParameterGroup> {
    const groups: Record<string, ParameterGroup> = {};

    parameters.forEach((param: ProcessedParameter) => {
      if (!groups[param.group]) {
        groups[param.group] = {
          name: param.group.charAt(0).toUpperCase() + param.group.slice(1),
          parameters: []
        };
      }
      groups[param.group]!.parameters.push(param.index);
    });

    return groups;
  }

  /**
   * Save plugin specification to file
   */
  savePluginSpec(plugin: PluginInfo, spec: PluginSpec, outputDir: string): string {
    const safeManufacturer = this.createSafeIdentifier(plugin.manufacturer);
    const safeName = this.createSafeIdentifier(plugin.name);
    const filename = `${safeManufacturer}-${safeName}.json`;
    const filepath = join(outputDir, filename);

    writeFileSync(filepath, JSON.stringify(spec, null, 2));
    return filepath;
  }

  /**
   * Create a safe identifier from a string
   */
  private createSafeIdentifier(input: string): string {
    return input.toLowerCase().replace(/[^a-z0-9]/g, '-');
  }
}

/**
 * Factory function to create spec generator
 */
export function createSpecGenerator(options: SpecGeneratorOptions): ISpecGenerator {
  return new SpecGenerator(options);
}