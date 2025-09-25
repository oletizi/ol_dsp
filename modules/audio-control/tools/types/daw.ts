/**
 * DAW Data Contracts
 *
 * Core interfaces for DAW-specific map generation and format conversion
 * used across all tools in the workflow.
 */

import { CanonicalMidiMap } from './midi.js';

export interface DAWMap {
  /** Source canonical map */
  sourceMap: CanonicalMidiMap;

  /** Target DAW identifier */
  targetDAW: string;

  /** Generated map content (format-specific) */
  content: string;

  /** Output file name */
  fileName: string;

  /** Generation metadata */
  metadata: DAWMapMetadata;
}

export interface DAWMapMetadata {
  /** Generation timestamp */
  generated: string;

  /** Generator tool/version */
  generator: string;

  /** Target DAW version (if known) */
  dawVersion?: string;

  /** Conversion notes/warnings */
  notes?: string[];

  /** Feature compatibility info */
  compatibility?: DAWCompatibilityInfo;
}

export interface DAWCompatibilityInfo {
  /** Supported features */
  supportedFeatures: string[];

  /** Unsupported features */
  unsupportedFeatures?: string[];

  /** Feature limitations */
  limitations?: Record<string, string>;
}

export interface ArdourMap extends DAWMap {
  targetDAW: 'ardour';

  /** Ardour-specific binding information */
  bindings: ArdourBinding[];

  /** Device information for Ardour */
  deviceInfo?: ArdourDeviceInfo;
}

export interface ArdourBinding {
  /** MIDI channel (0-15) */
  channel: number;

  /** MIDI CC number */
  ctl?: number;

  /** MIDI note number */
  note?: number;

  /** Encoder CC (relative) */
  'enc-r'?: number;

  /** Ardour function name */
  function?: string;

  /** Plugin URI (for plugin control) */
  uri?: string;

  /** Ardour action name */
  action?: string;

  /** Encoder flag */
  encoder?: 'yes' | 'no';

  /** RPN number */
  rpn?: number;

  /** NRPN number */
  nrpn?: number;

  /** 14-bit RPN */
  rpn14?: number;

  /** 14-bit NRPN */
  nrpn14?: number;

  /** Threshold value */
  threshold?: number;

  /** Momentary flag */
  momentary?: 'yes' | 'no';
}

export interface ArdourDeviceInfo {
  /** Device name */
  'device-name': string;

  /** Device configuration */
  'device-info': {
    'bank-size'?: number;
    'motorized'?: 'yes' | 'no';
    'threshold'?: number;
    'has-master-fader'?: 'yes' | 'no';
    'has-lcd'?: 'yes' | 'no';
    'has-timecode'?: 'yes' | 'no';
    'has-meters'?: 'yes' | 'no';
    'uses-logic-control-buttons'?: 'yes' | 'no';
    'uses-mackie-control-buttons'?: 'yes' | 'no';
    'uses-ipmidi'?: 'yes' | 'no';
    'has-touch-sense-faders'?: 'yes' | 'no';
    'has-jog-wheel'?: 'yes' | 'no';
    'has-global-controls'?: 'yes' | 'no';
    'has-segmented-display'?: 'yes' | 'no';
  };
}

export interface ReaperMap extends DAWMap {
  targetDAW: 'reaper';

  /** Reaper-specific configuration */
  config?: ReaperMapConfig;
}

export interface ReaperMapConfig {
  /** Control surface name */
  surfaceName?: string;

  /** Device name */
  deviceName?: string;

  /** Number of tracks */
  trackCount?: number;

  /** Bank size */
  bankSize?: number;
}

export interface LogicProMap extends DAWMap {
  targetDAW: 'logic';

  /** Logic Pro specific configuration */
  config?: LogicMapConfig;
}

export interface LogicMapConfig {
  /** Control surface type */
  surfaceType?: string;

  /** Icon path */
  iconPath?: string;

  /** Model name */
  modelName?: string;
}

export interface DAWGenerationRequest {
  /** Source canonical map */
  canonicalMap: CanonicalMidiMap;

  /** Target DAW */
  targetDAW: 'ardour' | 'reaper' | 'logic' | 'cubase' | 'ableton';

  /** Generation options */
  options?: DAWGenerationOptions;
}

export interface DAWGenerationOptions {
  /** Output directory */
  outputDir?: string;

  /** Custom file name */
  fileName?: string;

  /** Include metadata comments */
  includeMetadata?: boolean;

  /** DAW-specific options */
  dawOptions?: Record<string, any>;
}

export interface DAWGenerationResult {
  /** Generation success */
  success: boolean;

  /** Generated DAW map */
  dawMap?: DAWMap;

  /** Output file path */
  outputPath?: string;

  /** Generation errors */
  errors?: string[];

  /** Generation warnings */
  warnings?: string[];

  /** Generation duration */
  duration?: number;
}