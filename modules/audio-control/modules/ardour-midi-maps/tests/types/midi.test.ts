/**
 * Tests for MIDI type definitions and interfaces
 */

import { describe, it, expect } from 'vitest';
import type {
  MidiChannel,
  MidiNote,
  MidiCC,
  MidiMessage,
  MidiNoteMessage,
  MidiCCMessage,
  MidiProgramChangeMessage,
  MidiPitchBendMessage,
  MidiEventType,
} from '@/types/midi.js';

describe('MIDI Types', () => {
  describe('Basic MIDI Interfaces', () => {
    it('should define MidiChannel interface correctly', () => {
      const channel: MidiChannel = { channel: 1 };
      expect(channel.channel).toBe(1);
    });

    it('should define MidiNote interface correctly', () => {
      const note: MidiNote = { note: 60 };
      expect(note.note).toBe(60);
      expect(note.velocity).toBeUndefined();

      const noteWithVelocity: MidiNote = { note: 60, velocity: 127 };
      expect(noteWithVelocity.velocity).toBe(127);
    });

    it('should define MidiCC interface correctly', () => {
      const cc: MidiCC = { controller: 1 };
      expect(cc.controller).toBe(1);
      expect(cc.value).toBeUndefined();

      const ccWithValue: MidiCC = { controller: 1, value: 64 };
      expect(ccWithValue.value).toBe(64);
    });
  });

  describe('MIDI Message Types', () => {
    it('should define MidiNoteMessage correctly', () => {
      const noteMessage: MidiNoteMessage = {
        type: 'note',
        channel: 1,
        note: 60,
        velocity: 100,
      };

      expect(noteMessage.type).toBe('note');
      expect(noteMessage.channel).toBe(1);
      expect(noteMessage.note).toBe(60);
      expect(noteMessage.velocity).toBe(100);
    });

    it('should define MidiCCMessage correctly', () => {
      const ccMessage: MidiCCMessage = {
        type: 'cc',
        channel: 1,
        controller: 7,
        value: 100,
      };

      expect(ccMessage.type).toBe('cc');
      expect(ccMessage.channel).toBe(1);
      expect(ccMessage.controller).toBe(7);
      expect(ccMessage.value).toBe(100);
    });

    it('should define MidiProgramChangeMessage correctly', () => {
      const programMessage: MidiProgramChangeMessage = {
        type: 'program-change',
        channel: 1,
        program: 5,
      };

      expect(programMessage.type).toBe('program-change');
      expect(programMessage.channel).toBe(1);
      expect(programMessage.program).toBe(5);
    });

    it('should define MidiPitchBendMessage correctly', () => {
      const pitchBendMessage: MidiPitchBendMessage = {
        type: 'pitch-bend',
        channel: 1,
        value: 8192,
      };

      expect(pitchBendMessage.type).toBe('pitch-bend');
      expect(pitchBendMessage.channel).toBe(1);
      expect(pitchBendMessage.value).toBe(8192);
    });
  });

  describe('Union Type MidiEventType', () => {
    it('should accept all MIDI message types', () => {
      const messages: MidiEventType[] = [
        {
          type: 'note',
          channel: 1,
          note: 60,
          velocity: 100,
        },
        {
          type: 'cc',
          channel: 2,
          controller: 1,
          value: 64,
        },
        {
          type: 'program-change',
          channel: 3,
          program: 10,
        },
        {
          type: 'pitch-bend',
          channel: 4,
          value: 4096,
        },
      ];

      expect(messages).toHaveLength(4);
      expect(messages[0]?.type).toBe('note');
      expect(messages[1]?.type).toBe('cc');
      expect(messages[2]?.type).toBe('program-change');
      expect(messages[3]?.type).toBe('pitch-bend');
    });

    it('should allow type discrimination', () => {
      const message: MidiEventType = {
        type: 'cc',
        channel: 1,
        controller: 7,
        value: 100,
      };

      if (message.type === 'cc') {
        expect(message.controller).toBe(7);
        expect(message.value).toBe(100);
      } else {
        throw new Error('Type discrimination failed');
      }
    });
  });

  describe('Type Safety', () => {
    it('should enforce required properties', () => {
      // These should compile correctly
      const validNote: MidiNoteMessage = {
        type: 'note',
        channel: 1,
        note: 60,
        velocity: 100,
      };

      const validCC: MidiCCMessage = {
        type: 'cc',
        channel: 1,
        controller: 1,
        value: 64,
      };

      expect(validNote).toBeDefined();
      expect(validCC).toBeDefined();

      // TypeScript would prevent these invalid constructions at compile time:
      // const invalidNote: MidiNoteMessage = { type: 'note', channel: 1 }; // Missing note and velocity
      // const invalidCC: MidiCCMessage = { type: 'cc', channel: 1 }; // Missing controller and value
    });

    it('should enforce correct message types', () => {
      // This should compile correctly
      const message: MidiEventType = {
        type: 'note',
        channel: 1,
        note: 60,
        velocity: 100,
      };

      expect(message.type).toBe('note');

      // TypeScript would prevent this invalid construction:
      // const invalidMessage: MidiEventType = { type: 'invalid', channel: 1 };
    });
  });

  describe('MIDI Value Ranges', () => {
    it('should handle typical MIDI value ranges', () => {
      // Note numbers (0-127)
      const lowNote: MidiNote = { note: 0 };
      const highNote: MidiNote = { note: 127 };
      expect(lowNote.note).toBe(0);
      expect(highNote.note).toBe(127);

      // Velocity values (0-127)
      const softNote: MidiNote = { note: 60, velocity: 1 };
      const loudNote: MidiNote = { note: 60, velocity: 127 };
      expect(softNote.velocity).toBe(1);
      expect(loudNote.velocity).toBe(127);

      // CC controller numbers (0-127)
      const ccMin: MidiCC = { controller: 0, value: 0 };
      const ccMax: MidiCC = { controller: 127, value: 127 };
      expect(ccMin.controller).toBe(0);
      expect(ccMax.controller).toBe(127);

      // Pitch bend values (0-16383, center is 8192)
      const pitchBendCenter: MidiPitchBendMessage = {
        type: 'pitch-bend',
        channel: 1,
        value: 8192,
      };
      expect(pitchBendCenter.value).toBe(8192);
    });

    it('should handle channel ranges', () => {
      // MIDI channels 1-16
      const channel1: MidiChannel = { channel: 1 };
      const channel16: MidiChannel = { channel: 16 };
      expect(channel1.channel).toBe(1);
      expect(channel16.channel).toBe(16);
    });
  });
});