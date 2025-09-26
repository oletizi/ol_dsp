#!/usr/bin/env node

/**
 * CLI tool for generating MIDI map templates and development helpers.
 * Creates boilerplate files, example configurations, and development scaffolding.
 *
 * Features:
 * - Generate template MIDI maps for common controllers
 * - Create plugin-specific mapping templates
 * - Generate development scaffolding (tests, docs)
 * - Interactive template creation wizard
 * - Template validation and customization
 *
 * @example
 * ```bash
 * # Generate basic template
 * generate-template --device "Novation Launchkey MK3" --output my-controller.yaml
 *
 * # Generate plugin template
 * generate-template --plugin "Native Instruments Massive X" --interactive
 *
 * # Generate test scaffolding
 * generate-template --type test --name my-controller
 * ```
 */

import { writeFileSync, statSync, mkdirSync } from 'fs';
import { join, dirname, extname } from 'path';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';
import type { CanonicalMidiMapOutput } from '@/validators/schema.js';

type TemplateType = 'basic' | 'plugin' | 'test' | 'documentation';
type OutputFormat = 'yaml' | 'json';

interface TemplateOptions {
  type: TemplateType;
  device?: string;
  plugin?: string;
  output?: string;
  format: OutputFormat;
  interactive: boolean;
  overwrite: boolean;
  verbose: boolean;
  help: boolean;
  name?: string;
}

interface DeviceTemplate {
  manufacturer: string;
  model: string;
  controls: Array<{
    id: string;
    name: string;
    type: 'encoder' | 'slider' | 'button' | 'button_group';
    cc: number;
    description?: string;
    range?: [number, number];
  }>;
  defaultChannel: number;
}

interface PluginTemplate {
  manufacturer: string;
  name: string;
  format?: string;
  commonParameters: Array<{
    name: string;
    category: string;
    description?: string;
  }>;
}

class TemplateGenerator {
  private readonly deviceTemplates: Record<string, DeviceTemplate> = {
    'novation-launchkey-mk3': {
      manufacturer: 'Novation',
      model: 'Launchkey MK3',
      defaultChannel: 1,
      controls: [
        { id: 'volume', name: 'Volume', type: 'slider', cc: 7, description: 'Master volume control' },
        { id: 'pan', name: 'Pan', type: 'encoder', cc: 10, description: 'Stereo panning' },
        { id: 'filter', name: 'Filter', type: 'encoder', cc: 74, description: 'Low-pass filter cutoff' },
        { id: 'resonance', name: 'Resonance', type: 'encoder', cc: 71, description: 'Filter resonance' },
        { id: 'attack', name: 'Attack', type: 'encoder', cc: 73, description: 'Envelope attack time' },
        { id: 'decay', name: 'Decay', type: 'encoder', cc: 75, description: 'Envelope decay time' },
        { id: 'sustain', name: 'Sustain', type: 'encoder', cc: 79, description: 'Envelope sustain level' },
        { id: 'release', name: 'Release', type: 'encoder', cc: 72, description: 'Envelope release time' },
      ],
    },
    'akai-mpk-mini-mk3': {
      manufacturer: 'Akai',
      model: 'MPK Mini MK3',
      defaultChannel: 1,
      controls: [
        { id: 'knob1', name: 'Knob 1', type: 'encoder', cc: 1, description: 'User-assignable knob 1' },
        { id: 'knob2', name: 'Knob 2', type: 'encoder', cc: 2, description: 'User-assignable knob 2' },
        { id: 'knob3', name: 'Knob 3', type: 'encoder', cc: 3, description: 'User-assignable knob 3' },
        { id: 'knob4', name: 'Knob 4', type: 'encoder', cc: 4, description: 'User-assignable knob 4' },
        { id: 'knob5', name: 'Knob 5', type: 'encoder', cc: 5, description: 'User-assignable knob 5' },
        { id: 'knob6', name: 'Knob 6', type: 'encoder', cc: 6, description: 'User-assignable knob 6' },
        { id: 'knob7', name: 'Knob 7', type: 'encoder', cc: 7, description: 'User-assignable knob 7' },
        { id: 'knob8', name: 'Knob 8', type: 'encoder', cc: 8, description: 'User-assignable knob 8' },
      ],
    },
    'arturia-beatstep': {
      manufacturer: 'Arturia',
      model: 'BeatStep',
      defaultChannel: 1,
      controls: [
        { id: 'encoder1', name: 'Encoder 1', type: 'encoder', cc: 1 },
        { id: 'encoder2', name: 'Encoder 2', type: 'encoder', cc: 2 },
        { id: 'encoder3', name: 'Encoder 3', type: 'encoder', cc: 3 },
        { id: 'encoder4', name: 'Encoder 4', type: 'encoder', cc: 4 },
        { id: 'encoder5', name: 'Encoder 5', type: 'encoder', cc: 5 },
        { id: 'encoder6', name: 'Encoder 6', type: 'encoder', cc: 6 },
        { id: 'encoder7', name: 'Encoder 7', type: 'encoder', cc: 7 },
        { id: 'encoder8', name: 'Encoder 8', type: 'encoder', cc: 8 },
        { id: 'encoder9', name: 'Encoder 9', type: 'encoder', cc: 9 },
        { id: 'encoder10', name: 'Encoder 10', type: 'encoder', cc: 10 },
        { id: 'encoder11', name: 'Encoder 11', type: 'encoder', cc: 11 },
        { id: 'encoder12', name: 'Encoder 12', type: 'encoder', cc: 12 },
        { id: 'encoder13', name: 'Encoder 13', type: 'encoder', cc: 13 },
        { id: 'encoder14', name: 'Encoder 14', type: 'encoder', cc: 14 },
        { id: 'encoder15', name: 'Encoder 15', type: 'encoder', cc: 15 },
        { id: 'encoder16', name: 'Encoder 16', type: 'encoder', cc: 16 },
      ],
    },
  };

  private readonly pluginTemplates: Record<string, PluginTemplate> = {
    'serum': {
      manufacturer: 'Xfer Records',
      name: 'Serum',
      format: 'VST3',
      commonParameters: [
        { name: 'Osc A Level', category: 'Oscillator', description: 'Oscillator A volume level' },
        { name: 'Osc B Level', category: 'Oscillator', description: 'Oscillator B volume level' },
        { name: 'Filter Cutoff', category: 'Filter', description: 'Low-pass filter cutoff frequency' },
        { name: 'Filter Resonance', category: 'Filter', description: 'Filter resonance/Q factor' },
        { name: 'Env 1 Attack', category: 'Envelope', description: 'Envelope 1 attack time' },
        { name: 'Env 1 Decay', category: 'Envelope', description: 'Envelope 1 decay time' },
        { name: 'Env 1 Sustain', category: 'Envelope', description: 'Envelope 1 sustain level' },
        { name: 'Env 1 Release', category: 'Envelope', description: 'Envelope 1 release time' },
        { name: 'LFO 1 Rate', category: 'Modulation', description: 'LFO 1 frequency/rate' },
        { name: 'LFO 1 Amount', category: 'Modulation', description: 'LFO 1 modulation amount' },
      ],
    },
    'massive-x': {
      manufacturer: 'Native Instruments',
      name: 'Massive X',
      format: 'VST3',
      commonParameters: [
        { name: 'Osc 1 Level', category: 'Oscillator', description: 'Oscillator 1 output level' },
        { name: 'Osc 2 Level', category: 'Oscillator', description: 'Oscillator 2 output level' },
        { name: 'Filter 1 Cutoff', category: 'Filter', description: 'Filter 1 cutoff frequency' },
        { name: 'Filter 1 Resonance', category: 'Filter', description: 'Filter 1 resonance' },
        { name: 'Env 1 Attack', category: 'Envelope', description: 'Envelope 1 attack time' },
        { name: 'Env 1 Decay', category: 'Envelope', description: 'Envelope 1 decay time' },
        { name: 'Env 1 Sustain', category: 'Envelope', description: 'Envelope 1 sustain level' },
        { name: 'Env 1 Release', category: 'Envelope', description: 'Envelope 1 release time' },
        { name: 'Macro 1', category: 'Macro', description: 'Macro control 1' },
        { name: 'Macro 2', category: 'Macro', description: 'Macro control 2' },
        { name: 'Macro 3', category: 'Macro', description: 'Macro control 3' },
        { name: 'Macro 4', category: 'Macro', description: 'Macro control 4' },
      ],
    },
  };

  /**
   * Parse command line arguments
   */
  parseArgs(args: string[] = process.argv.slice(2)): TemplateOptions {
    const options: TemplateOptions = {
      type: 'basic',
      format: 'yaml',
      interactive: false,
      overwrite: false,
      verbose: false,
      help: false,
    };

    for (let i = 0; i < args.length; i++) {
      const arg = args[i];

      if (arg === '--help' || arg === '-h') {
        options.help = true;
      } else if (arg === '--type' || arg === '-t') {
        if (i + 1 < args.length) {
          const type = args[i + 1] as TemplateType;
          if (!['basic', 'plugin', 'test', 'documentation'].includes(type)) {
            console.error(`Invalid template type: ${args[i + 1]}`);
            process.exit(1);
          }
          options.type = type;
          i++;
        }
      } else if (arg === '--device' || arg === '-d') {
        if (i + 1 < args.length) {
          options.device = args[i + 1];
          i++;
        }
      } else if (arg === '--plugin' || arg === '-p') {
        if (i + 1 < args.length) {
          options.plugin = args[i + 1];
          i++;
        }
      } else if (arg === '--output' || arg === '-o') {
        if (i + 1 < args.length) {
          options.output = args[i + 1];
          i++;
        }
      } else if (arg === '--name' || arg === '-n') {
        if (i + 1 < args.length) {
          options.name = args[i + 1];
          i++;
        }
      } else if (arg === '--format' || arg === '-f') {
        if (i + 1 < args.length) {
          const format = args[i + 1].toLowerCase() as OutputFormat;
          if (!['yaml', 'json'].includes(format)) {
            console.error(`Invalid format: ${args[i + 1]}`);
            process.exit(1);
          }
          options.format = format;
          i++;
        }
      } else if (arg === '--interactive' || arg === '-i') {
        options.interactive = true;
      } else if (arg === '--overwrite') {
        options.overwrite = true;
      } else if (arg === '--verbose' || arg === '-v') {
        options.verbose = true;
      } else {
        console.error(`Unknown option: ${arg}`);
        process.exit(1);
      }
    }

    return options;
  }

  /**
   * Display help information
   */
  showHelp(): void {
    console.log(`Canonical MIDI Map Template Generator

Usage:
  generate-template [options]

Template Types:
  basic           Generate basic MIDI map template
  plugin          Generate plugin-specific mapping template
  test            Generate test files and scaffolding
  documentation   Generate documentation templates

Options:
  -h, --help              Show this help message
  -t, --type <type>       Template type [default: basic]
  -d, --device <name>     Device name or template key
  -p, --plugin <name>     Plugin name or template key
  -n, --name <name>       Template name/identifier
  -o, --output <file>     Output file path
  -f, --format <format>   Output format (yaml, json) [default: yaml]
  -i, --interactive       Interactive template creation
  --overwrite             Overwrite existing files
  -v, --verbose           Show detailed information

Device Templates:
  novation-launchkey-mk3     Novation Launchkey MK3 series
  akai-mpk-mini-mk3          Akai MPK Mini MK3
  arturia-beatstep           Arturia BeatStep

Plugin Templates:
  serum                      Xfer Records Serum
  massive-x                  Native Instruments Massive X

Examples:
  generate-template --device "Novation Launchkey MK3"          # Basic device template
  generate-template --plugin serum --output serum-map.yaml    # Plugin template
  generate-template --type test --name my-controller          # Test scaffolding
  generate-template --interactive                             # Interactive wizard

Exit Codes:
  0    Template generated successfully
  1    Generation failed
  2    Invalid command line arguments`);
  }

  /**
   * List available device templates
   */
  listDeviceTemplates(): void {
    console.log('📱 Available Device Templates:');
    for (const [key, template] of Object.entries(this.deviceTemplates)) {
      console.log(`  ${key}: ${template.manufacturer} ${template.model} (${template.controls.length} controls)`);
    }
  }

  /**
   * List available plugin templates
   */
  listPluginTemplates(): void {
    console.log('🎛️  Available Plugin Templates:');
    for (const [key, template] of Object.entries(this.pluginTemplates)) {
      console.log(`  ${key}: ${template.manufacturer} ${template.name} (${template.commonParameters.length} parameters)`);
    }
  }

  /**
   * Generate a basic MIDI map template
   */
  private generateBasicTemplate(options: TemplateOptions): CanonicalMidiMapOutput {
    let deviceTemplate: DeviceTemplate;

    // Try to find a matching device template
    const deviceKey = options.device?.toLowerCase().replace(/\s+/g, '-');
    if (deviceKey && this.deviceTemplates[deviceKey]) {
      deviceTemplate = this.deviceTemplates[deviceKey];
    } else if (options.device) {
      // Parse custom device string
      const parts = options.device.split(' ');
      const manufacturer = parts[0] || 'Unknown';
      const model = parts.slice(1).join(' ') || 'Controller';

      deviceTemplate = {
        manufacturer,
        model,
        defaultChannel: 1,
        controls: [
          { id: 'control1', name: 'Control 1', type: 'encoder', cc: 1, description: 'User-assignable control 1' },
          { id: 'control2', name: 'Control 2', type: 'encoder', cc: 2, description: 'User-assignable control 2' },
          { id: 'control3', name: 'Control 3', type: 'slider', cc: 7, description: 'Volume/level control' },
          { id: 'control4', name: 'Control 4', type: 'button', cc: 64, description: 'Toggle/switch control' },
        ],
      };
    } else {
      // Default generic template
      deviceTemplate = {
        manufacturer: 'Generic',
        model: 'MIDI Controller',
        defaultChannel: 1,
        controls: [
          { id: 'volume', name: 'Volume', type: 'slider', cc: 7, description: 'Master volume control' },
          { id: 'pan', name: 'Pan', type: 'encoder', cc: 10, description: 'Stereo panning' },
          { id: 'filter', name: 'Filter', type: 'encoder', cc: 74, description: 'Filter cutoff frequency' },
          { id: 'resonance', name: 'Resonance', type: 'encoder', cc: 71, description: 'Filter resonance' },
        ],
      };
    }

    const template: CanonicalMidiMapOutput = {
      version: '1.0.0',
      device: {
        manufacturer: deviceTemplate.manufacturer,
        model: deviceTemplate.model,
      },
      metadata: {
        name: options.name || `${deviceTemplate.manufacturer} ${deviceTemplate.model} Map`,
        description: `MIDI mapping for ${deviceTemplate.manufacturer} ${deviceTemplate.model}`,
        author: 'Generated by template generator',
        date: new Date().toISOString().split('T')[0],
        tags: ['template', 'generated', deviceTemplate.manufacturer.toLowerCase()],
      },
      midi_channel: deviceTemplate.defaultChannel,
      controls: deviceTemplate.controls.map(control => ({
        id: control.id,
        name: control.name,
        type: control.type,
        cc: control.cc,
        ...(control.description && { description: control.description }),
        ...(control.range && { range: control.range }),
      })),
    };

    return template;
  }

  /**
   * Generate a plugin-specific template
   */
  private generatePluginTemplate(options: TemplateOptions): CanonicalMidiMapOutput {
    let pluginTemplate: PluginTemplate;
    let deviceTemplate: DeviceTemplate;

    // Find plugin template
    const pluginKey = options.plugin?.toLowerCase().replace(/\s+/g, '-');
    if (pluginKey && this.pluginTemplates[pluginKey]) {
      pluginTemplate = this.pluginTemplates[pluginKey];
    } else if (options.plugin) {
      // Parse custom plugin string
      const parts = options.plugin.split(' ');
      const manufacturer = parts[0] || 'Unknown';
      const name = parts.slice(1).join(' ') || 'Plugin';

      pluginTemplate = {
        manufacturer,
        name,
        commonParameters: [
          { name: 'Parameter 1', category: 'General', description: 'Plugin parameter 1' },
          { name: 'Parameter 2', category: 'General', description: 'Plugin parameter 2' },
        ],
      };
    } else {
      pluginTemplate = {
        manufacturer: 'Generic',
        name: 'Plugin',
        commonParameters: [
          { name: 'Parameter 1', category: 'General' },
          { name: 'Parameter 2', category: 'General' },
        ],
      };
    }

    // Use a default device template
    const deviceKey = options.device?.toLowerCase().replace(/\s+/g, '-') || 'novation-launchkey-mk3';
    deviceTemplate = this.deviceTemplates[deviceKey] || this.deviceTemplates['novation-launchkey-mk3'];

    const template: CanonicalMidiMapOutput = {
      version: '1.0.0',
      device: {
        manufacturer: deviceTemplate.manufacturer,
        model: deviceTemplate.model,
      },
      metadata: {
        name: options.name || `${pluginTemplate.name} - ${deviceTemplate.model} Map`,
        description: `MIDI mapping for ${pluginTemplate.manufacturer} ${pluginTemplate.name} using ${deviceTemplate.manufacturer} ${deviceTemplate.model}`,
        author: 'Generated by template generator',
        date: new Date().toISOString().split('T')[0],
        tags: ['template', 'plugin', pluginTemplate.manufacturer.toLowerCase(), deviceTemplate.manufacturer.toLowerCase()],
      },
      plugin: {
        manufacturer: pluginTemplate.manufacturer,
        name: pluginTemplate.name,
        ...(pluginTemplate.format && { format: pluginTemplate.format as any }),
        description: `${pluginTemplate.manufacturer} ${pluginTemplate.name}`,
      },
      midi_channel: deviceTemplate.defaultChannel,
      controls: deviceTemplate.controls.slice(0, pluginTemplate.commonParameters.length).map((control, index) => {
        const param = pluginTemplate.commonParameters[index];
        return {
          id: control.id,
          name: control.name,
          type: control.type,
          cc: control.cc,
          description: `Control for ${param.name} (${param.category})`,
          plugin_parameter: param.name,
        };
      }),
    };

    return template;
  }

  /**
   * Generate test scaffolding
   */
  private generateTestTemplate(options: TemplateOptions): string {
    const testName = options.name || 'test-controller';
    const className = testName.split('-').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join('');

    return `import { describe, it, expect } from 'vitest';
import { CanonicalMapParser } from '@/parsers/yaml-parser.js';
import { readFileSync } from 'fs';
import { join } from 'path';

describe('${className} MIDI Map', () => {
  const mapPath = join(__dirname, '../maps/${testName}.yaml');

  it('should load and validate the map file', () => {
    const content = readFileSync(mapPath, 'utf8');
    const result = CanonicalMapParser.parseFromYAML(content);

    expect(result.validation.valid).toBe(true);
    expect(result.validation.errors).toHaveLength(0);
    expect(result.map).toBeDefined();
  });

  it('should have correct device information', () => {
    const content = readFileSync(mapPath, 'utf8');
    const result = CanonicalMapParser.parseFromYAML(content);

    expect(result.map?.device.manufacturer).toBeTruthy();
    expect(result.map?.device.model).toBeTruthy();
  });

  it('should have valid MIDI CC assignments', () => {
    const content = readFileSync(mapPath, 'utf8');
    const result = CanonicalMapParser.parseFromYAML(content);

    if (result.map) {
      for (const control of result.map.controls) {
        if (control.cc !== undefined) {
          expect(control.cc).toBeGreaterThanOrEqual(0);
          expect(control.cc).toBeLessThanOrEqual(127);
        }
      }
    }
  });

  it('should have unique control IDs', () => {
    const content = readFileSync(mapPath, 'utf8');
    const result = CanonicalMapParser.parseFromYAML(content);

    if (result.map) {
      const ids = result.map.controls.map(c => c.id);
      const uniqueIds = new Set(ids);
      expect(uniqueIds.size).toBe(ids.length);
    }
  });

  it('should not have duplicate CC assignments', () => {
    const content = readFileSync(mapPath, 'utf8');
    const result = CanonicalMapParser.parseFromYAML(content);

    if (result.map) {
      const ccNumbers = result.map.controls
        .map(c => c.cc)
        .filter(cc => cc !== undefined);

      const uniqueCCs = new Set(ccNumbers);
      expect(uniqueCCs.size).toBe(ccNumbers.length);
    }
  });
});`;
  }

  /**
   * Generate documentation template
   */
  private generateDocumentationTemplate(options: TemplateOptions): string {
    const name = options.name || 'MIDI Controller';

    return `# ${name} MIDI Mapping

## Overview

This document describes the MIDI mapping configuration for the ${name}.

## Device Information

- **Manufacturer**: [Device Manufacturer]
- **Model**: [Device Model]
- **Firmware**: [Firmware Version if applicable]

## Plugin Information (if applicable)

- **Plugin Manufacturer**: [Plugin Manufacturer]
- **Plugin Name**: [Plugin Name]
- **Plugin Format**: [VST3/AU/etc.]

## Control Mappings

### Encoders/Knobs

| Control | MIDI CC | Parameter | Description |
|---------|---------|-----------|-------------|
| Knob 1  | CC 1    | [Parameter] | [Description] |
| Knob 2  | CC 2    | [Parameter] | [Description] |

### Sliders/Faders

| Control | MIDI CC | Parameter | Description |
|---------|---------|-----------|-------------|
| Fader 1 | CC 7    | Volume    | Master volume control |

### Buttons

| Control | MIDI CC | Mode | Parameter | Description |
|---------|---------|------|-----------|-------------|
| Button 1| CC 64   | Toggle | [Parameter] | [Description] |

## MIDI Channel Configuration

- **Default Channel**: 1
- **Channel Overrides**: [List any control-specific channel overrides]

## Setup Instructions

1. Load the MIDI map file in your DAW or application
2. Configure your MIDI controller to send on the correct channel
3. Test each control to ensure proper mapping

## Troubleshooting

### Common Issues

- **No response from controls**: Check MIDI channel configuration
- **Wrong parameter mapped**: Verify CC assignments match controller settings
- **Multiple controls affecting same parameter**: Check for duplicate CC assignments

### Validation

Run the map through the validation tool:

\`\`\`bash
validate-maps ${name.toLowerCase().replace(/\s+/g, '-')}.yaml
\`\`\`

## Customization

To customize this mapping:

1. Edit the YAML/JSON file directly
2. Modify CC assignments as needed
3. Add or remove controls
4. Update plugin parameter mappings
5. Re-validate the configuration

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0   | [Date] | Initial template |

---

Generated by Canonical MIDI Maps Template Generator`;
  }

  /**
   * Interactive template creation wizard
   */
  private async runInteractiveWizard(options: TemplateOptions): Promise<CanonicalMidiMapOutput> {
    // This would normally use a proper interactive prompt library
    // For now, we'll generate a comprehensive template with comments

    const template = this.generateBasicTemplate(options);

    // Add helpful comments to guide users
    template.metadata.description = `MIDI mapping template - customize as needed

Instructions:
1. Update device information to match your controller
2. Modify control mappings (CC numbers, names, descriptions)
3. Add plugin information if mapping to a specific plugin
4. Test with your hardware and adjust as needed
5. Run validation: validate-maps [filename]

For more help, see: generate-template --help`;

    return template;
  }

  /**
   * Main template generation function
   */
  async generate(options: TemplateOptions): Promise<boolean> {
    try {
      let content: string;
      let outputPath: string;

      // Determine output path
      if (options.output) {
        outputPath = options.output;
      } else {
        const baseName = options.name || `${options.type}-template`;
        const ext = options.format === 'json' ? '.json' : '.yaml';
        outputPath = `${baseName}${ext}`;
      }

      // Check if output file exists
      const outputExists = (() => {
        try {
          statSync(outputPath);
          return true;
        } catch {
          return false;
        }
      })();

      if (outputExists && !options.overwrite) {
        console.error(`❌ Output file ${outputPath} already exists. Use --overwrite to replace.`);
        return false;
      }

      // Generate content based on type
      switch (options.type) {
        case 'basic': {
          const template = this.generateBasicTemplate(options);
          if (options.interactive) {
            const interactiveTemplate = await this.runInteractiveWizard(options);
            content = options.format === 'json' ?
              CanonicalMapParser.serializeToJSON(interactiveTemplate) :
              CanonicalMapParser.serializeToYAML(interactiveTemplate);
          } else {
            content = options.format === 'json' ?
              CanonicalMapParser.serializeToJSON(template) :
              CanonicalMapParser.serializeToYAML(template);
          }
          break;
        }

        case 'plugin': {
          const template = this.generatePluginTemplate(options);
          content = options.format === 'json' ?
            CanonicalMapParser.serializeToJSON(template) :
            CanonicalMapParser.serializeToYAML(template);
          break;
        }

        case 'test': {
          content = this.generateTestTemplate(options);
          outputPath = outputPath.replace(/\.(yaml|json)$/, '.test.ts');
          break;
        }

        case 'documentation': {
          content = this.generateDocumentationTemplate(options);
          outputPath = outputPath.replace(/\.(yaml|json)$/, '.md');
          break;
        }

        default:
          console.error(`❌ Unknown template type: ${options.type}`);
          return false;
      }

      // Create output directory if needed
      const outputDir = dirname(outputPath);
      try {
        mkdirSync(outputDir, { recursive: true });
      } catch (error) {
        // Directory might already exist
      }

      // Write the file
      writeFileSync(outputPath, content, 'utf8');

      console.log(`✅ Generated ${options.type} template: ${outputPath}`);

      if (options.verbose) {
        console.log(`📄 Template details:`);
        console.log(`  Type: ${options.type}`);
        console.log(`  Format: ${options.format}`);
        console.log(`  Size: ${content.length} characters`);

        if (options.type === 'basic' || options.type === 'plugin') {
          // Parse and show template info
          let template;
          if (options.format === 'yaml') {
            template = CanonicalMapParser.parseFromYAML(content).map;
          } else {
            template = CanonicalMapParser.parseFromJSON(content).map;
          }

          if (template) {
            console.log(`  Device: ${template.device.manufacturer} ${template.device.model}`);
            console.log(`  Controls: ${template.controls.length}`);
            if (template.plugin) {
              console.log(`  Plugin: ${template.plugin.manufacturer} ${template.plugin.name}`);
            }
          }
        }
      }

      // Show next steps
      console.log('\n📋 Next steps:');
      switch (options.type) {
        case 'basic':
        case 'plugin':
          console.log(`  1. Edit ${outputPath} to match your controller`);
          console.log(`  2. Validate: validate-maps ${outputPath}`);
          console.log(`  3. Test with your hardware setup`);
          break;
        case 'test':
          console.log(`  1. Move ${outputPath} to your test directory`);
          console.log(`  2. Create the corresponding map file`);
          console.log(`  3. Run: npm test ${outputPath}`);
          break;
        case 'documentation':
          console.log(`  1. Edit ${outputPath} with your specific details`);
          console.log(`  2. Add to your project documentation`);
          break;
      }

      return true;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Unknown error';
      console.error(`❌ Failed to generate template: ${message}`);
      return false;
    }
  }
}

/**
 * Main CLI entry point
 */
async function main(): Promise<void> {
  const generator = new TemplateGenerator();
  const options = generator.parseArgs();

  if (options.help) {
    generator.showHelp();
    console.log('\n');
    generator.listDeviceTemplates();
    console.log('\n');
    generator.listPluginTemplates();
    process.exit(0);
  }

  try {
    const success = await generator.generate(options);
    process.exit(success ? 0 : 1);
  } catch (error) {
    const message = error instanceof Error ? error.message : 'Unknown error';
    console.error(`Fatal error: ${message}`);
    process.exit(2);
  }
}

// Run CLI if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  await main();
}

export { TemplateGenerator, main };