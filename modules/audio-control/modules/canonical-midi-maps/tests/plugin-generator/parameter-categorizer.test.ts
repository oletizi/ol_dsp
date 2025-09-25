/**
 * Tests for parameter categorization logic
 */

import { describe, it, expect, beforeEach } from 'vitest';
import {
  ParameterCategorizer,
  createParameterCategorizer
} from '@/tools/plugin-generator/parameter-categorizer.js';
import type { IParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

describe('ParameterCategorizer', () => {
  let categorizer: IParameterCategorizer;

  beforeEach(() => {
    categorizer = new ParameterCategorizer();
  });

  describe('categorizeParameter', () => {
    describe('master parameters', () => {
      it('should categorize master volume controls', () => {
        expect(categorizer.categorizeParameter('Master Volume')).toBe('master');
        expect(categorizer.categorizeParameter('master gain')).toBe('master');
        expect(categorizer.categorizeParameter('VOLUME')).toBe('master');
        expect(categorizer.categorizeParameter('Main Gain')).toBe('master');
      });

      it('should categorize tune and octave controls', () => {
        expect(categorizer.categorizeParameter('Master Tune')).toBe('master');
        expect(categorizer.categorizeParameter('octave')).toBe('master');
        expect(categorizer.categorizeParameter('Output Level')).toBe('master');
      });
    });

    describe('oscillator parameters', () => {
      it('should categorize oscillator controls', () => {
        expect(categorizer.categorizeParameter('Osc 1 Level')).toBe('oscillator');
        expect(categorizer.categorizeParameter('VCO Frequency')).toBe('oscillator');
        expect(categorizer.categorizeParameter('Wave Shape')).toBe('oscillator');
        expect(categorizer.categorizeParameter('Pitch Bend')).toBe('oscillator');
      });

      it('should categorize detune and sync controls', () => {
        expect(categorizer.categorizeParameter('Detune')).toBe('oscillator');
        expect(categorizer.categorizeParameter('Osc Sync')).toBe('oscillator');
      });
    });

    describe('filter parameters', () => {
      it('should categorize filter controls', () => {
        expect(categorizer.categorizeParameter('Filter Cutoff')).toBe('filter');
        expect(categorizer.categorizeParameter('VCF Resonance')).toBe('filter');
        expect(categorizer.categorizeParameter('Low Pass Freq')).toBe('filter');
        expect(categorizer.categorizeParameter('Q Factor')).toBe('filter');
      });

      it('should handle single letter Q parameter', () => {
        expect(categorizer.categorizeParameter('Q ')).toBe('filter');
        expect(categorizer.categorizeParameter('q value')).toBe('filter');
      });
    });

    describe('envelope parameters', () => {
      it('should categorize ADSR controls', () => {
        expect(categorizer.categorizeParameter('Attack Time')).toBe('envelope');
        expect(categorizer.categorizeParameter('Decay')).toBe('envelope');
        expect(categorizer.categorizeParameter('Sustain Level')).toBe('envelope');
        expect(categorizer.categorizeParameter('Release')).toBe('envelope');
      });

      it('should categorize envelope controls', () => {
        expect(categorizer.categorizeParameter('Env 1 Amount')).toBe('envelope');
        expect(categorizer.categorizeParameter('ADSR Envelope')).toBe('envelope');
      });
    });

    describe('LFO parameters', () => {
      it('should categorize LFO controls', () => {
        expect(categorizer.categorizeParameter('LFO Rate')).toBe('lfo');
        expect(categorizer.categorizeParameter('Mod Depth')).toBe('lfo');
        expect(categorizer.categorizeParameter('Modulation Speed')).toBe('lfo');
      });

      it('should categorize generic rate and depth', () => {
        expect(categorizer.categorizeParameter('Rate')).toBe('lfo');
        expect(categorizer.categorizeParameter('Depth')).toBe('lfo');
      });
    });

    describe('effects parameters', () => {
      it('should categorize common effects', () => {
        expect(categorizer.categorizeParameter('Chorus Mix')).toBe('effects');
        expect(categorizer.categorizeParameter('Delay Time')).toBe('effects');
        expect(categorizer.categorizeParameter('Reverb Size')).toBe('effects');
        expect(categorizer.categorizeParameter('Phaser Rate')).toBe('effects');
      });

      it('should categorize more effects', () => {
        expect(categorizer.categorizeParameter('Flanger Depth')).toBe('effects');
        expect(categorizer.categorizeParameter('Distortion Drive')).toBe('effects');
      });
    });

    describe('amplifier parameters', () => {
      it('should categorize amplifier controls', () => {
        expect(categorizer.categorizeParameter('Amp Level')).toBe('amplifier');
        expect(categorizer.categorizeParameter('VCA Control')).toBe('amplifier');
        expect(categorizer.categorizeParameter('Velocity Sensitivity')).toBe('amplifier');
      });
    });

    describe('miscellaneous parameters', () => {
      it('should default unknown parameters to misc', () => {
        expect(categorizer.categorizeParameter('Unknown Parameter')).toBe('misc');
        expect(categorizer.categorizeParameter('Custom Control')).toBe('misc');
        expect(categorizer.categorizeParameter('')).toBe('misc');
        expect(categorizer.categorizeParameter('xyz')).toBe('misc');
      });
    });

    describe('case sensitivity', () => {
      it('should handle different cases correctly', () => {
        expect(categorizer.categorizeParameter('MASTER VOLUME')).toBe('master');
        expect(categorizer.categorizeParameter('filter cutoff')).toBe('filter');
        expect(categorizer.categorizeParameter('MiXeD CaSe OsC')).toBe('oscillator');
      });
    });

    describe('parameter name variations', () => {
      it('should handle common naming variations', () => {
        expect(categorizer.categorizeParameter('Main Out')).toBe('master');
        expect(categorizer.categorizeParameter('Osc1 Wave')).toBe('oscillator');
        expect(categorizer.categorizeParameter('LP Cutoff')).toBe('filter');
        expect(categorizer.categorizeParameter('Env Attack')).toBe('envelope');
      });

      it('should handle compound parameter names', () => {
        expect(categorizer.categorizeParameter('Filter Env Attack')).toBe('envelope');
        expect(categorizer.categorizeParameter('Osc Filter Cutoff')).toBe('filter');
        expect(categorizer.categorizeParameter('Master Output Volume')).toBe('master');
      });
    });
  });

  describe('createParameterCategorizer factory', () => {
    it('should create ParameterCategorizer instance', () => {
      const categorizer = createParameterCategorizer();

      expect(categorizer).toBeInstanceOf(ParameterCategorizer);
      expect(typeof categorizer.categorizeParameter).toBe('function');
    });

    it('should create separate instances', () => {
      const categorizer1 = createParameterCategorizer();
      const categorizer2 = createParameterCategorizer();

      expect(categorizer1).not.toBe(categorizer2);
      expect(categorizer1).toBeInstanceOf(ParameterCategorizer);
      expect(categorizer2).toBeInstanceOf(ParameterCategorizer);
    });

    it('should create functional categorizer', () => {
      const categorizer = createParameterCategorizer();

      expect(categorizer.categorizeParameter('Volume')).toBe('master');
      expect(categorizer.categorizeParameter('Cutoff')).toBe('filter');
      expect(categorizer.categorizeParameter('Random')).toBe('misc');
    });
  });
});