/**
 * Zod validation schemas for canonical MIDI map format.
 * Provides runtime validation with detailed error reporting for MIDI map configurations.
 *
 * All schemas validate according to MIDI specifications:
 * - MIDI channels: 1-16
 * - CC numbers: 0-127
 * - Note numbers: 0-127
 * - Parameter ranges and types
 *
 * @example
 * ```typescript
 * import { CanonicalMidiMapSchema } from '@/validators/schema.js';
 *
 * const result = CanonicalMidiMapSchema.safeParse(unknownData);
 * if (result.success) {
 *   console.log('Valid MIDI map:', result.data);
 * } else {
 *   console.error('Validation errors:', result.error.errors);
 * }
 * ```
 */

import { z } from 'zod';

/**
 * Schema for numeric value ranges with optional default values.
 * Used for parameter ranges, MIDI value ranges, etc.
 */
const ValueRangeSchema = z.object({
  /** Minimum value in the range */
  min: z.number(),
  /** Maximum value in the range */
  max: z.number(),
  /** Optional default value within the range */
  default: z.number().optional(),
});

/**
 * Schema for defining how MIDI input should be interpreted.
 * Controls physical input behavior like sensitivity, deadzones, and response curves.
 */
const InputBehaviorSchema = z.object({
  /** How input changes should be interpreted */
  mode: z.enum(['absolute', 'relative', 'toggle', 'momentary']).optional(),
  /** Sensitivity multiplier (0.0 = least sensitive, 1.0 = most sensitive) */
  sensitivity: z.number().min(0).max(1).optional(),
  /** Minimum change required to register input (0.0 = no deadzone, 1.0 = large deadzone) */
  deadzone: z.number().min(0).max(1).optional(),
  /** Response curve for value mapping */
  curve: z.enum(['linear', 'exponential', 'logarithmic', 'custom']).optional(),
  /** Whether to invert the control direction */
  invert: z.boolean().optional(),
});

/**
 * Legacy schema for mapping behavior configuration.
 * Defines how MIDI values are scaled and processed before reaching plugin parameters.
 *
 * @deprecated Use new canonical format instead. Kept for backwards compatibility during migration.
 */
const MappingBehaviorSchema = z.object({
  /** Value scaling algorithm */
  scaling: z.enum(['linear', 'exponential', 'logarithmic', 'custom']).optional(),
  /** Custom curve points for 'custom' scaling (normalized 0-1) */
  curve: z.array(z.number()).optional(),
  /** Quantization step size (0.01 = 1% steps, 1 = integer steps) */
  quantize: z.number().optional(),
  /** Smoothing factor (0 = no smoothing, 1 = maximum smoothing) */
  smoothing: z.number().min(0).max(1).optional(),
  /** Whether parameter has negative and positive ranges around center */
  bipolar: z.boolean().optional(),
});

/**
 * Legacy schema for MIDI input configuration.
 * Defines what MIDI messages to listen for and how to interpret them.
 *
 * @deprecated Use new canonical format instead. Kept for backwards compatibility during migration.
 */
const MidiInputDefinitionSchema = z.object({
  /** Type of MIDI message to handle */
  type: z.enum(['cc', 'note', 'pitchbend', 'aftertouch', 'program']),
  /** MIDI channel to listen on (1-16, omit for all channels) */
  channel: z.number().min(1).max(16).optional(),
  /** CC number, note number, or program number (0-127, depending on type) */
  number: z.number().min(0).max(127).optional(),
  /** Expected input value range */
  range: ValueRangeSchema.optional(),
  /** Input interpretation behavior */
  behavior: InputBehaviorSchema.optional(),
});

/**
 * Legacy schema for plugin parameter targeting.
 * Defines what aspect of the plugin to control and its characteristics.
 *
 * @deprecated Use new canonical format instead. Kept for backwards compatibility during migration.
 */
const PluginTargetDefinitionSchema = z.object({
  /** Type of plugin control being targeted */
  type: z.enum(['parameter', 'bypass', 'preset', 'macro']),
  /** Unique identifier for the parameter (URI, name, or index) */
  identifier: z.string(),
  /** Human-readable parameter name */
  name: z.string().optional(),
  /** Parameter value range */
  range: ValueRangeSchema.optional(),
  /** Parameter units (Hz, dB, %, etc.) */
  units: z.string().optional(),
  /** Parameter category for organization */
  category: z.string().optional(),
});

/**
 * Legacy schemas exported for migration scripts and backwards compatibility.
 * These schemas validate the older MIDI mapping format during the transition period.
 *
 * @deprecated Use CanonicalMidiMapSchema for new implementations
 */
export { MappingBehaviorSchema, MidiInputDefinitionSchema, PluginTargetDefinitionSchema };

/**
 * Schema for individual button definitions within button groups.
 * Validates button properties including MIDI CC assignments and behavior modes.
 */
const ButtonDefinitionSchema = z.object({
  /** Unique identifier for this button within the group */
  id: z.string(),
  /** Display name for the button */
  name: z.string(),
  /** MIDI CC number for this button (0-127) */
  cc: z.number().min(0).max(127),
  /** MIDI channel (string for registry reference or number 1-16) */
  channel: z.union([z.string(), z.number()]),
  /** Button behavior: toggle (on/off) or momentary (press/release) */
  mode: z.enum(['toggle', 'momentary']),
  /** Target plugin parameter for this button */
  plugin_parameter: z.union([z.string(), z.number()]).optional(),
});

/**
 * Schema for control element definitions.
 * Validates physical controls (encoders, sliders, buttons) and their MIDI mappings.
 * Ensures MIDI CC numbers are within valid range (0-127) and channels are properly specified.
 */
const ControlDefinitionSchema = z.object({
  /** Unique identifier for this control within the map */
  id: z.string(),
  /** Human-readable name for the control */
  name: z.string(),
  /** Type of physical control */
  type: z.enum(['encoder', 'slider', 'button', 'button_group']),
  /** MIDI CC number (0-127) for the control */
  cc: z.number().min(0).max(127).optional(),
  /** MIDI channel override (string for registry reference or number) */
  channel: z.union([z.string(), z.number()]).optional(),
  /** Value range for the control [min, max] */
  range: z.array(z.number()).length(2).optional(),
  /** Description of the control's function */
  description: z.string().optional(),
  /** Button behavior mode (for button and button_group types) */
  mode: z.enum(['toggle', 'momentary']).optional(),
  /** Target plugin parameter (name or index) */
  plugin_parameter: z.union([z.string(), z.number()]).optional(),
  /** For button_group type: array of individual buttons */
  buttons: z.array(ButtonDefinitionSchema).optional(),
});

/**
 * Schema for MIDI map metadata used in organization and discovery.
 * Validates map information used by the registry system and management tools.
 */
const MapMetadataSchema = z.object({
  /** Human-readable name for the MIDI map */
  name: z.string(),
  /** Detailed description of the mapping's purpose */
  description: z.string().optional(),
  /** Author or creator of the mapping */
  author: z.string().optional(),
  /** Creation or last modification date (ISO date string recommended) */
  date: z.string().optional(),
  /** Tags for categorization and search */
  tags: z.array(z.string()).optional(),
});

/**
 * Schema for MIDI controller/device information.
 * Validates device identification used for compatibility checking.
 */
const DeviceDefinitionSchema = z.object({
  /** Device manufacturer name (e.g., 'Novation', 'Akai', 'Native Instruments') */
  manufacturer: z.string(),
  /** Specific device model (e.g., 'Launchkey MK3 49', 'MPK Mini MK3') */
  model: z.string(),
  /** Optional firmware version for compatibility notes */
  firmware: z.string().optional(),
});

/**
 * Schema for audio plugin/instrument information.
 * Validates plugin metadata for maps that target specific plugins.
 */
const PluginDefinitionSchema = z.object({
  /** Plugin manufacturer (e.g., 'Native Instruments', 'Arturia') */
  manufacturer: z.string(),
  /** Plugin name (e.g., 'Massive X', 'Pigments', 'Serum') */
  name: z.string(),
  /** Plugin version for compatibility tracking */
  version: z.string().optional(),
  /** Audio plugin format */
  format: z.enum(['VST', 'VST3', 'AU', 'AAX', 'LV2', 'CLAP']).optional(),
  /** Brief description of the plugin */
  description: z.string().optional(),
  /** Additional notes about compatibility or usage */
  notes: z.string().optional(),
});

/**
 * Main schema for canonical MIDI map validation.
 * This is the root schema that validates complete MIDI map configurations.
 *
 * Validates:
 * - MIDI channels are within valid range (1-16)
 * - All control definitions are properly structured
 * - Required fields are present
 * - Optional plugin and device information
 *
 * @example
 * ```typescript
 * const result = CanonicalMidiMapSchema.safeParse(mapData);
 * if (result.success) {
 *   // mapData is valid - result.data contains the validated map
 *   console.log('Valid map:', result.data.metadata.name);
 * } else {
 *   // Validation failed - result.error contains detailed errors
 *   result.error.errors.forEach(err => {
 *     console.error(`${err.path.join('.')}: ${err.message}`);
 *   });
 * }
 * ```
 */
export const CanonicalMidiMapSchema = z.object({
  /** Semantic version of the canonical format */
  version: z.string(),
  /** MIDI controller/device information */
  device: DeviceDefinitionSchema,
  /** Map metadata for organization and discovery */
  metadata: MapMetadataSchema,
  /** Optional plugin/instrument being controlled */
  plugin: PluginDefinitionSchema.optional(),
  /** Default MIDI channel (1-16) for all controls */
  midi_channel: z.number().min(1).max(16).optional(),
  /** Path to external MIDI channel registry file */
  midi_channel_registry: z.string().optional(),
  /** Array of control definitions (knobs, faders, buttons) */
  controls: z.array(ControlDefinitionSchema),
});

/**
 * Input type for canonical MIDI map validation.
 * Represents the raw data structure before validation and transformation.
 */
export type CanonicalMidiMapInput = z.input<typeof CanonicalMidiMapSchema>;

/**
 * Output type for canonical MIDI map validation.
 * Represents the validated and transformed data structure after successful validation.
 * This type should be used for all validated MIDI map data in the application.
 */
export type CanonicalMidiMapOutput = z.output<typeof CanonicalMidiMapSchema>;