/**
 * Runtime loader for user-customized plugin mappings.
 *
 * This module loads plugin mappings from the JSON file written by LiveDeployer,
 * allowing users to customize mappings at runtime without rebuilding the module.
 *
 * @module runtime-loader
 */

import { PluginMappingsDatabase } from '@/types/plugin-mappings';
import * as fs from 'fs';
import { logger } from '@/logger';

/**
 * Loads runtime plugin mappings from a JSON file.
 *
 * This function reads user-customized mappings that were written by the
 * LiveDeployer during the deployment process. If the file doesn't exist,
 * it returns an empty database - this is NOT considered an error since
 * the file is optional (canonical defaults will be used instead).
 *
 * @param jsonPath - Absolute path to the plugin-mappings.json file
 * @returns PluginMappingsDatabase object, or empty object if file not found
 * @throws Error if file exists but contains invalid JSON or malformed data
 *
 * @example
 * ```typescript
 * const runtimeMappings = loadRuntimeMappings('/path/to/data/plugin-mappings.json');
 * // Returns: { "launch-control-xl-3_channev": { ... }, ... }
 * ```
 */
export function loadRuntimeMappings(jsonPath: string): PluginMappingsDatabase {
  try {
    // Check if file exists
    if (!fs.existsSync(jsonPath)) {
      logger.debug(`Runtime mappings file not found at ${jsonPath} - using canonical defaults only`);
      return {};
    }

    // Read file synchronously (required for module initialization in Max for Live)
    const fileContent = fs.readFileSync(jsonPath, 'utf-8');

    // Parse JSON
    const parsed = JSON.parse(fileContent);

    // Basic validation: ensure it's an object
    if (typeof parsed !== 'object' || parsed === null) {
      throw new Error('Invalid runtime mappings: expected object, got ' + typeof parsed);
    }

    // Validate structure: check that values have expected shape
    const keys = Object.keys(parsed);
    for (let i = 0; i < keys.length; i++) {
      const key = keys[i];
      if (!key) continue;
      const mapping = parsed[key];

      if (!mapping || typeof mapping !== 'object') {
        throw new Error(`Invalid mapping for key "${key}": expected object`);
      }

      if (!mapping.controller || typeof mapping.controller !== 'object') {
        throw new Error(`Invalid mapping for key "${key}": missing or invalid controller`);
      }

      if (!mapping.pluginName || typeof mapping.pluginName !== 'string') {
        throw new Error(`Invalid mapping for key "${key}": missing or invalid pluginName`);
      }

      if (!mapping.mappings || typeof mapping.mappings !== 'object') {
        throw new Error(`Invalid mapping for key "${key}": missing or invalid mappings`);
      }
    }

    logger.info(`Loaded ${keys.length} runtime mapping(s) from ${jsonPath}`);
    return parsed as PluginMappingsDatabase;

  } catch (error: any) {
    // File not found is OK - already handled above
    if (error.code === 'ENOENT') {
      logger.debug(`Runtime mappings file not found: ${jsonPath}`);
      return {};
    }

    // JSON parsing errors or validation errors should be thrown
    logger.error(`Failed to load runtime mappings from ${jsonPath}: ${error.message}`);
    throw new Error(`Failed to load runtime mappings: ${error.message}`);
  }
}
