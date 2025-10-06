/**
 * TypeScript interfaces for plugin MIDI CC mappings.
 *
 * These interfaces define the structure for JSON data files that map
 * MIDI controller CC messages to plugin parameters in Ableton Live.
 *
 * @module types/plugin-mappings
 */

/**
 * Represents information about a MIDI controller hardware device.
 */
export interface ControllerInfo {
  /**
   * The manufacturer name of the controller (e.g., "Novation", "Arturia").
   * Optional for backwards compatibility.
   */
  manufacturer?: string;

  /**
   * The model name/number of the controller (e.g., "Launch Control XL 3").
   * Optional for backwards compatibility.
   */
  model?: string;
}

/**
 * Describes how a single CC number maps to a plugin parameter.
 */
export interface CCMapping {
  /**
   * The device index in Ableton Live's device chain (1-based indexing).
   * Typically 1 for the first device on a track.
   */
  deviceIndex: number;

  /**
   * The zero-based index of the parameter within the plugin.
   * This matches the parameter order exposed by the VST/AU plugin.
   */
  parameterIndex: number;

  /**
   * Human-readable name of the parameter being controlled.
   * Used for documentation and debugging purposes.
   */
  parameterName: string;

  /**
   * The response curve for the parameter mapping.
   * - 'linear': Direct 1:1 mapping of MIDI value to parameter
   * - 'exponential': Exponential curve for volume/gain-like parameters
   * - 'logarithmic': Logarithmic curve for frequency-like parameters
   */
  curve: 'linear' | 'exponential' | 'logarithmic';
}

/**
 * Complete mapping from a MIDI controller to a specific plugin.
 *
 * This represents a single "preset" or "template" that defines how
 * a particular hardware controller's knobs/faders map to a plugin's
 * parameters.
 */
export interface PluginMapping {
  /**
   * Information about the MIDI controller hardware.
   */
  controller: ControllerInfo;

  /**
   * The exact name of the plugin as it appears in Ableton Live.
   * Must match the plugin's display name for proper routing.
   */
  pluginName: string;

  /**
   * The manufacturer/vendor of the plugin (e.g., "Arturia", "TAL Software").
   */
  pluginManufacturer: string;

  /**
   * Map of MIDI CC numbers to their parameter mappings.
   *
   * The keys are CC numbers (0-127) as strings.
   * The values describe which plugin parameter each CC controls.
   *
   * Example:
   * ```json
   * {
   *   "5": {
   *     "deviceIndex": 1,
   *     "parameterIndex": 2,
   *     "parameterName": "Cutoff",
   *     "curve": "linear"
   *   }
   * }
   * ```
   */
  mappings: {
    [ccNumber: string]: CCMapping;
  };

  /**
   * Metadata about this mapping configuration.
   */
  metadata: {
    /**
     * Display name for this mapping preset.
     */
    name?: string;

    /**
     * Human-readable description of what this mapping is for.
     */
    description?: string;

    /**
     * Version number for tracking mapping iterations.
     * Recommended format: semver (e.g., "1.0.0")
     */
    version?: string;

    /**
     * Source type indicating where this mapping originated.
     * - 'canonical': Pre-defined mappings shipped with the application
     * - 'device-extracted': Mappings extracted from Live device state
     * - 'user-custom': User-created or manually edited mappings
     */
    source?: 'canonical' | 'device-extracted' | 'user-custom';

    /**
     * ISO 8601 timestamp of when this mapping was extracted from a device.
     * Only present when source is 'device-extracted'.
     */
    extractedAt?: string;

    /**
     * The controller slot number from which this mapping was extracted.
     * Only present when source is 'device-extracted'.
     * Useful for tracking which physical controller position was used.
     */
    controllerSlot?: number;
  };
}

/**
 * Database of all plugin mappings, organized by composite key.
 *
 * The key format is: "{controller-slug}_{plugin-slug}"
 * where slugs are lowercase, hyphen-separated versions of the names.
 *
 * Example keys:
 * - "launch-control-xl-3_channev"
 * - "launch-control-xl-3_tal-j-8"
 *
 * This structure allows for:
 * - Quick lookup by controller + plugin combination
 * - Multiple mappings for the same plugin with different controllers
 * - Runtime loading and updating without code changes
 */
export interface PluginMappingsDatabase {
  [key: string]: PluginMapping;
}

/**
 * Controller hardware information for the registry.
 */
export interface Controller {
  /**
   * Manufacturer name of the controller.
   */
  manufacturer: string;

  /**
   * Model name/number of the controller.
   */
  model: string;
}

/**
 * Registry of available MIDI controllers.
 *
 * The key is a slug version of the controller name.
 * Used for validation and controller selection.
 */
export interface ControllersRegistry {
  [key: string]: Controller;
}
