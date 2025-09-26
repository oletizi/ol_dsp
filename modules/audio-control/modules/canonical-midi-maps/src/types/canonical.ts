/**
 * Root interface for a canonical MIDI map configuration.
 * This is a DAW-agnostic format that can be converted to various DAW-specific formats.
 *
 * The canonical format supports:
 * - Multiple control surface types (encoders, sliders, buttons)
 * - Flexible plugin parameter mapping
 * - MIDI channel management and registry systems
 * - Rich metadata for organization and discovery
 *
 * @example
 * ```typescript
 * const map: CanonicalMidiMap = {
 *   version: '1.0.0',
 *   device: {
 *     manufacturer: 'Novation',
 *     model: 'Launchkey MK3 49'
 *   },
 *   metadata: {
 *     name: 'Launchkey MK3 → Massive X',
 *     description: 'Comprehensive mapping for Massive X synthesizer',
 *     author: 'Audio Team',
 *     tags: ['synthesizer', 'novation']
 *   },
 *   plugin: {
 *     manufacturer: 'Native Instruments',
 *     name: 'Massive X',
 *     format: 'VST3'
 *   },
 *   midi_channel: 1,
 *   controls: []
 * };
 * ```
 */
export interface CanonicalMidiMap {
  /** Semantic version of the canonical format */
  version: string;
  /** MIDI controller/device information */
  device: DeviceDefinition;
  /** Map metadata for organization and discovery */
  metadata: MapMetadata;
  /** Optional plugin/instrument being controlled */
  plugin?: PluginDefinition;
  /** Default MIDI channel (1-16) for all controls */
  midi_channel?: number;
  /** Path to external MIDI channel registry file */
  midi_channel_registry?: string;
  /** Array of control definitions (knobs, faders, buttons) */
  controls: ControlDefinition[];
}

/**
 * Metadata for organizing and discovering MIDI maps.
 * Used by the registry system and map management tools.
 *
 * @example
 * ```typescript
 * const metadata: MapMetadata = {
 *   name: 'Studio Controller Setup',
 *   description: 'Professional mapping for music production workflow',
 *   author: 'John Producer',
 *   date: '2024-01-15',
 *   tags: ['studio', 'production', 'mixing', 'synthesizer']
 * };
 * ```
 */
export interface MapMetadata {
  /** Human-readable name for the MIDI map */
  name: string;
  /** Detailed description of the mapping's purpose */
  description?: string;
  /** Author or creator of the mapping */
  author?: string;
  /** Creation or last modification date (ISO date string) */
  date?: string;
  /** Tags for categorization and search */
  tags?: string[];
}

/**
 * Defines the MIDI controller or device being mapped.
 * Used for device identification and compatibility checking.
 *
 * @example
 * ```typescript
 * const device: DeviceDefinition = {
 *   manufacturer: 'Akai',
 *   model: 'MPK Mini MK3',
 *   firmware: '1.0.5'
 * };
 * ```
 */
export interface DeviceDefinition {
  /** Device manufacturer name (e.g., 'Novation', 'Akai', 'Native Instruments') */
  manufacturer: string;
  /** Specific device model (e.g., 'Launchkey MK3 49', 'MPK Mini MK3') */
  model: string;
  /** Optional firmware version for compatibility notes */
  firmware?: string;
}

/**
 * Defines the audio plugin or instrument being controlled.
 * Optional - some maps may be for general DAW control rather than specific plugins.
 *
 * @example
 * ```typescript
 * const plugin: PluginDefinition = {
 *   manufacturer: 'Arturia',
 *   name: 'Pigments',
 *   version: '4.0.1',
 *   format: 'VST3',
 *   description: 'Wavetable synthesizer with advanced modulation',
 *   notes: 'Best used with firmware 1.2+ for full parameter access'
 * };
 * ```
 */
export interface PluginDefinition {
  /** Plugin manufacturer (e.g., 'Native Instruments', 'Arturia') */
  manufacturer: string;
  /** Plugin name (e.g., 'Massive X', 'Pigments', 'Serum') */
  name: string;
  /** Plugin version for compatibility tracking */
  version?: string;
  /** Audio plugin format */
  format?: 'VST' | 'VST3' | 'AU' | 'AAX' | 'LV2' | 'CLAP';
  /** Brief description of the plugin */
  description?: string;
  /** Additional notes about compatibility or usage */
  notes?: string;
}

/**
 * Defines a single control element and its MIDI mapping.
 * Supports various control types with flexible parameter mapping.
 *
 * @example
 * ```typescript
 * // Encoder control
 * const filterCutoff: ControlDefinition = {
 *   id: 'filter_cutoff',
 *   name: 'Filter Cutoff',
 *   type: 'encoder',
 *   cc: 20,
 *   channel: 1,
 *   range: [0, 127],
 *   description: 'Low-pass filter cutoff frequency',
 *   plugin_parameter: 'filter.cutoff'
 * };
 *
 * // Button control
 * const playButton: ControlDefinition = {
 *   id: 'transport_play',
 *   name: 'Play',
 *   type: 'button',
 *   cc: 60,
 *   channel: 1,
 *   mode: 'momentary',
 *   description: 'Start/stop transport'
 * };
 * ```
 */
export interface ControlDefinition {
  /** Unique identifier for this control within the map */
  id: string;
  /** Human-readable name for the control */
  name: string;
  /** Type of physical control */
  type: 'encoder' | 'slider' | 'button' | 'button_group';
  /** MIDI CC number (0-127) for the control */
  cc?: number;
  /** MIDI channel override (string for registry reference or number 1-16) */
  channel?: string | number;
  /** Value range for the control [min, max] */
  range?: number[];
  /** Description of the control's function */
  description?: string;
  /** Button behavior mode */
  mode?: 'toggle' | 'momentary';
  /** Target plugin parameter (name or index) */
  plugin_parameter?: string | number;
  /** For button_group type: array of individual buttons */
  buttons?: ButtonDefinition[];
}

/**
 * Defines an individual button within a button group.
 * Used for grouping related buttons like transport controls or preset selection.
 *
 * @example
 * ```typescript
 * const playButton: ButtonDefinition = {
 *   id: 'play',
 *   name: 'Play',
 *   cc: 60,
 *   channel: 1,
 *   mode: 'momentary',
 *   plugin_parameter: 'transport.play'
 * };
 * ```
 */
export interface ButtonDefinition {
  /** Unique identifier for this button within the group */
  id: string;
  /** Display name for the button */
  name: string;
  /** MIDI CC number for this button */
  cc: number;
  /** MIDI channel (string for registry reference or number 1-16) */
  channel: string | number;
  /** Button behavior: toggle (on/off) or momentary (press/release) */
  mode: 'toggle' | 'momentary';
  /** Target plugin parameter for this button */
  plugin_parameter?: string | number;
}

/**
 * Legacy interface for backwards compatibility during migration.
 * This interface is deprecated in favor of the new CanonicalMidiMap format.
 * Maintained for supporting existing map files during transition period.
 *
 * @deprecated Use CanonicalMidiMap format instead
 *
 * @example
 * ```typescript
 * const legacyMapping: LegacyMidiMapping = {
 *   id: 'filter-cutoff-mapping',
 *   description: 'Controls filter cutoff frequency',
 *   midiInput: {
 *     type: 'cc',
 *     channel: 1,
 *     number: 20,
 *     range: { min: 0, max: 127 }
 *   },
 *   pluginTarget: {
 *     type: 'parameter',
 *     identifier: 'filter.cutoff',
 *     range: { min: 20, max: 20000 }
 *   },
 *   enabled: true
 * };
 * ```
 */
export interface LegacyMidiMapping {
  /** Unique identifier for this mapping */
  id: string;
  /** Optional description of the mapping */
  description?: string;
  /** MIDI input configuration */
  midiInput: MidiInputDefinition;
  /** Plugin parameter target */
  pluginTarget: PluginTargetDefinition;
  /** Value mapping and scaling behavior */
  mapping?: MappingBehavior;
  /** Whether this mapping is currently active */
  enabled?: boolean;
}

/**
 * Defines MIDI input characteristics for legacy mappings.
 * Specifies what MIDI messages to listen for and how to interpret them.
 *
 * @example
 * ```typescript
 * // CC input with full range
 * const ccInput: MidiInputDefinition = {
 *   type: 'cc',
 *   channel: 1,
 *   number: 7, // Volume CC
 *   range: { min: 0, max: 127 },
 *   behavior: { mode: 'absolute' }
 * };
 *
 * // Pitchbend input with custom range
 * const pitchInput: MidiInputDefinition = {
 *   type: 'pitchbend',
 *   channel: 1,
 *   range: { min: -8192, max: 8191 },
 *   behavior: { mode: 'absolute', sensitivity: 0.5 }
 * };
 * ```
 */
export interface MidiInputDefinition {
  /** Type of MIDI message to handle */
  type: 'cc' | 'note' | 'pitchbend' | 'aftertouch' | 'program';
  /** MIDI channel to listen on (1-16, omit for all channels) */
  channel?: number;
  /** CC number, note number, or program number (depending on type) */
  number?: number;
  /** Expected input value range */
  range?: ValueRange;
  /** Input interpretation behavior */
  behavior?: InputBehavior;
}

/**
 * Defines the plugin parameter or control being targeted.
 * Specifies what aspect of the plugin to control and its characteristics.
 *
 * @example
 * ```typescript
 * // Standard parameter control
 * const filterParam: PluginTargetDefinition = {
 *   type: 'parameter',
 *   identifier: 'filter.cutoff',
 *   name: 'Low Pass Filter Cutoff',
 *   range: { min: 20, max: 20000, default: 1000 },
 *   units: 'Hz',
 *   category: 'Filter'
 * };
 *
 * // Bypass control
 * const bypassControl: PluginTargetDefinition = {
 *   type: 'bypass',
 *   identifier: 'main.bypass',
 *   name: 'Plugin Bypass'
 * };
 * ```
 */
export interface PluginTargetDefinition {
  /** Type of plugin control being targeted */
  type: 'parameter' | 'bypass' | 'preset' | 'macro';
  /** Unique identifier for the parameter (URI, name, or index) */
  identifier: string;
  /** Human-readable parameter name */
  name?: string;
  /** Parameter value range */
  range?: ValueRange;
  /** Parameter units (Hz, dB, %, etc.) */
  units?: string;
  /** Parameter category for organization */
  category?: string;
}

/**
 * Defines a numeric value range with optional default.
 * Used for MIDI values, plugin parameters, and scaling calculations.
 *
 * @example
 * ```typescript
 * // MIDI CC range
 * const midiRange: ValueRange = { min: 0, max: 127, default: 64 };
 *
 * // Audio frequency range
 * const freqRange: ValueRange = { min: 20, max: 20000, default: 1000 };
 *
 * // Decibel range
 * const dbRange: ValueRange = { min: -60, max: 12, default: 0 };
 * ```
 */
export interface ValueRange {
  /** Minimum value in the range */
  min: number;
  /** Maximum value in the range */
  max: number;
  /** Optional default value (used for initialization) */
  default?: number;
}

/**
 * Defines how MIDI input should be interpreted and processed.
 * Controls the relationship between physical control movement and parameter changes.
 *
 * @example
 * ```typescript
 * // Standard absolute control
 * const absoluteControl: InputBehavior = {
 *   mode: 'absolute',
 *   curve: 'linear',
 *   sensitivity: 1.0
 * };
 *
 * // Relative encoder with deadzone
 * const relativeEncoder: InputBehavior = {
 *   mode: 'relative',
 *   sensitivity: 0.5,
 *   deadzone: 2, // Ignore small movements
 *   curve: 'exponential'
 * };
 *
 * // Inverted logarithmic control
 * const invertedLog: InputBehavior = {
 *   mode: 'absolute',
 *   curve: 'logarithmic',
 *   invert: true
 * };
 * ```
 */
export interface InputBehavior {
  /** How input changes should be interpreted */
  mode?: 'absolute' | 'relative' | 'toggle' | 'momentary';
  /** Sensitivity multiplier (0.1 = less sensitive, 2.0 = more sensitive) */
  sensitivity?: number;
  /** Minimum change required to register input (reduces jitter) */
  deadzone?: number;
  /** Response curve for value mapping */
  curve?: 'linear' | 'exponential' | 'logarithmic' | 'custom';
  /** Whether to invert the control direction */
  invert?: boolean;
}

/**
 * Defines value mapping and processing behavior between MIDI input and plugin parameters.
 * Controls how raw MIDI values are transformed before being sent to the target parameter.
 *
 * @example
 * ```typescript
 * // Linear mapping with smoothing
 * const smoothLinear: MappingBehavior = {
 *   scaling: 'linear',
 *   smoothing: 0.8, // Heavy smoothing
 *   quantize: 1 // No quantization
 * };
 *
 * // Exponential curve for frequency parameters
 * const freqMapping: MappingBehavior = {
 *   scaling: 'exponential',
 *   bipolar: false,
 *   smoothing: 0.2
 * };
 *
 * // Custom curve mapping
 * const customCurve: MappingBehavior = {
 *   scaling: 'custom',
 *   curve: [0, 0.1, 0.3, 0.6, 1.0], // Custom response curve
 *   quantize: 0.01 // Quantize to 1% steps
 * };
 * ```
 */
export interface MappingBehavior {
  /** Value scaling algorithm */
  scaling?: 'linear' | 'exponential' | 'logarithmic' | 'custom';
  /** Custom curve points for 'custom' scaling (normalized 0-1) */
  curve?: number[];
  /** Quantization step size (0.01 = 1% steps, 1 = integer steps) */
  quantize?: number;
  /** Smoothing factor (0 = no smoothing, 0.9 = heavy smoothing) */
  smoothing?: number;
  /** Whether parameter has negative and positive ranges around center */
  bipolar?: boolean;
}

/**
 * Result of MIDI map validation process.
 * Contains validation status and any issues found during parsing or validation.
 *
 * @example
 * ```typescript
 * const result: ValidationResult = {
 *   valid: false,
 *   errors: [
 *     {
 *       path: 'controls[0].cc',
 *       message: 'CC number must be between 0 and 127',
 *       code: 'INVALID_CC_NUMBER'
 *     }
 *   ],
 *   warnings: [
 *     {
 *       path: 'metadata.description',
 *       message: 'Consider adding a description',
 *       code: 'MISSING_DESCRIPTION'
 *     }
 *   ]
 * };
 * ```
 */
export interface ValidationResult {
  /** Whether the MIDI map passed validation */
  valid: boolean;
  /** Array of validation errors (prevent usage) */
  errors: ValidationError[];
  /** Array of validation warnings (recommendations) */
  warnings: ValidationWarning[];
}

/**
 * Represents a validation error that prevents MIDI map usage.
 * Errors must be fixed before the map can be used.
 *
 * @example
 * ```typescript
 * const error: ValidationError = {
 *   path: 'controls[2].cc',
 *   message: 'CC number 200 is invalid. Must be 0-127.',
 *   code: 'INVALID_CC_NUMBER'
 * };
 * ```
 */
export interface ValidationError {
  /** JSON path to the problematic field */
  path: string;
  /** Human-readable error message */
  message: string;
  /** Error code for programmatic handling */
  code: string;
}

/**
 * Represents a validation warning for MIDI map best practices.
 * Warnings are recommendations but don't prevent usage.
 *
 * @example
 * ```typescript
 * const warning: ValidationWarning = {
 *   path: 'metadata.author',
 *   message: 'Consider adding an author for better tracking',
 *   code: 'MISSING_AUTHOR'
 * };
 * ```
 */
export interface ValidationWarning {
  /** JSON path to the field with the warning */
  path: string;
  /** Human-readable warning message */
  message: string;
  /** Warning code for programmatic handling */
  code: string;
}