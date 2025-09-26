/**
 * Represents a single MIDI binding in an Ardour MIDI map.
 * Bindings connect MIDI input events to Ardour functions or actions.
 *
 * Each binding must specify a MIDI channel and exactly one MIDI message type
 * (ctl, note, enc-r, rpn, nrpn, rpn14, or nrpn14).
 *
 * @example
 * ```typescript
 * // CC binding for track volume control
 * const volumeBinding: ArdourBinding = {
 *   channel: 1,
 *   ctl: 7, // Volume CC
 *   function: 'track-set-gain[1]' // Control track 1 gain
 * };
 *
 * // Note binding for transport stop
 * const stopBinding: ArdourBinding = {
 *   channel: 1,
 *   note: 64,
 *   function: 'transport-stop',
 *   momentary: 'yes'
 * };
 * ```
 */
export interface ArdourBinding {
  /** MIDI channel (1-16) */
  channel: number;
  /** Control Change number (0-127) - mutually exclusive with other message types */
  ctl?: number;
  /** Note number (0-127) - mutually exclusive with other message types */
  note?: number;
  /** Encoder relative mode CC number (0-127) - mutually exclusive with other message types */
  'enc-r'?: number;
  /** Ardour function to invoke (e.g., 'transport-stop', 'track-set-gain[1]') */
  function?: string;
  /** Plugin or processor URI for parameter control */
  uri?: string;
  /** Ardour action to invoke (alternative to function) */
  action?: string;
  /** Whether this is an encoder control ('yes' | 'no') */
  encoder?: 'yes' | 'no';
  /** Registered Parameter Number (0-16383) - for 7-bit RPN messages */
  rpn?: number;
  /** Non-Registered Parameter Number (0-16383) - for 7-bit NRPN messages */
  nrpn?: number;
  /** Registered Parameter Number (0-16383) - for 14-bit RPN messages */
  rpn14?: number;
  /** Non-Registered Parameter Number (0-16383) - for 14-bit NRPN messages */
  nrpn14?: number;
  /** Threshold value for trigger sensitivity (0-127) */
  threshold?: number;
  /** Whether button press is momentary ('yes') or toggle ('no') */
  momentary?: 'yes' | 'no';
}

/**
 * Groups related MIDI bindings with an optional comment for organization.
 * Binding groups help organize MIDI maps by function or device section,
 * making the generated XML more readable and maintainable.
 *
 * @example
 * ```typescript
 * // Group for controlling a specific plugin
 * const synthGroup: ArdourBindingGroup = {
 *   comment: 'MIDI Channel 1: Massive X Synthesizer Controls',
 *   bindings: [
 *     { channel: 1, ctl: 1, uri: 'plugin-param-uri-1' },
 *     { channel: 1, ctl: 2, uri: 'plugin-param-uri-2' }
 *   ]
 * };
 * ```
 */
export interface ArdourBindingGroup {
  /** Optional comment describing this group (appears as XML comment) */
  comment?: string;
  /** Array of MIDI bindings in this group */
  bindings: ArdourBinding[];
}

/**
 * Represents a complete Ardour MIDI map configuration.
 * This is the root structure that gets serialized to Ardour's XML format.
 *
 * The map can contain both individual bindings and organized binding groups.
 * All bindings from groups are flattened into the main bindings array during serialization.
 *
 * @example
 * ```typescript
 * const map: ArdourMidiMap = {
 *   name: 'My Controller Setup',
 *   version: '1.0.0',
 *   bindings: [
 *     { channel: 1, ctl: 7, function: 'track-set-gain[1]' }
 *   ],
 *   bindingGroups: [
 *     {
 *       comment: 'Transport Controls',
 *       bindings: [
 *         { channel: 1, note: 60, function: 'transport-stop', momentary: 'yes' },
 *         { channel: 1, note: 61, function: 'transport-roll', momentary: 'yes' }
 *       ]
 *     }
 *   ]
 * };
 * ```
 */
export interface ArdourMidiMap {
  /** Human-readable name for the MIDI map */
  name: string;
  /** Version string (defaults to '1.0.0' if not specified) */
  version?: string;
  /** Array of individual MIDI bindings */
  bindings: ArdourBinding[];
  /** Optional organized groups of bindings */
  bindingGroups?: ArdourBindingGroup[];
  /** Optional device information and capabilities */
  deviceInfo?: ArdourDeviceInfo;
}

/**
 * Device information and capabilities for Ardour control surfaces.
 * This information helps Ardour optimize its behavior for specific hardware capabilities.
 *
 * @example
 * ```typescript
 * const deviceInfo: ArdourDeviceInfo = {
 *   'device-name': 'Novation Launchkey MK3',
 *   'device-info': {
 *     'bank-size': 8,
 *     'motorized': 'no',
 *     'has-lcd': 'no',
 *     'has-timecode': 'no',
 *     'uses-logic-control-buttons': 'no',
 *     'has-touch-sense-faders': 'no'
 *   }
 * };
 * ```
 */
export interface ArdourDeviceInfo {
  /** Display name of the MIDI device */
  'device-name': string;
  /** Hardware capabilities and configuration */
  'device-info': {
    /** Number of channels/strips in a bank (for banking controllers) */
    'bank-size'?: number;
    /** Whether faders are motorized (can be moved by software) */
    'motorized'?: 'yes' | 'no';
    /** Global threshold value for button sensitivity */
    'threshold'?: number;
    /** Whether device has a dedicated master fader */
    'has-master-fader'?: 'yes' | 'no';
    /** Whether device has an LCD or OLED display */
    'has-lcd'?: 'yes' | 'no';
    /** Whether device can display timecode information */
    'has-timecode'?: 'yes' | 'no';
    /** Whether device has level meters display */
    'has-meters'?: 'yes' | 'no';
    /** Whether device uses Logic Control protocol buttons */
    'uses-logic-control-buttons'?: 'yes' | 'no';
    /** Whether device uses Mackie Control Universal buttons */
    'uses-mackie-control-buttons'?: 'yes' | 'no';
    /** Whether device supports IP-based MIDI communication */
    'uses-ipmidi'?: 'yes' | 'no';
    /** Whether faders have touch sensors */
    'has-touch-sense-faders'?: 'yes' | 'no';
    /** Whether device has a jog wheel for scrubbing */
    'has-jog-wheel'?: 'yes' | 'no';
    /** Whether device has global transport/control buttons */
    'has-global-controls'?: 'yes' | 'no';
    /** Whether device has segmented LED display */
    'has-segmented-display'?: 'yes' | 'no';
  };
}

/**
 * Predefined Ardour function names for MIDI bindings.
 * These functions control various aspects of Ardour's mixer, transport, and tracks.
 *
 * Functions that operate on specific tracks use array notation: 'track-set-gain[1]'
 * where the number in brackets specifies the track number (1-based).
 *
 * @example
 * ```typescript
 * // Use with specific track numbers
 * const trackFunctions: ArdourFunction[] = [
 *   'track-set-gain',    // + '[1]' for track 1 gain
 *   'track-set-pan',     // + '[2]' for track 2 pan
 *   'track-select'       // + '[3]' for selecting track 3
 * ];
 *
 * // Transport functions work globally
 * const transportFunctions: ArdourFunction[] = [
 *   'transport-start',
 *   'transport-stop',
 *   'toggle-roll'
 * ];
 * ```
 */
export type ArdourFunction =
  /** Set track gain/volume (append '[trackNum]' for specific track) */
  | 'track-set-gain'
  /** Set track pan position (append '[trackNum]' for specific track) */
  | 'track-set-pan'
  /** Set track mute state (append '[trackNum]' for specific track) */
  | 'track-set-mute'
  /** Set track solo state (append '[trackNum]' for specific track) */
  | 'track-set-solo'
  /** Set track record enable (append '[trackNum]' for specific track) */
  | 'track-set-rec-enable'
  /** Set track send gain (append '[trackNum]' for specific track) */
  | 'track-set-send-gain'
  /** Select track (append '[trackNum]' for specific track) */
  | 'track-select'
  /** Set track monitor state (append '[trackNum]' for specific track) */
  | 'track-set-monitor'
  /** Set track polarity/phase invert (append '[trackNum]' for specific track) */
  | 'track-set-polarity'
  /** Set track trim/gain adjustment (append '[trackNum]' for specific track) */
  | 'track-set-trim'
  /** Toggle global record enable */
  | 'toggle-rec-enable'
  /** Toggle track mute (append '[trackNum]' for specific track) */
  | 'toggle-track-mute'
  /** Toggle track solo (append '[trackNum]' for specific track) */
  | 'toggle-track-solo'
  /** Start transport (play from current position) */
  | 'transport-start'
  /** Stop transport */
  | 'transport-stop'
  /** Roll transport (start playing) */
  | 'transport-roll'
  /** Toggle transport roll/stop */
  | 'toggle-roll'
  /** Stop and return to last start position */
  | 'stop-forget'
  /** Set transport playback speed */
  | 'set-transport-speed'
  /** Switch to next bank of tracks */
  | 'next-bank'
  /** Switch to previous bank of tracks */
  | 'prev-bank'
  /** Jump by musical bars */
  | 'jump-by-bars'
  /** Jump by musical beats */
  | 'jump-by-beats'
  /** Jump by time in seconds */
  | 'jump-by-seconds';

/**
 * Represents a complete control surface definition for Ardour.
 * This is a higher-level abstraction that includes both device metadata
 * and the MIDI map configuration.
 *
 * @example
 * ```typescript
 * const controlSurface: ArdourControlSurface = {
 *   id: 'launchkey-mk3-setup',
 *   name: 'Novation Launchkey MK3 Studio Setup',
 *   description: 'Professional controller setup for music production',
 *   manufacturer: 'Novation',
 *   model: 'Launchkey MK3 49',
 *   deviceInfo: {
 *     'device-name': 'Launchkey MK3',
 *     'device-info': {
 *       'bank-size': 8,
 *       'motorized': 'no'
 *     }
 *   },
 *   midiMap: {
 *     name: 'Launchkey MK3 Map',
 *     bindings: []
 *   }
 * };
 * ```
 */
export interface ArdourControlSurface {
  /** Unique identifier for this control surface configuration */
  id: string;
  /** Human-readable name for the control surface */
  name: string;
  /** Optional detailed description of the setup */
  description?: string;
  /** Device manufacturer (e.g., 'Novation', 'Akai', 'Native Instruments') */
  manufacturer?: string;
  /** Specific device model (e.g., 'Launchkey MK3 49', 'MPK Mini MK3') */
  model?: string;
  /** Device capabilities and hardware information */
  deviceInfo?: ArdourDeviceInfo;
  /** The MIDI mapping configuration */
  midiMap: ArdourMidiMap;
}