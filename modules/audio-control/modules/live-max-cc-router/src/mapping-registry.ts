/**
 * Mapping registry that merges canonical and runtime plugin mappings.
 *
 * This registry provides a unified interface for accessing plugin mappings,
 * automatically merging build-time canonical defaults with runtime user
 * customizations. Runtime mappings take precedence over canonical defaults.
 *
 * @module mapping-registry
 */

import { CANONICAL_PLUGIN_MAPS, AVAILABLE_CONTROLLERS } from '@/canonical-plugin-maps';
import { PluginMapping, PluginMappingsDatabase, Controller } from '@/types/plugin-mappings';
import { loadRuntimeMappings } from '@/runtime-loader';
import { logger } from '@/logger';

/**
 * Registry for accessing merged canonical and runtime plugin mappings.
 *
 * This class combines two sources of plugin mappings:
 * 1. Canonical defaults (from YAML, compiled at build time)
 * 2. Runtime customizations (from JSON, written by LiveDeployer)
 *
 * Runtime mappings override canonical defaults for the same key.
 */
export class MappingRegistry {
  private readonly mergedMappings: PluginMappingsDatabase;
  private readonly runtimeMappingsPath: string | null;

  /**
   * Creates a new MappingRegistry.
   *
   * @param runtimeMappingsPath - Optional path to runtime mappings JSON file.
   *                              If not provided, only canonical defaults are used.
   *
   * @example
   * ```typescript
   * const registry = new MappingRegistry('/path/to/data/plugin-mappings.json');
   * const mapping = registry.getPluginMapping('Launch Control XL 3', 'CHANNEV');
   * ```
   */
  constructor(runtimeMappingsPath?: string) {
    this.runtimeMappingsPath = runtimeMappingsPath || null;
    this.mergedMappings = this.buildMergedMappings();
  }

  /**
   * Builds the merged mappings database.
   *
   * Merges canonical and runtime mappings, with runtime taking precedence.
   * This allows users to override specific mappings without modifying the
   * canonical defaults.
   *
   * @private
   * @returns Merged PluginMappingsDatabase
   */
  private buildMergedMappings(): PluginMappingsDatabase {
    // Start with canonical defaults
    const merged: PluginMappingsDatabase = { ...CANONICAL_PLUGIN_MAPS };
    const canonicalCount = Object.keys(CANONICAL_PLUGIN_MAPS).length;

    logger.debug(`Starting with ${canonicalCount} canonical mapping(s)`);

    // Load and merge runtime mappings if path provided
    if (this.runtimeMappingsPath) {
      const runtimeMappings = loadRuntimeMappings(this.runtimeMappingsPath);
      const runtimeKeys = Object.keys(runtimeMappings);

      if (runtimeKeys.length > 0) {
        let overrideCount = 0;
        let newCount = 0;

        for (let i = 0; i < runtimeKeys.length; i++) {
          const key = runtimeKeys[i];
          if (!key) continue;

          const runtimeMapping = runtimeMappings[key];
          if (!runtimeMapping) continue;

          if (merged[key]) {
            overrideCount++;
            logger.debug(`Runtime mapping overrides canonical for key: ${key}`);
          } else {
            newCount++;
            logger.debug(`Runtime mapping adds new key: ${key}`);
          }
          merged[key] = runtimeMapping;
        }

        logger.info(`Merged ${runtimeKeys.length} runtime mapping(s) (${overrideCount} overrides, ${newCount} new)`);
      }
    }

    const finalCount = Object.keys(merged).length;
    logger.info(`Registry initialized with ${finalCount} total mapping(s)`);

    return merged;
  }

  /**
   * Gets a plugin mapping by its composite key.
   *
   * The key format is "{controller-slug}_{plugin-slug}" where slugs are
   * lowercase, hyphen-separated versions of the names.
   *
   * @param key - Composite key (e.g., "launch-control-xl-3_channev")
   * @returns PluginMapping if found, undefined otherwise
   *
   * @example
   * ```typescript
   * const mapping = registry.getMapping('launch-control-xl-3_channev');
   * if (mapping) {
   *   console.log(`Found mapping for ${mapping.pluginName}`);
   * }
   * ```
   */
  public getMapping(key: string): PluginMapping | undefined {
    return this.mergedMappings[key];
  }

  /**
   * Gets a plugin mapping by controller and plugin name.
   *
   * This method performs fuzzy matching by normalizing the controller and
   * plugin names to lowercase slugs (replacing non-alphanumeric characters
   * with hyphens).
   *
   * @param controllerModel - Controller model name (e.g., "Launch Control XL 3")
   * @param pluginName - Plugin name (e.g., "CHANNEV", "TAL-J-8")
   * @returns PluginMapping if found, undefined otherwise
   *
   * @example
   * ```typescript
   * const mapping = registry.getPluginMapping('Launch Control XL 3', 'CHANNEV');
   * if (mapping) {
   *   const ccMappings = mapping.mappings;
   *   // Apply mappings...
   * }
   * ```
   */
  public getPluginMapping(controllerModel: string, pluginName: string): PluginMapping | undefined {
    // Normalize to slug format
    const controllerSlug = controllerModel.toLowerCase().replace(/[^a-z0-9]+/g, '-');
    const pluginSlug = pluginName.toLowerCase().replace(/[^a-z0-9]+/g, '-');
    const key = `${controllerSlug}_${pluginSlug}`;

    logger.debug(`Looking up mapping: controller="${controllerModel}" (${controllerSlug}), plugin="${pluginName}" (${pluginSlug})`);

    const mapping = this.mergedMappings[key];
    if (mapping) {
      logger.debug(`Found mapping for key: ${key}`);
    } else {
      logger.debug(`No mapping found for key: ${key}`);
    }

    return mapping;
  }

  /**
   * Gets all available MIDI controllers.
   *
   * Returns the list of controllers that have at least one mapping
   * defined in the registry.
   *
   * @returns Array of Controller objects
   *
   * @example
   * ```typescript
   * const controllers = registry.getAvailableControllers();
   * controllers.forEach(ctrl => {
   *   console.log(`${ctrl.manufacturer} ${ctrl.model}`);
   * });
   * ```
   */
  public getAvailableControllers(): Controller[] {
    return Object.values(AVAILABLE_CONTROLLERS);
  }

  /**
   * Alias for getAvailableControllers() for backward compatibility.
   *
   * @returns Array of Controller objects
   */
  public getControllers(): Controller[] {
    return this.getAvailableControllers();
  }

  /**
   * Gets all mapping keys in the registry.
   *
   * Useful for debugging and discovering available mappings.
   *
   * @returns Array of mapping keys
   *
   * @example
   * ```typescript
   * const keys = registry.getAllKeys();
   * console.log(`Available mappings: ${keys.join(', ')}`);
   * ```
   */
  public getAllKeys(): string[] {
    return Object.keys(this.mergedMappings);
  }

  /**
   * Gets count of mappings in the registry.
   *
   * @returns Total number of plugin mappings
   */
  public getMappingCount(): number {
    return Object.keys(this.mergedMappings).length;
  }
}

/**
 * Factory function to create a MappingRegistry instance.
 *
 * This is the recommended way to create a registry, providing a clean
 * interface and allowing for future initialization logic.
 *
 * @param runtimeMappingsPath - Optional path to runtime mappings JSON file
 * @returns New MappingRegistry instance
 *
 * @example
 * ```typescript
 * // Using canonical defaults only
 * const registry = createMappingRegistry();
 *
 * // With runtime customizations
 * const registry = createMappingRegistry('/path/to/data/plugin-mappings.json');
 * ```
 */
export function createMappingRegistry(runtimeMappingsPath?: string): MappingRegistry {
  return new MappingRegistry(runtimeMappingsPath);
}
