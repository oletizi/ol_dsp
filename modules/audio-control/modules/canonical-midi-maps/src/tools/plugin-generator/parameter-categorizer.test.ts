/**
 * Test suite for parameter categorizer functionality
 */

import { describe, expect, it } from 'vitest';
import { ParameterCategorizer, createParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

describe('ParameterCategorizer', () => {
  let categorizer: ParameterCategorizer;

  beforeEach(() => {
    categorizer = new ParameterCategorizer();
  });

  describe('envelope parameters', () => {
    it('should categorize basic envelope parameters', () => {
      expect(categorizer.categorizeParameter('Attack')).toBe('envelope');
      expect(categorizer.categorizeParameter('Decay')).toBe('envelope');
      expect(categorizer.categorizeParameter('Sustain')).toBe('envelope');
      expect(categorizer.categorizeParameter('Release')).toBe('envelope');
      expect(categorizer.categorizeParameter('ADSR')).toBe('envelope');
      expect(categorizer.categorizeParameter('Envelope')).toBe('envelope');
    });

    it('should categorize compound envelope parameters', () => {
      expect(categorizer.categorizeParameter('Filter Env Attack')).toBe('envelope');
      expect(categorizer.categorizeParameter('Amp Envelope Decay')).toBe('envelope');
      expect(categorizer.categorizeParameter('VCA ENV Sustain')).toBe('envelope');
      expect(categorizer.categorizeParameter('LFO Env Release')).toBe('envelope');
    });

    it('should handle case insensitive envelope parameters', () => {
      expect(categorizer.categorizeParameter('ATTACK')).toBe('envelope');
      expect(categorizer.categorizeParameter('decay')).toBe('envelope');
      expect(categorizer.categorizeParameter('Sustain')).toBe('envelope');
      expect(categorizer.categorizeParameter('rElEaSe')).toBe('envelope');
    });
  });

  describe('filter parameters', () => {
    it('should categorize basic filter parameters', () => {
      expect(categorizer.categorizeParameter('Filter')).toBe('filter');
      expect(categorizer.categorizeParameter('VCF')).toBe('filter');
      expect(categorizer.categorizeParameter('Cutoff')).toBe('filter');
      expect(categorizer.categorizeParameter('Resonance')).toBe('filter');
      expect(categorizer.categorizeParameter('Frequency')).toBe('filter');
      expect(categorizer.categorizeParameter('LP Cutoff')).toBe('filter');
    });

    it('should categorize Q parameter correctly', () => {
      expect(categorizer.categorizeParameter('Q')).toBe('filter');
      expect(categorizer.categorizeParameter('Filter Q')).toBe('filter');
      expect(categorizer.categorizeParameter('VCF Q')).toBe('filter');
    });

    it('should handle compound filter parameters', () => {
      expect(categorizer.categorizeParameter('LP Filter Cutoff')).toBe('filter');
      expect(categorizer.categorizeParameter('High Pass Resonance')).toBe('filter');
      expect(categorizer.categorizeParameter('Filter Frequency')).toBe('filter');
    });
  });

  describe('oscillator parameters', () => {
    it('should categorize basic oscillator parameters', () => {
      expect(categorizer.categorizeParameter('Oscillator')).toBe('oscillator');
      expect(categorizer.categorizeParameter('OSC')).toBe('oscillator');
      expect(categorizer.categorizeParameter('VCO')).toBe('oscillator');
      expect(categorizer.categorizeParameter('Wave')).toBe('oscillator');
      expect(categorizer.categorizeParameter('Pitch')).toBe('oscillator');
      expect(categorizer.categorizeParameter('Detune')).toBe('oscillator');
      expect(categorizer.categorizeParameter('Sync')).toBe('oscillator');
    });

    it('should handle compound oscillator parameters', () => {
      expect(categorizer.categorizeParameter('OSC 1 Wave')).toBe('oscillator');
      expect(categorizer.categorizeParameter('VCO Pitch')).toBe('oscillator');
      expect(categorizer.categorizeParameter('Oscillator Detune')).toBe('oscillator');
    });
  });

  describe('LFO parameters', () => {
    it('should categorize basic LFO parameters', () => {
      expect(categorizer.categorizeParameter('LFO')).toBe('lfo');
      expect(categorizer.categorizeParameter('Modulation')).toBe('lfo');
      expect(categorizer.categorizeParameter('Rate')).toBe('lfo');
      expect(categorizer.categorizeParameter('Depth')).toBe('lfo');
      expect(categorizer.categorizeParameter('Speed')).toBe('lfo');
    });

    it('should handle compound LFO parameters', () => {
      expect(categorizer.categorizeParameter('LFO 1 Rate')).toBe('lfo');
      expect(categorizer.categorizeParameter('Mod Depth')).toBe('lfo');
      expect(categorizer.categorizeParameter('LFO Speed')).toBe('lfo');
    });
  });

  describe('effects parameters', () => {
    it('should categorize basic effects parameters', () => {
      expect(categorizer.categorizeParameter('Chorus')).toBe('effects');
      expect(categorizer.categorizeParameter('Delay')).toBe('effects');
      expect(categorizer.categorizeParameter('Reverb')).toBe('effects');
      expect(categorizer.categorizeParameter('Phaser')).toBe('effects');
      expect(categorizer.categorizeParameter('Flanger')).toBe('effects');
      expect(categorizer.categorizeParameter('Distortion')).toBe('effects');
    });

    it('should handle compound effects parameters', () => {
      expect(categorizer.categorizeParameter('Chorus Rate')).toBe('effects');
      expect(categorizer.categorizeParameter('Delay Time')).toBe('effects');
      expect(categorizer.categorizeParameter('Reverb Size')).toBe('effects');
    });
  });

  describe('amplifier parameters', () => {
    it('should categorize basic amplifier parameters', () => {
      expect(categorizer.categorizeParameter('Amplifier')).toBe('amplifier');
      expect(categorizer.categorizeParameter('Amp')).toBe('amplifier');
      expect(categorizer.categorizeParameter('VCA')).toBe('amplifier');
      expect(categorizer.categorizeParameter('Velocity')).toBe('amplifier');
    });

    it('should handle compound amplifier parameters', () => {
      expect(categorizer.categorizeParameter('Amp Gain')).toBe('amplifier');
      expect(categorizer.categorizeParameter('VCA Level')).toBe('amplifier');
      expect(categorizer.categorizeParameter('Velocity Sensitivity')).toBe('amplifier');
    });
  });

  describe('master parameters', () => {
    it('should categorize basic master parameters', () => {
      expect(categorizer.categorizeParameter('Master')).toBe('master');
      expect(categorizer.categorizeParameter('Volume')).toBe('master');
      expect(categorizer.categorizeParameter('Gain')).toBe('master');
      expect(categorizer.categorizeParameter('Tune')).toBe('master');
      expect(categorizer.categorizeParameter('Octave')).toBe('master');
      expect(categorizer.categorizeParameter('Output')).toBe('master');
      expect(categorizer.categorizeParameter('Main')).toBe('master');
      expect(categorizer.categorizeParameter('Out')).toBe('master');
    });

    it('should handle compound master parameters', () => {
      expect(categorizer.categorizeParameter('Master Volume')).toBe('master');
      expect(categorizer.categorizeParameter('Output Gain')).toBe('master');
      expect(categorizer.categorizeParameter('Main Level')).toBe('master');
    });

    it('should match "out" as whole word only', () => {
      expect(categorizer.categorizeParameter('Out')).toBe('master');
      expect(categorizer.categorizeParameter('Main Out')).toBe('master');
      // Should not match "out" inside other words
      expect(categorizer.categorizeParameter('Cutoff')).toBe('filter'); // This should be filter, not master
    });
  });

  describe('parameter precedence and edge cases', () => {
    it('should prioritize more specific categories over general ones', () => {
      // "Filter Env Attack" should be envelope, not filter
      expect(categorizer.categorizeParameter('Filter Env Attack')).toBe('envelope');

      // "LFO Rate" should be lfo, not master (rate could be general)
      expect(categorizer.categorizeParameter('LFO Rate')).toBe('lfo');

      // "Delay Rate" should be effects, not lfo/master
      expect(categorizer.categorizeParameter('Delay Rate')).toBe('effects');

      // "Volume Envelope" should be envelope, not master
      expect(categorizer.categorizeParameter('Volume Envelope')).toBe('envelope');
    });

    it('should handle edge cases correctly', () => {
      // Empty string
      expect(categorizer.categorizeParameter('')).toBe('misc');

      // Unknown parameter
      expect(categorizer.categorizeParameter('Random Unknown Parameter')).toBe('misc');

      // Numeric parameter names
      expect(categorizer.categorizeParameter('Parameter 1')).toBe('misc');

      // Special characters
      expect(categorizer.categorizeParameter('Param-1_Test')).toBe('misc');
    });

    it('should handle mixed case consistently', () => {
      expect(categorizer.categorizeParameter('FILTER ENV ATTACK')).toBe('envelope');
      expect(categorizer.categorizeParameter('filter env attack')).toBe('envelope');
      expect(categorizer.categorizeParameter('Filter ENV Attack')).toBe('envelope');
    });
  });

  describe('misc parameters', () => {
    it('should return misc for unrecognized parameters', () => {
      expect(categorizer.categorizeParameter('Random Parameter')).toBe('misc');
      expect(categorizer.categorizeParameter('Unknown Control')).toBe('misc');
      expect(categorizer.categorizeParameter('Mystery Knob')).toBe('misc');
      expect(categorizer.categorizeParameter('Parameter 42')).toBe('misc');
      expect(categorizer.categorizeParameter('Control X')).toBe('misc');
    });
  });

  describe('factory function', () => {
    it('should create a functional parameter categorizer', () => {
      const categorizer = createParameterCategorizer();
      expect(categorizer).toBeDefined();
      expect(categorizer.categorizeParameter('Attack')).toBe('envelope');
      expect(categorizer.categorizeParameter('Filter')).toBe('filter');
      expect(categorizer.categorizeParameter('LFO')).toBe('lfo');
    });
  });
});