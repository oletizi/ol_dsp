/**
 * Registry system for managing canonical MIDI maps.
 * Provides storage, search, and discovery functionality for MIDI map collections.
 */

/**
 * Represents a single MIDI map entry in the registry.
 * Contains metadata and file references for map discovery and loading.
 *
 * @example
 * ```typescript
 * const entry: MapRegistryEntry = {
 *   id: 'novation-launchkey-mk3-massive-x',
 *   filePath: 'maps/novation-launchkey-mk3-massive-x.yaml',
 *   metadata: {
 *     name: 'Novation Launchkey MK3 → Massive X',
 *     version: '1.0.0',
 *     description: 'Professional synthesizer mapping',
 *     author: 'Audio Team',
 *     tags: ['synthesizer', 'novation', 'native-instruments']
 *   },
 *   controller: {
 *     manufacturer: 'Novation',
 *     model: 'Launchkey MK3 49'
 *   },
 *   plugin: {
 *     manufacturer: 'Native Instruments',
 *     name: 'Massive X',
 *     format: 'VST3'
 *   }
 * };
 * ```
 */
export interface MapRegistryEntry {
  /** Unique identifier for this registry entry */
  id: string;
  /** Path to the MIDI map file (relative or absolute) */
  filePath: string;
  /** Map metadata for organization and discovery */
  metadata: {
    /** Human-readable name for the mapping */
    name: string;
    /** Semantic version of the mapping */
    version: string;
    /** Detailed description of the mapping */
    description?: string;
    /** Author or creator of the mapping */
    author?: string;
    /** Tags for categorization and search */
    tags?: string[];
  };
  /** MIDI controller/device information */
  controller: {
    /** Controller manufacturer name */
    manufacturer: string;
    /** Controller model name */
    model: string;
  };
  /** Plugin/instrument information */
  plugin: {
    /** Plugin manufacturer name */
    manufacturer: string;
    /** Plugin name */
    name: string;
    /** Plugin format (VST3, AU, etc.) */
    format?: string;
  };
}

/**
 * Registry for managing canonical MIDI maps.
 * Provides in-memory storage and search capabilities for MIDI map collections.
 *
 * Features:
 * - Registration and management of map entries
 * - Search by controller, plugin, tags, or free text
 * - Statistics and analytics on registered maps
 * - Built-in maps for common controller/plugin combinations
 *
 * @example
 * ```typescript
 * const registry = new CanonicalMapRegistry();
 *
 * // Register a new map
 * registry.register({
 *   id: 'my-custom-map',
 *   filePath: 'maps/custom.yaml',
 *   metadata: { name: 'My Custom Map', version: '1.0.0' },
 *   controller: { manufacturer: 'Akai', model: 'MPK Mini' },
 *   plugin: { manufacturer: 'Arturia', name: 'Pigments' }
 * });
 *
 * // Search for maps
 * const novationMaps = registry.findByController('Novation');
 * const synthMaps = registry.findByTags(['synthesizer']);
 * const searchResults = registry.search('massive');
 * ```
 */
export class CanonicalMapRegistry {
  private maps = new Map<string, MapRegistryEntry>();

  /**
   * Registers a new MIDI map in the registry.
   * If an entry with the same ID already exists, it will be replaced.
   *
   * @param entry - The map registry entry to add
   *
   * @example
   * ```typescript
   * registry.register({
   *   id: 'akai-mpk-arturia-pigments',
   *   filePath: 'maps/akai-mpk-pigments.yaml',
   *   metadata: {
   *     name: 'Akai MPK → Arturia Pigments',
   *     version: '1.0.0',
   *     tags: ['synthesizer', 'akai', 'arturia']
   *   },
   *   controller: { manufacturer: 'Akai', model: 'MPK Mini MK3' },
   *   plugin: { manufacturer: 'Arturia', name: 'Pigments' }
   * });
   * ```
   */
  register(entry: MapRegistryEntry): void {
    this.maps.set(entry.id, entry);
  }

  /**
   * Removes a MIDI map from the registry by ID.
   *
   * @param id - The unique identifier of the map to remove
   * @returns True if the map was found and removed, false otherwise
   *
   * @example
   * ```typescript
   * const removed = registry.unregister('old-map-id');
   * if (removed) {
   *   console.log('Map successfully removed');
   * }
   * ```
   */
  unregister(id: string): boolean {
    return this.maps.delete(id);
  }

  /**
   * Retrieves a specific MIDI map entry by its ID.
   *
   * @param id - The unique identifier of the map to retrieve
   * @returns The map entry if found, undefined otherwise
   *
   * @example
   * ```typescript
   * const map = registry.get('novation-launchkey-mk3-massive-x');
   * if (map) {
   *   console.log(`Found map: ${map.metadata.name}`);
   *   // Load the actual map file using map.filePath
   * }
   * ```
   */
  get(id: string): MapRegistryEntry | undefined {
    return this.maps.get(id);
  }

  /**
   * Retrieves all registered MIDI map entries.
   *
   * @returns Array of all registry entries
   *
   * @example
   * ```typescript
   * const allMaps = registry.getAll();
   * console.log(`Total maps: ${allMaps.length}`);
   *
   * allMaps.forEach(map => {
   *   console.log(`${map.controller.manufacturer} ${map.controller.model} → ${map.plugin.name}`);
   * });
   * ```
   */
  getAll(): MapRegistryEntry[] {
    return Array.from(this.maps.values());
  }

  /**
   * Finds MIDI maps by controller manufacturer and optionally model.
   * Search is case-insensitive and supports partial matching.
   *
   * @param manufacturer - Controller manufacturer to search for
   * @param model - Optional model name to further filter results
   * @returns Array of matching registry entries
   *
   * @example
   * ```typescript
   * // Find all Novation controllers
   * const novationMaps = registry.findByController('Novation');
   *
   * // Find specific model
   * const launchkeyMaps = registry.findByController('Novation', 'Launchkey MK3');
   *
   * // Partial matching works too
   * const akaiMaps = registry.findByController('aka'); // Finds Akai controllers
   * ```
   */
  findByController(manufacturer: string, model?: string): MapRegistryEntry[] {
    return this.getAll().filter(entry => {
      const controllerMatch = entry.controller.manufacturer.toLowerCase().includes(manufacturer.toLowerCase());
      if (model) {
        return controllerMatch && entry.controller.model.toLowerCase().includes(model.toLowerCase());
      }
      return controllerMatch;
    });
  }

  /**
   * Finds MIDI maps by plugin manufacturer and optionally plugin name.
   * Search is case-insensitive and supports partial matching.
   *
   * @param manufacturer - Plugin manufacturer to search for
   * @param name - Optional plugin name to further filter results
   * @returns Array of matching registry entries
   *
   * @example
   * ```typescript
   * // Find all Native Instruments plugins
   * const niMaps = registry.findByPlugin('Native Instruments');
   *
   * // Find specific plugin
   * const massiveMaps = registry.findByPlugin('Native Instruments', 'Massive X');
   *
   * // Partial matching
   * const arturiaMaps = registry.findByPlugin('art'); // Finds Arturia plugins
   * ```
   */
  findByPlugin(manufacturer: string, name?: string): MapRegistryEntry[] {
    return this.getAll().filter(entry => {
      const manufacturerMatch = entry.plugin.manufacturer.toLowerCase().includes(manufacturer.toLowerCase());
      if (name) {
        return manufacturerMatch && entry.plugin.name.toLowerCase().includes(name.toLowerCase());
      }
      return manufacturerMatch;
    });
  }

  /**
   * Finds MIDI maps that have any of the specified tags.
   * Search is case-insensitive and supports partial matching.
   *
   * @param tags - Array of tags to search for
   * @returns Array of entries that match any of the provided tags
   *
   * @example
   * ```typescript
   * // Find synthesizer-related maps
   * const synthMaps = registry.findByTags(['synthesizer', 'synth']);
   *
   * // Find maps for specific workflows
   * const studioMaps = registry.findByTags(['studio', 'production', 'mixing']);
   *
   * // Single tag search
   * const novationMaps = registry.findByTags(['novation']);
   * ```
   */
  findByTags(tags: string[]): MapRegistryEntry[] {
    return this.getAll().filter(entry => {
      if (!entry.metadata.tags) return false;
      return tags.some(tag => 
        entry.metadata.tags?.some(entryTag => 
          entryTag.toLowerCase().includes(tag.toLowerCase())
        )
      );
    });
  }

  /**
   * Performs a full-text search across all map metadata.
   * Searches through names, descriptions, manufacturers, models, and tags.
   *
   * @param query - Search query string (case-insensitive)
   * @returns Array of entries matching the search query
   *
   * @example
   * ```typescript
   * // Search for any maps related to 'massive'
   * const massiveResults = registry.search('massive');
   *
   * // Search for controller models
   * const mk3Results = registry.search('mk3');
   *
   * // Search for workflow types
   * const productionMaps = registry.search('production');
   * ```
   */
  search(query: string): MapRegistryEntry[] {
    const lowerQuery = query.toLowerCase();
    return this.getAll().filter(entry => {
      return (
        entry.metadata.name.toLowerCase().includes(lowerQuery) ||
        entry.metadata.description?.toLowerCase().includes(lowerQuery) ||
        entry.controller.manufacturer.toLowerCase().includes(lowerQuery) ||
        entry.controller.model.toLowerCase().includes(lowerQuery) ||
        entry.plugin.manufacturer.toLowerCase().includes(lowerQuery) ||
        entry.plugin.name.toLowerCase().includes(lowerQuery) ||
        entry.metadata.tags?.some(tag => tag.toLowerCase().includes(lowerQuery))
      );
    });
  }

  /**
   * Gets statistical information about the registered maps.
   * Useful for analytics, reporting, and understanding map collection.
   *
   * @returns Statistics object with counts and lists
   *
   * @example
   * ```typescript
   * const stats = registry.getStats();
   *
   * console.log(`Total maps: ${stats.totalMaps}`);
   * console.log(`Controller manufacturers: ${stats.controllerManufacturers.join(', ')}`);
   * console.log(`Plugin formats: ${stats.formats.join(', ')}`);
   * console.log(`Most common tags: ${stats.tags.slice(0, 5).join(', ')}`);
   * ```
   */
  getStats(): {
    /** Total number of registered maps */
    totalMaps: number;
    /** List of all controller manufacturers (sorted) */
    controllerManufacturers: string[];
    /** List of all plugin manufacturers (sorted) */
    pluginManufacturers: string[];
    /** List of all plugin formats (sorted) */
    formats: string[];
    /** List of all tags used (sorted) */
    tags: string[];
  } {
    const entries = this.getAll();
    const controllerManufacturers = new Set<string>();
    const pluginManufacturers = new Set<string>();
    const formats = new Set<string>();
    const tags = new Set<string>();

    entries.forEach(entry => {
      controllerManufacturers.add(entry.controller.manufacturer);
      pluginManufacturers.add(entry.plugin.manufacturer);
      if (entry.plugin.format) {
        formats.add(entry.plugin.format);
      }
      entry.metadata.tags?.forEach(tag => tags.add(tag));
    });

    return {
      totalMaps: entries.length,
      controllerManufacturers: Array.from(controllerManufacturers).sort(),
      pluginManufacturers: Array.from(pluginManufacturers).sort(),
      formats: Array.from(formats).sort(),
      tags: Array.from(tags).sort(),
    };
  }
}

/**
 * Default global registry instance.
 * Pre-populated with built-in MIDI maps for common controller/plugin combinations.
 * Most applications should use this instance rather than creating their own.
 *
 * @example
 * ```typescript
 * import { defaultRegistry } from '@/registry/map-registry.js';
 *
 * // Search the default registry
 * const results = defaultRegistry.search('launchkey');
 *
 * // Add your own maps to the default registry
 * defaultRegistry.register(myCustomMap);
 * ```
 */
export const defaultRegistry = new CanonicalMapRegistry();

// Register built-in maps
defaultRegistry.register({
  id: 'novation-launchkey-mk3-massive-x',
  filePath: 'maps/novation-launchkey-mk3-massive-x.yaml',
  metadata: {
    name: 'Novation Launchkey MK3 → Native Instruments Massive X',
    version: '1.0.0',
    description: 'Comprehensive mapping for controlling Massive X synthesizer with Novation Launchkey MK3',
    author: 'Audio Control Team',
    tags: ['synthesizer', 'novation', 'native-instruments', 'launchkey', 'massive-x'],
  },
  controller: {
    manufacturer: 'Novation',
    model: 'Launchkey MK3 49',
  },
  plugin: {
    manufacturer: 'Native Instruments',
    name: 'Massive X',
    format: 'VST3',
  },
});

defaultRegistry.register({
  id: 'akai-mpk-mini-arturia-pigments',
  filePath: 'maps/akai-mpk-mini-arturia-pigments.yaml',
  metadata: {
    name: 'Akai MPK Mini MK3 → Arturia Pigments',
    version: '1.0.0',
    description: 'Compact mapping for controlling Arturia Pigments with Akai MPK Mini MK3',
    author: 'Audio Control Team',
    tags: ['synthesizer', 'akai', 'arturia', 'mpk-mini', 'pigments'],
  },
  controller: {
    manufacturer: 'Akai',
    model: 'MPK Mini MK3',
  },
  plugin: {
    manufacturer: 'Arturia',
    name: 'Pigments',
    format: 'VST3',
  },
});