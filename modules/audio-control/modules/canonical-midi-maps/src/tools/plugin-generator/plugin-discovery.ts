/**
 * Plugin discovery service for scanning and listing plugins
 */

import type { PluginInfo, GeneratorArgs } from './types.js';
import type { PlughostProcessManager } from './process-manager.js';

/**
 * Interface for plugin discovery service
 */
export interface IPluginDiscovery {
  getPluginList(): Promise<PluginInfo[]>;
  shouldSkipPlugin(plugin: PluginInfo): boolean;
}

/**
 * Plugin discovery service options
 */
export interface PluginDiscoveryOptions {
  processManager: PlughostProcessManager;
  args: GeneratorArgs;
}

/**
 * Implementation of plugin discovery using plughost
 */
export class PluginDiscovery implements IPluginDiscovery {
  constructor(private readonly options: PluginDiscoveryOptions) {}

  /**
   * Get list of available plugins
   */
  async getPluginList(): Promise<PluginInfo[]> {
    const { args } = this.options;
    console.log(`🔍 Scanning for plugins${args.format ? ` (${args.format} only)` : ''}...`);

    try {
      return await this.getBatchPluginList();
    } catch (error) {
      console.log('⚠️  Batch JSON listing failed, trying fallback approach...');
      console.log(`Error: ${error instanceof Error ? error.message : error}`);
      return await this.getPluginListFallback();
    }
  }

  /**
   * Get plugin list using batch JSON approach
   */
  private async getBatchPluginList(): Promise<PluginInfo[]> {
    const { processManager, args } = this.options;

    // Try batch JSON approach first
    const plughostArgs = ['--list', '--skip-instantiation', '--batch-json'];
    if (args.quick) {
      plughostArgs.push('--quick-scan');
    }
    if (args.format) {
      plughostArgs.push('--format-filter', args.format);
    }

    console.log('🚀 Attempting batch JSON plugin listing...');
    const output = await processManager.runPlughost(plughostArgs, 180000); // 3 minute timeout for batch
    console.log('\n✅ Batch listing completed');

    // Extract JSON from output
    const jsonMatch = output.match(/\{[\s\S]*\}/);
    if (!jsonMatch) {
      throw new Error('No JSON found in batch output');
    }

    const pluginData = JSON.parse(jsonMatch[0]);

    if (!pluginData.plugins || !Array.isArray(pluginData.plugins)) {
      throw new Error('Invalid JSON structure');
    }

    let plugins = pluginData.plugins.map((p: any) => ({
      manufacturer: p.manufacturer || 'Unknown',
      name: p.name,
      version: p.version || '1.0.0',
      format: p.format,
      uid: p.uid || '',
      category: p.category || 'Effect'
    }));

    // Filter problematic plugins in quick mode
    if (args.quick) {
      plugins = this.filterProblematicPlugins(plugins);
    }

    return plugins;
  }

  /**
   * Fallback plugin listing using basic approach
   */
  private async getPluginListFallback(): Promise<PluginInfo[]> {
    const { processManager, args } = this.options;

    console.log('🔄 Falling back to basic plugin listing...');

    try {
      const output = await processManager.runPlughost(['--list'], 120000); // 2 minute timeout

      const plugins: PluginInfo[] = [];
      const lines = output.split('\n');

      for (const line of lines) {
        const match = line.match(/Next Plugin: <Format:([^>]+)>, <Name: ([^>]+)>/);
        if (match && match[1] && match[2]) {
          const format = match[1];
          const name = match[2];

          // Apply format filter if specified
          if (args.format && format !== args.format) {
            continue;
          }

          // Skip problematic plugins in quick mode
          if (args.quick && processManager.isProblematicPlugin(name)) {
            console.log(`⏭️  Skipping problematic plugin: ${name}`);
            continue;
          }

          plugins.push({
            manufacturer: 'Unknown', // Basic listing doesn't provide manufacturer
            name,
            version: '1.0.0',
            format,
            uid: '',
            category: 'Effect'
          });
        }
      }

      return plugins;
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : error;
      console.error('❌ Fallback listing also failed:', errorMessage);
      return [];
    }
  }

  /**
   * Filter out problematic plugins
   */
  private filterProblematicPlugins(plugins: PluginInfo[]): PluginInfo[] {
    const { processManager } = this.options;
    const originalCount = plugins.length;

    const filteredPlugins = plugins.filter((p: PluginInfo) => !processManager.isProblematicPlugin(p.name));
    const filteredCount = originalCount - filteredPlugins.length;

    if (filteredCount > 0) {
      console.log(`⏭️  Filtered out ${filteredCount} problematic plugins in quick mode`);
    }

    return filteredPlugins;
  }

  /**
   * Check if plugin should be skipped (system plugins, etc.)
   */
  shouldSkipPlugin(plugin: PluginInfo): boolean {
    // Skip system plugins
    return plugin.name.match(/^(AU|AUAudio|AUNet|AUMatrix|AUSample)/) !== null;
  }
}

/**
 * Factory function to create plugin discovery service
 */
export function createPluginDiscovery(options: PluginDiscoveryOptions): IPluginDiscovery {
  return new PluginDiscovery(options);
}