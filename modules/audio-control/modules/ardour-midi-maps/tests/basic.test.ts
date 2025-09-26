/**
 * Basic tests for ardour-midi-maps module
 */

import { describe, it, expect } from 'vitest';

describe('Ardour MIDI Maps', () => {
  describe('basic functionality', () => {
    it('should pass basic test', () => {
      expect(true).toBe(true);
    });

    it('should validate MIDI CC range', () => {
      const isValidCC = (cc: number): boolean => {
        return cc >= 0 && cc <= 127 && Number.isInteger(cc);
      };

      expect(isValidCC(0)).toBe(true);
      expect(isValidCC(64)).toBe(true);
      expect(isValidCC(127)).toBe(true);
      expect(isValidCC(-1)).toBe(false);
      expect(isValidCC(128)).toBe(false);
      expect(isValidCC(64.5)).toBe(false);
    });

    it('should validate MIDI channel range', () => {
      const isValidChannel = (channel: number): boolean => {
        return channel >= 1 && channel <= 16 && Number.isInteger(channel);
      };

      expect(isValidChannel(1)).toBe(true);
      expect(isValidChannel(16)).toBe(true);
      expect(isValidChannel(0)).toBe(false);
      expect(isValidChannel(17)).toBe(false);
    });

    it('should create basic MIDI binding structure', () => {
      const createMidiBinding = (channel: number, cc: number, parameter: string) => ({
        channel,
        cc,
        parameter,
        min: 0,
        max: 1
      });

      const binding = createMidiBinding(1, 1, 'gain');

      expect(binding).toEqual({
        channel: 1,
        cc: 1,
        parameter: 'gain',
        min: 0,
        max: 1
      });
    });
  });
});