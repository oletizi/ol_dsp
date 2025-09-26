/**
 * Represents a MIDI channel specification.
 * MIDI channels are numbered 1-16 in the specification.
 *
 * @example
 * ```typescript
 * const channel: MidiChannel = { channel: 1 }; // MIDI Channel 1
 * ```
 */
export interface MidiChannel {
  /** MIDI channel number (1-16) */
  channel: number;
}

/**
 * Represents a MIDI note message with note number and optional velocity.
 * Note numbers follow the MIDI specification: C4 = 60, range 0-127.
 *
 * @example
 * ```typescript
 * // Middle C with full velocity
 * const middleC: MidiNote = { note: 60, velocity: 127 };
 *
 * // Note without specific velocity (uses default)
 * const noteOn: MidiNote = { note: 64 }; // E4
 * ```
 */
export interface MidiNote {
  /** MIDI note number (0-127, where 60 = C4) */
  note: number;
  /** Note velocity (0-127). If omitted, uses default velocity */
  velocity?: number;
}

/**
 * Represents a MIDI Control Change (CC) message.
 * CC numbers 0-127 control various parameters. Common controllers:
 * - 1: Modulation Wheel
 * - 7: Channel Volume
 * - 10: Pan Position
 * - 64: Sustain Pedal
 *
 * @example
 * ```typescript
 * // Volume control at 75% (96/127)
 * const volume: MidiCC = { controller: 7, value: 96 };
 *
 * // Modulation wheel without specific value
 * const modWheel: MidiCC = { controller: 1 };
 * ```
 */
export interface MidiCC {
  /** MIDI CC controller number (0-127) */
  controller: number;
  /** CC value (0-127). If omitted, uses default or current value */
  value?: number;
}

/**
 * Base interface for all MIDI message types.
 * All MIDI messages include a type discriminator and channel number.
 * This serves as the foundation for more specific message types.
 *
 * @example
 * ```typescript
 * // This is typically used as a base for other message types
 * const baseMessage: MidiMessage = {
 *   type: 'cc',
 *   channel: 1
 * };
 * ```
 */
export interface MidiMessage {
  /** The type of MIDI message */
  type: 'note' | 'cc' | 'program-change' | 'pitch-bend';
  /** MIDI channel number (1-16) */
  channel: number;
}

/**
 * Represents a complete MIDI Note On/Off message with channel, note, and velocity.
 * Used for triggering sounds, activating buttons, or sending note-based control data.
 *
 * @example
 * ```typescript
 * // Note On message for middle C at full velocity on channel 1
 * const noteOn: MidiNoteMessage = {
 *   type: 'note',
 *   channel: 1,
 *   note: 60, // C4
 *   velocity: 127
 * };
 *
 * // Note Off (velocity 0)
 * const noteOff: MidiNoteMessage = {
 *   type: 'note',
 *   channel: 1,
 *   note: 60,
 *   velocity: 0
 * };
 * ```
 */
export interface MidiNoteMessage extends MidiMessage {
  /** Always 'note' for note messages */
  type: 'note';
  /** MIDI note number (0-127, where 60 = C4) */
  note: number;
  /** Note velocity (0-127, where 0 = Note Off) */
  velocity: number;
}

/**
 * Represents a MIDI Control Change message for continuous parameter control.
 * CC messages are the most common way to control plugin parameters, mixer faders,
 * and other continuous values in audio applications.
 *
 * @example
 * ```typescript
 * // Set channel volume to 75% on channel 1
 * const volumeControl: MidiCCMessage = {
 *   type: 'cc',
 *   channel: 1,
 *   controller: 7, // Channel Volume
 *   value: 96 // ~75% of 127
 * };
 *
 * // Pan fully left
 * const panLeft: MidiCCMessage = {
 *   type: 'cc',
 *   channel: 1,
 *   controller: 10, // Pan Position
 *   value: 0 // Full left
 * };
 * ```
 */
export interface MidiCCMessage extends MidiMessage {
  /** Always 'cc' for Control Change messages */
  type: 'cc';
  /** MIDI CC controller number (0-127) */
  controller: number;
  /** CC value (0-127) */
  value: number;
}

/**
 * Represents a MIDI Program Change message for selecting presets or patches.
 * Program Change messages switch between different sounds or presets on a device.
 *
 * @example
 * ```typescript
 * // Select program 1 (preset 1) on channel 1
 * const selectPreset: MidiProgramChangeMessage = {
 *   type: 'program-change',
 *   channel: 1,
 *   program: 0 // Program numbers are 0-127, often displayed as 1-128
 * };
 * ```
 */
export interface MidiProgramChangeMessage extends MidiMessage {
  /** Always 'program-change' for Program Change messages */
  type: 'program-change';
  /** Program number (0-127, often displayed as 1-128 in UI) */
  program: number;
}

/**
 * Represents a MIDI Pitch Bend message for smooth pitch modulation.
 * Pitch bend provides 14-bit resolution (0-16383) with 8192 as center (no bend).
 *
 * @example
 * ```typescript
 * // No pitch bend (center position)
 * const noBend: MidiPitchBendMessage = {
 *   type: 'pitch-bend',
 *   channel: 1,
 *   value: 8192 // Center position
 * };
 *
 * // Maximum upward pitch bend
 * const bendUp: MidiPitchBendMessage = {
 *   type: 'pitch-bend',
 *   channel: 1,
 *   value: 16383 // Maximum upward
 * };
 *
 * // Maximum downward pitch bend
 * const bendDown: MidiPitchBendMessage = {
 *   type: 'pitch-bend',
 *   channel: 1,
 *   value: 0 // Maximum downward
 * };
 * ```
 */
export interface MidiPitchBendMessage extends MidiMessage {
  /** Always 'pitch-bend' for Pitch Bend messages */
  type: 'pitch-bend';
  /** 14-bit pitch bend value (0-16383, where 8192 = no bend) */
  value: number;
}

/**
 * Union type representing all possible MIDI message types.
 * This discriminated union allows type-safe handling of different MIDI message types
 * using the 'type' property as a discriminator.
 *
 * @example
 * ```typescript
 * function handleMidiEvent(event: MidiEventType) {
 *   switch (event.type) {
 *     case 'note':
 *       console.log(`Note ${event.note} with velocity ${event.velocity}`);
 *       break;
 *     case 'cc':
 *       console.log(`CC ${event.controller} set to ${event.value}`);
 *       break;
 *     case 'program-change':
 *       console.log(`Program changed to ${event.program}`);
 *       break;
 *     case 'pitch-bend':
 *       console.log(`Pitch bend: ${event.value}`);
 *       break;
 *   }
 * }
 * ```
 */
export type MidiEventType = MidiNoteMessage | MidiCCMessage | MidiProgramChangeMessage | MidiPitchBendMessage;