import { expect } from 'chai';
import { midiNoteToNumber } from '@/lib-midi.js';

describe('lib-midi', () => {
    describe('midiNoteToNumber', () => {
        // Test basic notes without accidentals
        it('converts basic notes correctly', () => {
            expect(midiNoteToNumber('C4')).to.equal(60);
            expect(midiNoteToNumber('D4')).to.equal(62);
            expect(midiNoteToNumber('E4')).to.equal(64);
            expect(midiNoteToNumber('F4')).to.equal(65);
            expect(midiNoteToNumber('G4')).to.equal(67);
            expect(midiNoteToNumber('A4')).to.equal(69);
            expect(midiNoteToNumber('B4')).to.equal(71);
        });

        // Test notes with sharp accidentals
        it('handles sharp notes correctly', () => {
            expect(midiNoteToNumber('C#4')).to.equal(61);
            expect(midiNoteToNumber('F#3')).to.equal(54);
            expect(midiNoteToNumber('G#4')).to.equal(68);
        });

        // Test notes with flat accidentals
        it('handles flat notes correctly', () => {
            expect(midiNoteToNumber('Bb3')).to.equal(58);
            expect(midiNoteToNumber('Eb4')).to.equal(63);
            expect(midiNoteToNumber('Ab4')).to.equal(68);
        });

        // Test notes across different octaves
        it('handles different octaves correctly', () => {
            expect(midiNoteToNumber('C0')).to.equal(12);
            expect(midiNoteToNumber('C1')).to.equal(24);
            expect(midiNoteToNumber('C2')).to.equal(36);
            expect(midiNoteToNumber('C8')).to.equal(108);
        });

        // Test edge cases
        it('handles edge cases correctly', () => {
            // Lowest note (C-1 in MIDI is note 0)
            expect(midiNoteToNumber('C-1')).to.equal(0);
            // Highest note (G9 in MIDI is note 127)
            expect(midiNoteToNumber('G9')).to.equal(127);
        });

        // Test invalid inputs
        it('handles invalid inputs correctly', () => {
            // Empty string
            expect(midiNoteToNumber('')).to.equal(-1);
            // Invalid note names
            expect(midiNoteToNumber('H4')).to.equal(-1);
            // Invalid accidentals
            expect(midiNoteToNumber('C$4')).to.equal(-1);
            // Missing octave
            expect(midiNoteToNumber('C')).to.equal(-1);
            // Invalid format
            expect(midiNoteToNumber('4C')).to.equal(-1);
            // Invalid octave
            expect(midiNoteToNumber('C10')).to.equal(-1);
        });

        // Test case sensitivity
        it('handles case sensitivity correctly', () => {
            // Lowercase notes should work the same as uppercase
            expect(midiNoteToNumber('c4')).to.equal(60);
            expect(midiNoteToNumber('d#4')).to.equal(63);
            expect(midiNoteToNumber('f#3')).to.equal(54);
            expect(midiNoteToNumber('bb3')).to.equal(58);
        });
    });
});
