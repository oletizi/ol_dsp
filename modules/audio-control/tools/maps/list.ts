#!/usr/bin/env node

/**
 * Maps Listing Tool
 *
 * Lists available canonical MIDI mappings with metadata,
 * device information, and plugin associations.
 */

import { readFile } from 'fs/promises';
import { join, basename, extname, dirname } from 'path';
import { glob } from 'glob';

import { CanonicalMapParser } from '@/modules/canonical-midi-maps/src/parsers/yaml-parser';
import type { CanonicalMidiMapOutput } from '@/modules/canonical-midi-maps/src/validators/schema';

interface MapSummary {
  /** File path */
  filePath: string;

  /** Map identifier */
  mapId: string;

  /** Device information */
  device: {
    manufacturer: string;
    model: string;
    firmware?: string;
  };

  /** Plugin information (if applicable) */
  plugin?: {
    manufacturer: string;
    name: string;
    version?: string;
    format?: string;
  };

  /** Map metadata */
  metadata: {
    name: string;
    description?: string;
    author?: string;
    date?: string;
    tags?: string[];
    version?: string;
  };

  /** Control statistics */
  controls: {
    total: number;
    encoders: number;
    sliders: number;
    buttons: number;
    buttonGroups: number;
    mapped: number;
  };

  /** MIDI information */
  midi: {
    channel?: number;
    ccRange: [number, number] | null;
    uniqueCCs: number;
  };

  /** File information */
  file: {
    format: 'yaml' | 'json';
    size: number;
    lastModified: Date;
  };
}

interface ListingOptions {
  /** Output format */
  format?: 'table' | 'json' | 'csv';

  /** Filter by device manufacturer */
  manufacturer?: string;

  /** Filter by device model */
  model?: string;

  /** Filter by plugin */
  plugin?: string;

  /** Filter by tags */
  tags?: string[];

  /** Show only mapped controls */
  mappedOnly?: boolean;

  /** Sort by field */
  sortBy?: 'name' | 'device' | 'plugin' | 'date' | 'controls' | 'size';

  /** Sort order */
  sortOrder?: 'asc' | 'desc';

  /** Group by field */
  groupBy?: 'device' | 'plugin' | 'manufacturer';

  /** Include detailed control information */
  verbose?: boolean;
}

class MapsLister {
  constructor(private readonly options: ListingOptions = {}) {
    // Set defaults
    this.options.format ??= 'table';
    this.options.sortBy ??= 'name';
    this.options.sortOrder ??= 'asc';
  }

  /**
   * List maps from directory
   */
  async listMaps(searchPath: string): Promise<MapSummary[]> {
    // Find all map files
    const mapFiles = await glob(join(searchPath, '**/*.{yaml,yml,json}'));

    if (mapFiles.length === 0) {
      throw new Error(`No map files found in ${searchPath}`);
    }

    // Process each map file
    const maps: MapSummary[] = [];

    for (const filePath of mapFiles) {
      try {
        const summary = await this.processMapFile(filePath);
        if (this.shouldIncludeMap(summary)) {
          maps.push(summary);
        }
      } catch (error) {
        console.warn(`Failed to process ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
      }
    }

    // Sort maps
    this.sortMaps(maps);

    return maps;
  }

  /**
   * Process a single map file
   */
  private async processMapFile(filePath: string): Promise<MapSummary> {
    const content = await readFile(filePath, 'utf8');
    const stats = await import('fs/promises').then(fs => fs.stat(filePath));
    const fileExtension = extname(filePath).toLowerCase();

    // Parse the map
    let parseResult: { map?: CanonicalMidiMapOutput; validation: any };

    if (fileExtension === '.yaml' || fileExtension === '.yml') {
      parseResult = CanonicalMapParser.parseFromYAML(content);
    } else if (fileExtension === '.json') {
      parseResult = CanonicalMapParser.parseFromJSON(content);
    } else {
      throw new Error(`Unsupported file format: ${fileExtension}`);
    }

    if (!parseResult.validation.valid || !parseResult.map) {
      throw new Error(`Invalid map format: ${parseResult.validation.errors?.[0]?.message || 'Unknown error'}`);
    }

    const map = parseResult.map;

    // Calculate control statistics
    const controls = this.calculateControlStats(map);

    // Calculate MIDI information
    const midi = this.calculateMidiInfo(map);

    // Generate map ID
    const mapId = this.generateMapId(map);

    return {
      filePath,
      mapId,
      device: {
        manufacturer: map.device.manufacturer,
        model: map.device.model,
        firmware: map.device.firmware,
      },
      plugin: map.plugin ? {
        manufacturer: map.plugin.manufacturer,
        name: map.plugin.name,
        version: map.plugin.version,
        format: map.plugin.format,
      } : undefined,
      metadata: {
        name: map.metadata.name,
        description: map.metadata.description,
        author: map.metadata.author,
        date: map.metadata.date,
        tags: map.metadata.tags,
        version: map.version,
      },
      controls,
      midi,
      file: {
        format: (fileExtension === '.yaml' || fileExtension === '.yml') ? 'yaml' : 'json',
        size: stats.size,
        lastModified: stats.mtime,
      },
    };
  }

  /**
   * Calculate control statistics
   */
  private calculateControlStats(map: CanonicalMidiMapOutput): MapSummary['controls'] {
    let encoders = 0;
    let sliders = 0;
    let buttons = 0;
    let buttonGroups = 0;
    let mapped = 0;

    map.controls.forEach(control => {
      switch (control.type) {
        case 'encoder':
          encoders++;
          break;
        case 'slider':
          sliders++;
          break;
        case 'button':
          buttons++;
          break;
        case 'button_group':
          buttonGroups++;
          if (control.buttons) {
            buttons += control.buttons.length;
          }
          break;
      }

      // Check if control is mapped to plugin parameter
      if (control.plugin_parameter !== undefined) {
        mapped++;
      }

      // Check button mappings in button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach(button => {
          if (button.plugin_parameter !== undefined) {
            mapped++;
          }
        });
      }
    });

    return {
      total: map.controls.length,
      encoders,
      sliders,
      buttons,
      buttonGroups,
      mapped,
    };
  }

  /**
   * Calculate MIDI information
   */
  private calculateMidiInfo(map: CanonicalMidiMapOutput): MapSummary['midi'] {
    const ccNumbers = new Set<number>();
    let minCC = 127;
    let maxCC = 0;

    map.controls.forEach(control => {
      if (control.cc !== undefined) {
        ccNumbers.add(control.cc);
        minCC = Math.min(minCC, control.cc);
        maxCC = Math.max(maxCC, control.cc);
      }

      // Check button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach(button => {
          ccNumbers.add(button.cc);
          minCC = Math.min(minCC, button.cc);
          maxCC = Math.max(maxCC, button.cc);
        });
      }
    });

    return {
      channel: map.midi_channel,
      ccRange: ccNumbers.size > 0 ? [minCC, maxCC] : null,
      uniqueCCs: ccNumbers.size,
    };
  }

  /**
   * Check if map should be included based on filters
   */
  private shouldIncludeMap(map: MapSummary): boolean {
    // Filter by manufacturer
    if (this.options.manufacturer &&
        !map.device.manufacturer.toLowerCase().includes(this.options.manufacturer.toLowerCase())) {
      return false;
    }

    // Filter by model
    if (this.options.model &&
        !map.device.model.toLowerCase().includes(this.options.model.toLowerCase())) {
      return false;
    }

    // Filter by plugin
    if (this.options.plugin) {
      if (!map.plugin) return false;
      const pluginText = `${map.plugin.manufacturer} ${map.plugin.name}`.toLowerCase();
      if (!pluginText.includes(this.options.plugin.toLowerCase())) {
        return false;
      }
    }

    // Filter by tags
    if (this.options.tags && this.options.tags.length > 0) {
      if (!map.metadata.tags) return false;
      const hasMatchingTag = this.options.tags.some(tag =>
        map.metadata.tags?.some(mapTag =>
          mapTag.toLowerCase().includes(tag.toLowerCase())
        )
      );
      if (!hasMatchingTag) return false;
    }

    // Filter mapped only
    if (this.options.mappedOnly && map.controls.mapped === 0) {
      return false;
    }

    return true;
  }

  /**
   * Sort maps array
   */
  private sortMaps(maps: MapSummary[]): void {
    const { sortBy, sortOrder } = this.options;

    maps.sort((a, b) => {
      let comparison = 0;

      switch (sortBy) {
        case 'name':
          comparison = a.metadata.name.localeCompare(b.metadata.name);
          break;
        case 'device':
          comparison = `${a.device.manufacturer} ${a.device.model}`.localeCompare(
            `${b.device.manufacturer} ${b.device.model}`
          );
          break;
        case 'plugin':
          const aPlugin = a.plugin ? `${a.plugin.manufacturer} ${a.plugin.name}` : '';
          const bPlugin = b.plugin ? `${b.plugin.manufacturer} ${b.plugin.name}` : '';
          comparison = aPlugin.localeCompare(bPlugin);
          break;
        case 'date':
          const aDate = a.metadata.date || '';
          const bDate = b.metadata.date || '';
          comparison = aDate.localeCompare(bDate);
          break;
        case 'controls':
          comparison = a.controls.total - b.controls.total;
          break;
        case 'size':
          comparison = a.file.size - b.file.size;
          break;
        default:
          comparison = 0;
      }

      return sortOrder === 'desc' ? -comparison : comparison;
    });
  }

  /**
   * Output maps in requested format
   */
  outputMaps(maps: MapSummary[]): void {
    switch (this.options.format) {
      case 'json':
        console.log(JSON.stringify(maps, null, 2));
        break;
      case 'csv':
        this.outputCSV(maps);
        break;
      case 'table':
      default:
        this.outputTable(maps);
        break;
    }
  }

  /**
   * Output maps as table
   */
  private outputTable(maps: MapSummary[]): void {
    if (maps.length === 0) {
      console.log('No maps found matching criteria.');
      return;
    }

    // Group maps if requested
    if (this.options.groupBy) {
      this.outputGroupedTable(maps);
      return;
    }

    console.log(`Found ${maps.length} MIDI maps:\n`);

    // Calculate column widths
    const widths = {
      name: Math.max(20, Math.max(...maps.map(m => m.metadata.name.length))),
      device: Math.max(25, Math.max(...maps.map(m => `${m.device.manufacturer} ${m.device.model}`.length))),
      plugin: Math.max(20, Math.max(...maps.map(m => m.plugin ? `${m.plugin.manufacturer} ${m.plugin.name}`.length : 4))),
      controls: 8,
      mapped: 6,
    };

    // Header
    console.log(
      'Name'.padEnd(widths.name) + ' ' +
      'Device'.padEnd(widths.device) + ' ' +
      'Plugin'.padEnd(widths.plugin) + ' ' +
      'Controls'.padEnd(widths.controls) + ' ' +
      'Mapped'
    );
    console.log('-'.repeat(widths.name + widths.device + widths.plugin + widths.controls + widths.mapped + 4));

    // Rows
    maps.forEach(map => {
      const name = map.metadata.name.padEnd(widths.name);
      const device = `${map.device.manufacturer} ${map.device.model}`.padEnd(widths.device);
      const plugin = (map.plugin ? `${map.plugin.manufacturer} ${map.plugin.name}` : 'None').padEnd(widths.plugin);
      const controls = map.controls.total.toString().padEnd(widths.controls);
      const mapped = map.controls.mapped.toString();

      console.log(`${name} ${device} ${plugin} ${controls} ${mapped}`);

      if (this.options.verbose) {
        console.log(`  File: ${map.filePath}`);
        console.log(`  Description: ${map.metadata.description || 'None'}`);
        console.log(`  CC Range: ${map.midi.ccRange ? `${map.midi.ccRange[0]}-${map.midi.ccRange[1]}` : 'None'}`);
        console.log(`  Tags: ${map.metadata.tags?.join(', ') || 'None'}`);
        console.log('');
      }
    });

    if (!this.options.verbose) {
      console.log(`\nTotal: ${maps.length} maps`);
      const totalControls = maps.reduce((sum, m) => sum + m.controls.total, 0);
      const totalMapped = maps.reduce((sum, m) => sum + m.controls.mapped, 0);
      console.log(`Controls: ${totalControls} total, ${totalMapped} mapped (${((totalMapped / totalControls) * 100).toFixed(1)}%)`);
    }
  }

  /**
   * Output maps grouped by category
   */
  private outputGroupedTable(maps: MapSummary[]): void {
    const groups = new Map<string, MapSummary[]>();

    // Group maps
    maps.forEach(map => {
      let groupKey: string;

      switch (this.options.groupBy) {
        case 'device':
          groupKey = `${map.device.manufacturer} ${map.device.model}`;
          break;
        case 'plugin':
          groupKey = map.plugin ? `${map.plugin.manufacturer} ${map.plugin.name}` : 'No Plugin';
          break;
        case 'manufacturer':
          groupKey = map.device.manufacturer;
          break;
        default:
          groupKey = 'Other';
      }

      if (!groups.has(groupKey)) {
        groups.set(groupKey, []);
      }
      groups.get(groupKey)!.push(map);
    });

    // Output each group
    groups.forEach((groupMaps, groupName) => {
      console.log(`\n=== ${groupName} (${groupMaps.length} maps) ===`);

      groupMaps.forEach(map => {
        console.log(`  • ${map.metadata.name}`);
        if (this.options.verbose) {
          console.log(`    Plugin: ${map.plugin ? `${map.plugin.manufacturer} ${map.plugin.name}` : 'None'}`);
          console.log(`    Controls: ${map.controls.total} (${map.controls.mapped} mapped)`);
          console.log(`    File: ${basename(map.filePath)}`);
        }
      });
    });

    console.log(`\nTotal: ${maps.length} maps in ${groups.size} groups`);
  }

  /**
   * Output maps as CSV
   */
  private outputCSV(maps: MapSummary[]): void {
    // Header
    console.log('Name,Device Manufacturer,Device Model,Plugin Manufacturer,Plugin Name,Plugin Format,Total Controls,Mapped Controls,MIDI Channel,CC Range,File Format,File Size,File Path');

    // Rows
    maps.forEach(map => {
      const row = [
        this.csvEscape(map.metadata.name),
        this.csvEscape(map.device.manufacturer),
        this.csvEscape(map.device.model),
        this.csvEscape(map.plugin?.manufacturer || ''),
        this.csvEscape(map.plugin?.name || ''),
        this.csvEscape(map.plugin?.format || ''),
        map.controls.total,
        map.controls.mapped,
        map.midi.channel || '',
        map.midi.ccRange ? `${map.midi.ccRange[0]}-${map.midi.ccRange[1]}` : '',
        map.file.format,
        map.file.size,
        this.csvEscape(map.filePath),
      ];
      console.log(row.join(','));
    });
  }

  private csvEscape(value: string): string {
    if (value.includes(',') || value.includes('"') || value.includes('\n')) {
      return `"${value.replace(/"/g, '""')}"`;
    }
    return value;
  }

  private generateMapId(map: CanonicalMidiMapOutput): string {
    const device = `${map.device.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.device.model.toLowerCase().replace(/\s+/g, '-')}`;
    const plugin = map.plugin ? `_${map.plugin.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.plugin.name.toLowerCase().replace(/\s+/g, '-')}` : '';
    return device + plugin;
  }
}

// CLI interface
async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0) {
    console.error('Usage: list.ts <maps-directory> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --format <format>        Output format: table, json, csv (default: table)');
    console.error('  --manufacturer <name>    Filter by device manufacturer');
    console.error('  --model <name>           Filter by device model');
    console.error('  --plugin <name>          Filter by plugin name');
    console.error('  --tags <tag1,tag2>       Filter by tags (comma-separated)');
    console.error('  --mapped-only            Show only maps with plugin mappings');
    console.error('  --sort-by <field>        Sort by: name, device, plugin, date, controls, size');
    console.error('  --sort-order <order>     Sort order: asc, desc');
    console.error('  --group-by <field>       Group by: device, plugin, manufacturer');
    console.error('  --verbose                Show detailed information');
    process.exit(1);
  }

  const searchPath = args[0];
  const options: ListingOptions = {};

  for (let i = 1; i < args.length; i++) {
    switch (args[i]) {
      case '--format':
        options.format = args[++i] as 'table' | 'json' | 'csv';
        break;
      case '--manufacturer':
        options.manufacturer = args[++i];
        break;
      case '--model':
        options.model = args[++i];
        break;
      case '--plugin':
        options.plugin = args[++i];
        break;
      case '--tags':
        options.tags = args[++i].split(',').map(t => t.trim());
        break;
      case '--mapped-only':
        options.mappedOnly = true;
        break;
      case '--sort-by':
        options.sortBy = args[++i] as ListingOptions['sortBy'];
        break;
      case '--sort-order':
        options.sortOrder = args[++i] as 'asc' | 'desc';
        break;
      case '--group-by':
        options.groupBy = args[++i] as ListingOptions['groupBy'];
        break;
      case '--verbose':
        options.verbose = true;
        break;
    }
  }

  try {
    const lister = new MapsLister(options);
    const maps = await lister.listMaps(searchPath);
    lister.outputMaps(maps);
  } catch (error) {
    console.error(`Error: ${error instanceof Error ? error.message : 'Unknown error'}`);
    process.exit(1);
  }
}

if (require.main === module) {
  main().catch(console.error);
}

export { MapsLister, type MapSummary, type ListingOptions };