import type { ArdourBinding, ArdourMidiMap, ArdourFunction, ArdourDeviceInfo, ArdourBindingGroup } from '@/types/ardour.js';

/**
 * Configuration options for creating a new MidiMapBuilder instance.
 *
 * @example
 * ```typescript
 * const options: MidiMapBuilderOptions = {
 *   name: 'My Controller Setup',
 *   version: '2.0.0',
 *   deviceInfo: {
 *     'device-name': 'My MIDI Controller',
 *     'device-info': { 'bank-size': 8 }
 *   }
 * };
 * ```
 */
export interface MidiMapBuilderOptions {
  /** The name of the MIDI map */
  name: string;
  /** Optional version string (defaults to '1.0.0') */
  version?: string;
  /** Optional device capabilities information */
  deviceInfo?: ArdourDeviceInfo;
}

/**
 * Base options for creating MIDI bindings.
 * These options are common to all binding types (CC, Note, Encoder).
 *
 * @example
 * ```typescript
 * const baseOptions: BindingOptions = {
 *   channel: 1,
 *   function: 'transport-stop',
 *   momentary: true
 * };
 * ```
 */
export interface BindingOptions {
  /** MIDI channel (1-16) */
  channel: number;
  /** Ardour function to invoke (will be converted to function string) */
  function?: string;
  /** Plugin or processor URI for parameter control */
  uri?: string;
  /** Ardour action to invoke (alternative to function) */
  action?: string;
  /** Whether this is an encoder control */
  encoder?: boolean;
  /** Whether button press is momentary (true) or toggle (false) */
  momentary?: boolean;
  /** Threshold value for trigger sensitivity (0-127) */
  threshold?: number;
}

/**
 * Options for creating Control Change (CC) bindings.
 * Extends BindingOptions with CC-specific controller number.
 *
 * @example
 * ```typescript
 * const volumeBinding: CCBindingOptions = {
 *   channel: 1,
 *   controller: 7, // Channel Volume
 *   function: 'track-set-gain[1]'
 * };
 * ```
 */
export interface CCBindingOptions extends BindingOptions {
  /** MIDI CC controller number (0-127) */
  controller: number;
}

/**
 * Options for creating Note bindings.
 * Extends BindingOptions with note-specific note number.
 *
 * @example
 * ```typescript
 * const playButton: NoteBindingOptions = {
 *   channel: 1,
 *   note: 60, // Middle C
 *   function: 'transport-roll',
 *   momentary: true
 * };
 * ```
 */
export interface NoteBindingOptions extends BindingOptions {
  /** MIDI note number (0-127, where 60 = C4) */
  note: number;
}

/**
 * Options for creating encoder relative bindings.
 * These are used for endless encoders that send relative movement data.
 *
 * @example
 * ```typescript
 * const panEncoder: EncoderRelativeBindingOptions = {
 *   channel: 1,
 *   controller: 10, // Pan CC
 *   function: 'track-set-pan[1]'
 * };
 * ```
 */
export interface EncoderRelativeBindingOptions extends BindingOptions {
  /** MIDI CC controller number used for relative encoder data (0-127) */
  controller: number;
}

/**
 * Fluent builder for creating Ardour MIDI maps.
 * Provides a chainable API for building complex MIDI mappings with organized comments and grouping.
 *
 * The builder supports:
 * - Individual CC, Note, and Encoder bindings
 * - Grouped bindings with comments for better organization
 * - Pre-built control patterns (transport controls, channel strips)
 * - Validation and binding count tracking
 *
 * @example
 * ```typescript
 * const map = new MidiMapBuilder({ name: 'My Setup' })
 *   .addChannelComment(1, 'Transport Controls')
 *   .addNoteBinding({ channel: 1, note: 60, function: 'transport-stop', momentary: true })
 *   .addNoteBinding({ channel: 1, note: 61, function: 'transport-roll', momentary: true })
 *   .addChannelComment(2, 'Track Controls')
 *   .addChannelStripControls(2, 1, 20) // Channel 2, Track 1, starting at CC 20
 *   .build();
 * ```
 */
export class MidiMapBuilder {
  private bindings: ArdourBinding[] = [];
  private bindingGroups: ArdourBindingGroup[] = [];
  private currentGroup: ArdourBindingGroup | null = null;
  private readonly name: string;
  private readonly version: string | undefined;
  private readonly deviceInfo: ArdourDeviceInfo | undefined;

  /**
   * Creates a new MidiMapBuilder instance.
   *
   * @param options - Configuration for the MIDI map
   * @example
   * ```typescript
   * const builder = new MidiMapBuilder({
   *   name: 'Studio Controller',
   *   version: '1.2.0',
   *   deviceInfo: {
   *     'device-name': 'My Controller',
   *     'device-info': { 'bank-size': 8 }
   *   }
   * });
   * ```
   */
  constructor(options: MidiMapBuilderOptions) {
    this.name = options.name;
    this.version = options.version;
    this.deviceInfo = options.deviceInfo;
  }

  /**
   * Adds a comment to organize bindings by MIDI channel or function.
   * Creates a new binding group with the specified comment.
   * All subsequent bindings will be added to this group until a new comment is added.
   *
   * @param channel - MIDI channel number for the comment
   * @param pluginName - Description of what this channel controls
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * builder
   *   .addChannelComment(1, 'Massive X Synthesizer')
   *   .addCCBinding({ channel: 1, controller: 1, uri: 'massive-x-cutoff' })
   *   .addCCBinding({ channel: 1, controller: 2, uri: 'massive-x-resonance' });
   * ```
   */
  addChannelComment(channel: number, pluginName: string): this {
    // Finalize current group if exists
    if (this.currentGroup && this.currentGroup.bindings.length > 0) {
      this.bindingGroups.push(this.currentGroup);
    }

    // Start new group with comment
    this.currentGroup = {
      comment: `MIDI Channel ${channel}: ${pluginName}`,
      bindings: []
    };

    return this;
  }

  /**
   * Adds a Control Change (CC) binding to the MIDI map.
   * CC bindings are ideal for continuous controls like faders, knobs, and sliders.
   *
   * @param options - CC binding configuration
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * // Volume fader on channel 1
   * builder.addCCBinding({
   *   channel: 1,
   *   controller: 7, // Volume CC
   *   function: 'track-set-gain[1]'
   * });
   *
   * // Plugin parameter control
   * builder.addCCBinding({
   *   channel: 2,
   *   controller: 20,
   *   uri: 'plugin-parameter-uri',
   *   encoder: true
   * });
   * ```
   */
  addCCBinding(options: CCBindingOptions): this {
    const binding: ArdourBinding = {
      channel: options.channel,
      ctl: options.controller,
    };

    if (options.function !== undefined) binding.function = options.function;
    if (options.uri !== undefined) binding.uri = options.uri;
    if (options.action !== undefined) binding.action = options.action;
    if (options.encoder) binding.encoder = 'yes';
    if (options.momentary) binding.momentary = 'yes';
    if (options.threshold !== undefined) binding.threshold = options.threshold;

    // Add to current group if exists, otherwise add to general bindings
    if (this.currentGroup) {
      this.currentGroup.bindings.push(binding);
    } else {
      this.bindings.push(binding);
    }

    return this;
  }

  /**
   * Adds a Note binding to the MIDI map.
   * Note bindings are perfect for buttons, triggers, and on/off controls.
   *
   * @param options - Note binding configuration
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * // Momentary play button
   * builder.addNoteBinding({
   *   channel: 1,
   *   note: 60, // Middle C
   *   function: 'transport-roll',
   *   momentary: true
   * });
   *
   * // Toggle mute button for track 1
   * builder.addNoteBinding({
   *   channel: 1,
   *   note: 36, // C2
   *   function: 'toggle-track-mute[1]',
   *   momentary: false // Toggle behavior
   * });
   * ```
   */
  addNoteBinding(options: NoteBindingOptions): this {
    const binding: ArdourBinding = {
      channel: options.channel,
      note: options.note,
    };

    if (options.function !== undefined) binding.function = options.function;
    if (options.uri !== undefined) binding.uri = options.uri;
    if (options.action !== undefined) binding.action = options.action;
    if (options.momentary) binding.momentary = 'yes';
    if (options.threshold !== undefined) binding.threshold = options.threshold;

    // Add to current group if exists, otherwise add to general bindings
    if (this.currentGroup) {
      this.currentGroup.bindings.push(binding);
    } else {
      this.bindings.push(binding);
    }

    return this;
  }

  /**
   * Adds an encoder relative binding for endless rotary encoders.
   * These bindings handle relative movement data from encoders that don't have fixed start/end positions.
   *
   * @param options - Encoder relative binding configuration
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * // Endless encoder for pan control
   * builder.addEncoderRelativeBinding({
   *   channel: 1,
   *   controller: 10, // Pan CC
   *   function: 'track-set-pan[1]'
   * });
   *
   * // Plugin parameter with threshold
   * builder.addEncoderRelativeBinding({
   *   channel: 2,
   *   controller: 21,
   *   uri: 'plugin-filter-frequency',
   *   threshold: 5 // Reduce sensitivity
   * });
   * ```
   */
  addEncoderRelativeBinding(options: EncoderRelativeBindingOptions): this {
    const binding: ArdourBinding = {
      channel: options.channel,
      'enc-r': options.controller,
    };

    if (options.function !== undefined) binding.function = options.function;
    if (options.uri !== undefined) binding.uri = options.uri;
    if (options.action !== undefined) binding.action = options.action;
    if (options.threshold !== undefined) binding.threshold = options.threshold;

    // Add to current group if exists, otherwise add to general bindings
    if (this.currentGroup) {
      this.currentGroup.bindings.push(binding);
    } else {
      this.bindings.push(binding);
    }

    return this;
  }

  /**
   * Adds a standard set of transport control bindings.
   * Creates bindings for common transport functions: stop, play, record, toggle, and stop-forget.
   *
   * @param channel - MIDI channel to use for transport controls
   * @param startNote - Starting note number (will use consecutive notes)
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * // Add transport controls starting at note 60 (C4)
   * builder.addTransportControls(1, 60);
   * // Creates bindings for notes 60-64:
   * // 60: transport-stop
   * // 61: transport-roll
   * // 62: toggle-rec-enable
   * // 63: toggle-roll
   * // 64: stop-forget
   * ```
   */
  addTransportControls(channel: number, startNote: number): this {
    const transportFunctions: ArdourFunction[] = [
      'transport-stop',
      'transport-roll',
      'toggle-rec-enable',
      'toggle-roll',
      'stop-forget',
    ];

    transportFunctions.forEach((func, index) => {
      this.addNoteBinding({
        channel,
        note: startNote + index,
        function: func,
        momentary: true,
      });
    });

    return this;
  }

  /**
   * Adds a complete channel strip control set for a specific track.
   * Creates bindings for faders, knobs, and buttons typically found on each channel of a mixer.
   *
   * @param channel - MIDI channel to use
   * @param stripNumber - Track number (1-based) to control
   * @param baseCC - Base CC number (will use consecutive CCs/notes)
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * // Add complete controls for track 1 on MIDI channel 1, starting at CC 20
   * builder.addChannelStripControls(1, 1, 20);
   * // Creates:
   * // CC 20: track-set-gain[1] (volume fader)
   * // CC 21: track-set-pan[1] (pan knob)
   * // CC 22: track-set-send-gain[1] (send level)
   * // Note 20: toggle-track-mute[1] (mute button)
   * // Note 21: toggle-track-solo[1] (solo button)
   * // Note 22: toggle-rec-enable[1] (record button)
   * // Note 23: track-select[1] (select button, momentary)
   * ```
   */
  addChannelStripControls(channel: number, stripNumber: number, baseCC: number): this {
    const stripControls = [
      { cc: baseCC, func: 'track-set-gain' as ArdourFunction },
      { cc: baseCC + 1, func: 'track-set-pan' as ArdourFunction },
      { cc: baseCC + 2, func: 'track-set-send-gain' as ArdourFunction },
    ];

    stripControls.forEach(({ cc, func }) => {
      this.addCCBinding({
        channel,
        controller: cc,
        function: `${func}[${stripNumber}]`,
      });
    });

    const buttonControls = [
      { note: baseCC, func: 'toggle-track-mute' as ArdourFunction },
      { note: baseCC + 1, func: 'toggle-track-solo' as ArdourFunction },
      { note: baseCC + 2, func: 'toggle-rec-enable' as ArdourFunction },
      { note: baseCC + 3, func: 'track-select' as ArdourFunction },
    ];

    buttonControls.forEach(({ note, func }) => {
      this.addNoteBinding({
        channel,
        note,
        function: `${func}[${stripNumber}]`,
        momentary: func === 'track-select',
      });
    });

    return this;
  }

  /**
   * Builds and returns the complete ArdourMidiMap.
   * Finalizes any pending binding groups and flattens all bindings into the final structure.
   *
   * @returns The complete MIDI map ready for serialization
   *
   * @example
   * ```typescript
   * const map = builder
   *   .addCCBinding({ channel: 1, controller: 7, function: 'track-set-gain[1]' })
   *   .addNoteBinding({ channel: 1, note: 60, function: 'transport-stop' })
   *   .build();
   *
   * // Use with serializer
   * const serializer = new ArdourXMLSerializer();
   * const xml = serializer.serializeMidiMap(map);
   * ```
   */
  build(): ArdourMidiMap {
    // Finalize current group if exists
    if (this.currentGroup && this.currentGroup.bindings.length > 0) {
      this.bindingGroups.push(this.currentGroup);
    }

    // Flatten all bindings for the final map
    const allBindings = [
      ...this.bindings,
      ...this.bindingGroups.flatMap(group => group.bindings)
    ];

    const map: ArdourMidiMap = {
      name: this.name,
      bindings: allBindings,
    };

    if (this.bindingGroups.length > 0) {
      map.bindingGroups = [...this.bindingGroups];
    }

    if (this.version !== undefined) {
      map.version = this.version;
    }

    if (this.deviceInfo !== undefined) {
      map.deviceInfo = this.deviceInfo;
    }

    return map;
  }

  /**
   * Clears all bindings and binding groups, resetting the builder to empty state.
   * Useful for reusing the same builder instance for multiple maps.
   *
   * @returns The builder instance for method chaining
   *
   * @example
   * ```typescript
   * const builder = new MidiMapBuilder({ name: 'Test' });
   *
   * // Build first map
   * const map1 = builder.addCCBinding(...).build();
   *
   * // Clear and build second map
   * const map2 = builder.clear().addNoteBinding(...).build();
   * ```
   */
  clear(): this {
    this.bindings = [];
    this.bindingGroups = [];
    this.currentGroup = null;
    return this;
  }

  /**
   * Gets the total number of bindings currently in the builder.
   * Includes bindings from both individual bindings and binding groups.
   *
   * @returns Total count of MIDI bindings
   *
   * @example
   * ```typescript
   * const builder = new MidiMapBuilder({ name: 'Test' })
   *   .addCCBinding({ channel: 1, controller: 7, function: 'track-set-gain[1]' })
   *   .addTransportControls(1, 60); // Adds 5 more bindings
   *
   * console.log(builder.getBindingCount()); // Outputs: 6
   * ```
   */
  getBindingCount(): number {
    return this.bindings.length + this.bindingGroups.reduce((total, group) => total + group.bindings.length, 0);
  }
}