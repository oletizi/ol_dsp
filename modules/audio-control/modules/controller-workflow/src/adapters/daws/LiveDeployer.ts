/**
 * Ableton Live DAW Deployer
 *
 * Converts canonical MIDI maps to Max for Live cc-router format.
 * Updates the canonical-plugin-maps.ts file used by the M4L device.
 *
 * @module adapters/daws/LiveDeployer
 */

import { homedir } from 'os';
import { join } from 'path';
import { readFile, writeFile } from 'fs/promises';
import type { CanonicalMidiMap } from '@oletizi/canonical-midi-maps';
import type {
  DAWDeployerInterface,
  DeploymentOptions,
  DeploymentResult,
} from '../../types/daw-deployer.js';

interface PluginMapping {
  controller: {
    manufacturer?: string;
    model?: string;
  };
  pluginName: string;
  pluginManufacturer: string;
  mappings: {
    [ccNumber: number]: {
      deviceIndex: number;
      parameterIndex: number;
      parameterName: string;
      curve: 'linear' | 'exponential' | 'logarithmic';
    };
  };
  metadata: {
    name?: string;
    description?: string;
    version?: string;
  };
}

/**
 * Ableton Live DAW Deployer
 *
 * Converts canonical maps to Max for Live format and updates the cc-router's
 * canonical-plugin-maps.ts file so mappings are available in Live.
 */
export class LiveDeployer implements DAWDeployerInterface {
  readonly dawName = 'Ableton Live';
  readonly version = '11/12';

  /**
   * Factory method to create LiveDeployer instance
   */
  static create(): LiveDeployer {
    return new LiveDeployer();
  }

  /**
   * Deploy canonical map to Live's cc-router module
   *
   * @param canonicalMap - Canonical MIDI map
   * @param options - Deployment options
   * @returns Deployment result with path to updated canonical-plugin-maps.ts
   */
  async deploy(canonicalMap: CanonicalMidiMap, options: DeploymentOptions): Promise<DeploymentResult> {
    try {
      // Convert to M4L format
      const pluginMapping = this.convertToM4LFormat(canonicalMap);

      // Get path to live-max-cc-router module
      const ccRouterPath = this.getCCRouterPath();
      const canonicalMapsFile = join(ccRouterPath, 'src', 'canonical-plugin-maps.ts');

      if (options.dryRun) {
        const result: DeploymentResult = {
          success: true,
          dawName: this.dawName,
          outputPath: canonicalMapsFile,
          installed: false,
        };
        return result;
      }

      // Read existing canonical-plugin-maps.ts
      const existingContent = await readFile(canonicalMapsFile, 'utf-8');

      // Update with new mapping
      const updatedContent = await this.updateCanonicalMaps(existingContent, pluginMapping, canonicalMap);

      // Write back to file
      await writeFile(canonicalMapsFile, updatedContent, 'utf-8');

      return {
        success: true,
        dawName: this.dawName,
        outputPath: canonicalMapsFile,
        installed: true,
        message: `Updated ${canonicalMapsFile}. Rebuild cc-router with: cd ${ccRouterPath} && npm run build`,
      };
    } catch (error) {
      const errorMsg = error instanceof Error ? error.message : String(error);
      return {
        success: false,
        dawName: this.dawName,
        errors: [errorMsg],
      };
    }
  }

  /**
   * Get path to live-max-cc-router module
   */
  private getCCRouterPath(): string {
    // Assume we're in controller-workflow, navigate to sibling module
    return join(__dirname, '..', '..', '..', '..', 'live-max-cc-router');
  }

  /**
   * Convert canonical MIDI map to M4L plugin mapping format
   */
  private convertToM4LFormat(canonicalMap: CanonicalMidiMap): PluginMapping {
    const mappings: PluginMapping['mappings'] = {};

    // Convert each control to M4L format
    for (const control of canonicalMap.controls) {
      if (control.cc === undefined) {
        continue; // Skip controls without CC numbers
      }

      mappings[control.cc] = {
        deviceIndex: 1, // Device 1 is the first plugin after cc-router (device 0)
        parameterIndex: this.extractParameterIndex(control.id),
        parameterName: control.name || control.id,
        curve: 'linear', // Default curve, could be enhanced with metadata
      };
    }

    // Determine plugin name from map name or metadata
    const pluginName = canonicalMap.plugin?.name || canonicalMap.metadata.name || 'Unknown Plugin';
    const pluginManufacturer = canonicalMap.plugin?.manufacturer || 'Unknown';

    return {
      controller: {
        manufacturer: canonicalMap.device.manufacturer,
        model: canonicalMap.device.model,
      },
      pluginName,
      pluginManufacturer,
      mappings,
      metadata: {
        ...(canonicalMap.metadata.name !== undefined && { name: canonicalMap.metadata.name }),
        ...(canonicalMap.metadata.description !== undefined && { description: canonicalMap.metadata.description }),
        ...(canonicalMap.version !== undefined && { version: canonicalMap.version }),
      },
    };
  }

  /**
   * Extract parameter index from control ID
   * Control IDs like "control_16" map to parameter index based on semantic meaning
   */
  private extractParameterIndex(controlId: string): number {
    // Try to extract number from control ID
    const match = controlId.match(/(\d+)$/);
    if (match && match[1]) {
      return parseInt(match[1], 10);
    }
    // Fallback to 0 if no number found
    return 0;
  }

  /**
   * Update canonical-plugin-maps.ts with new mapping
   */
  private async updateCanonicalMaps(
    existingContent: string,
    newMapping: PluginMapping,
    canonicalMap: CanonicalMidiMap
  ): Promise<string> {
    // Generate mapping key from controller and plugin name
    const mappingKey = this.generateMappingKey(canonicalMap);

    // Convert mapping to TypeScript object literal
    const mappingString = this.serializePluginMapping(newMapping);

    // Check if mapping already exists
    const keyPattern = new RegExp(`"${mappingKey}"\\s*:\\s*{`, 'g');

    if (keyPattern.test(existingContent)) {
      // Replace existing mapping
      const replacePattern = new RegExp(
        `"${mappingKey}"\\s*:\\s*{[\\s\\S]*?^  },`,
        'gm'
      );
      return existingContent.replace(replacePattern, `"${mappingKey}": ${mappingString},`);
    } else {
      // Add new mapping
      // Find the closing of CANONICAL_PLUGIN_MAPS object
      const closingBraceIndex = existingContent.lastIndexOf('};');
      if (closingBraceIndex === -1) {
        throw new Error('Could not find CANONICAL_PLUGIN_MAPS object closing brace');
      }

      // Insert before closing brace
      const before = existingContent.substring(0, closingBraceIndex);
      const after = existingContent.substring(closingBraceIndex);

      // Add comma after last entry if needed
      const needsComma = !before.trimEnd().endsWith(',');
      const comma = needsComma ? ',' : '';

      return `${before}${comma}\n  "${mappingKey}": ${mappingString}\n${after}`;
    }
  }

  /**
   * Generate mapping key from canonical map
   */
  private generateMappingKey(canonicalMap: CanonicalMidiMap): string {
    const controller = (canonicalMap.device.model ?? 'unknown').toLowerCase().replace(/\s+/g, '-');
    const plugin = (canonicalMap.plugin?.name ?? canonicalMap.metadata.name ?? 'unknown')
      .toLowerCase()
      .replace(/\s+/g, '-');
    return `${controller}_${plugin}`;
  }

  /**
   * Serialize PluginMapping to TypeScript object literal string
   */
  private serializePluginMapping(mapping: PluginMapping): string {
    const lines: string[] = ['{'];

    // Controller
    lines.push('    "controller": {');
    if (mapping.controller.manufacturer) {
      lines.push(`      "manufacturer": "${mapping.controller.manufacturer}",`);
    }
    if (mapping.controller.model) {
      lines.push(`      "model": "${mapping.controller.model}"`);
    }
    lines.push('    },');

    // Plugin info
    lines.push(`    "pluginName": "${mapping.pluginName}",`);
    lines.push(`    "pluginManufacturer": "${mapping.pluginManufacturer}",`);

    // Mappings
    lines.push('    "mappings": {');
    const ccNumbers = Object.keys(mapping.mappings).sort((a, b) => parseInt(a) - parseInt(b));
    ccNumbers.forEach((ccStr, index) => {
      const cc = parseInt(ccStr);
      const m = mapping.mappings[cc];
      if (!m) {
        throw new Error(`Missing mapping for CC ${cc}`);
      }
      const comma = index < ccNumbers.length - 1 ? ',' : '';
      lines.push(`      "${cc}": {`);
      lines.push(`        "deviceIndex": ${m.deviceIndex},`);
      lines.push(`        "parameterIndex": ${m.parameterIndex},`);
      lines.push(`        "parameterName": "${m.parameterName}",`);
      lines.push(`        "curve": "${m.curve}"`);
      lines.push(`      }${comma}`);
    });
    lines.push('    },');

    // Metadata
    lines.push('    "metadata": {');
    const metadataLines: string[] = [];
    if (mapping.metadata.name) {
      metadataLines.push(`      "name": "${mapping.metadata.name}"`);
    }
    if (mapping.metadata.description) {
      metadataLines.push(`      "description": "${mapping.metadata.description}"`);
    }
    if (mapping.metadata.version) {
      metadataLines.push(`      "version": "${mapping.metadata.version}"`);
    }
    lines.push(metadataLines.join(',\n'));
    lines.push('    }');

    lines.push('  }');

    return lines.join('\n');
  }

  /**
   * Check if Ableton Live is installed
   */
  async isInstalled(): Promise<boolean> {
    // For now, assume Live is installed if cc-router module exists
    try {
      const ccRouterPath = this.getCCRouterPath();
      await readFile(join(ccRouterPath, 'package.json'), 'utf-8');
      return true;
    } catch {
      return false;
    }
  }

  /**
   * Get Ableton Live's configuration directory
   */
  async getConfigDirectory(): Promise<string> {
    const home = homedir();
    switch (process.platform) {
      case 'darwin':
        return join(home, 'Music', 'Ableton', 'User Library', 'Remote Scripts');
      case 'win32':
        return join(home, 'Documents', 'Ableton', 'User Library', 'Remote Scripts');
      case 'linux':
        return join(home, 'Ableton', 'User Library', 'Remote Scripts');
      default:
        throw new Error(`Unsupported platform: ${process.platform}`);
    }
  }
}

/**
 * Factory function for creating LiveDeployer instances
 */
export function createLiveDeployer(): LiveDeployer {
  return LiveDeployer.create();
}
