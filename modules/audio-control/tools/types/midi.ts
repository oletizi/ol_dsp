/**
 * MIDI Data Contracts
 *
 * Core interfaces for MIDI controls, mappings, and canonical map format
 * used across all tools in the workflow.
 */

export interface MidiControl {
  /** Unique control identifier */
  id: string;

  /** Human-readable control name */
  name: string;

  /** Control type */
  type: 'encoder' | 'slider' | 'button' | 'button_group' | 'knob' | 'fader';

  /** MIDI CC number */
  cc?: number;

  /** MIDI note number (for buttons) */
  note?: number;

  /** MIDI channel (1-16) */
  channel: number;

  /** Control value range */
  range?: [number, number];

  /** Control behavior mode */
  mode?: 'toggle' | 'momentary' | 'absolute' | 'relative';

  /** Control description */
  description?: string;

  /** For button groups, child buttons */
  buttons?: MidiControl[];
}

export interface CanonicalMidiMap {
  /** Map version */
  version: string;

  /** Device information */
  device: DeviceInfo;

  /** Map metadata */
  metadata: MapMetadata;

  /** Plugin information (if mapped to specific plugin) */
  plugin?: PluginInfo;

  /** Default MIDI channel */
  defaultChannel?: number;

  /** MIDI channel registry path */
  channelRegistryPath?: string;

  /** All MIDI controls */
  controls: MidiControl[];

  /** Control mappings to plugin parameters */
  mappings?: ControlMapping[];
}

export interface DeviceInfo {
  /** Device manufacturer */
  manufacturer: string;

  /** Device model */
  model: string;

  /** Device firmware version */
  firmware?: string;

  /** Device identifier/serial */
  deviceId?: string;
}

export interface MapMetadata {
  /** Map name */
  name: string;

  /** Map description */
  description?: string;

  /** Map author */
  author?: string;

  /** Creation date */
  created?: string;

  /** Last update date */
  updated?: string;

  /** Map tags */
  tags?: string[];

  /** Map version */
  version?: string;
}

export interface ControlMapping {
  /** Control ID */
  controlId: string;

  /** Plugin parameter index */
  parameterIndex: number;

  /** Plugin parameter name (for reference) */
  parameterName?: string;

  /** Value scaling behavior */
  scaling?: 'linear' | 'exponential' | 'logarithmic' | 'custom';

  /** Custom scaling curve */
  curve?: number[];

  /** Value quantization steps */
  quantize?: number;

  /** Value smoothing factor */
  smoothing?: number;

  /** Bipolar value handling */
  bipolar?: boolean;

  /** Value inversion */
  invert?: boolean;
}

export interface MidiMessage {
  /** Message type */
  type: 'note' | 'cc' | 'program_change' | 'pitch_bend' | 'aftertouch';

  /** MIDI channel (0-15) */
  channel: number;

  /** Timestamp (if available) */
  timestamp?: number;
}

export interface MidiNoteMessage extends MidiMessage {
  type: 'note';
  note: number;
  velocity: number;
  noteOn: boolean;
}

export interface MidiCCMessage extends MidiMessage {
  type: 'cc';
  controller: number;
  value: number;
}

export interface MidiProgramChangeMessage extends MidiMessage {
  type: 'program_change';
  program: number;
}

export interface MidiPitchBendMessage extends MidiMessage {
  type: 'pitch_bend';
  value: number; // -8192 to 8191
}