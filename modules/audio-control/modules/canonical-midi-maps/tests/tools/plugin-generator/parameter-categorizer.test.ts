/**
 * Tests for parameter categorization functionality
 */

import { describe, it, expect, beforeEach } from 'vitest';
import { ParameterCategorizer, createParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';
import type { IParameterCategorizer } from '@/tools/plugin-generator/parameter-categorizer.js';

describe('ParameterCategorizer', () => {
  let categorizer: IParameterCategorizer;

  beforeEach(() => {
    categorizer = createParameterCategorizer();
  });

  describe('master parameters', () => {
    const masterParams = [
      'Master Volume',
      'master gain',
      'OUTPUT LEVEL',
      'volume control',
      'Main Tune',
      'octave shift',
      'MASTER_VOLUME',
    ];

    it.each(masterParams)('should categorize "%s" as master', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('master');
    });
  });

  describe('oscillator parameters', () => {
    const oscParams = [
      'OSC1 Wave',
      'vco frequency',
      'Oscillator Shape',
      'wave select',
      'Pitch Bend',
      'detune amount',
      'sync mode',
      'OSC_PITCH',
    ];

    it.each(oscParams)('should categorize "%s" as oscillator', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('oscillator');
    });
  });

  describe('filter parameters', () => {
    const filterParams = [
      'Filter Cutoff',
      'vcf frequency',
      'Low Pass',
      'cutoff freq',
      'Resonance',
      'Q Factor',
      'FREQ_MOD',
    ];

    it.each(filterParams)('should categorize "%s" as filter', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('filter');
    });
  });

  describe('envelope parameters', () => {
    const envelopeParams = [
      'ENV1 Attack',
      'envelope decay',
      'ADSR Sustain',
      'release time',
      'attack rate',
      'decay level',
      'sustain volume',
    ];

    it.each(envelopeParams)('should categorize "%s" as envelope', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('envelope');
    });
  });

  describe('lfo parameters', () => {
    const lfoParams = [
      'LFO Rate',
      'modulation depth',
      'mod speed',
      'rate control',
      'depth amount',
      'LFO_SPEED',
    ];

    it.each(lfoParams)('should categorize "%s" as lfo', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('lfo');
    });
  });

  describe('effects parameters', () => {
    const effectsParams = [
      'Chorus Rate',
      'delay time',
      'Reverb Size',
      'phaser depth',
      'flanger speed',
      'distortion amount',
      'CHORUS_MIX',
    ];

    it.each(effectsParams)('should categorize "%s" as effects', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('effects');
    });
  });

  describe('amplifier parameters', () => {
    const ampParams = [
      'AMP Gain',
      'vca level',
      'Velocity Sensitivity',
      'amplifier volume',
      'AMP_ENV',
    ];

    it.each(ampParams)('should categorize "%s" as amplifier', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('amplifier');
    });
  });

  describe('misc parameters', () => {
    const miscParams = [
      'Parameter 1',
      'Unknown Control',
      'Custom Setting',
      'User Defined',
      'Special Mode',
      '',
    ];

    it.each(miscParams)('should categorize "%s" as misc', (paramName) => {
      expect(categorizer.categorizeParameter(paramName)).toBe('misc');
    });
  });

  describe('case insensitivity', () => {
    it('should handle different case variations', () => {
      expect(categorizer.categorizeParameter('MASTER VOLUME')).toBe('master');
      expect(categorizer.categorizeParameter('master volume')).toBe('master');
      expect(categorizer.categorizeParameter('Master Volume')).toBe('master');
      expect(categorizer.categorizeParameter('MaStEr VoLuMe')).toBe('master');
    });
  });

  describe('edge cases', () => {
    it('should handle empty string', () => {
      expect(categorizer.categorizeParameter('')).toBe('misc');
    });

    it('should handle whitespace-only string', () => {
      expect(categorizer.categorizeParameter('   ')).toBe('misc');
    });

    it('should handle special characters', () => {
      expect(categorizer.categorizeParameter('Filter*Cutoff#1')).toBe('filter');
      expect(categorizer.categorizeParameter('OSC-1_WAVE')).toBe('oscillator');
    });

    it('should handle numbers in parameter names', () => {
      expect(categorizer.categorizeParameter('ENV1 Attack')).toBe('envelope');
      expect(categorizer.categorizeParameter('LFO2 Rate')).toBe('lfo');
      expect(categorizer.categorizeParameter('OSC3 Wave')).toBe('oscillator');
    });
  });

  describe('priority handling', () => {
    it('should handle parameters with multiple category keywords (first match wins)', () => {
      // Based on the order in the categorizeParameter method
      expect(categorizer.categorizeParameter('Master Filter Volume')).toBe('master'); // master comes first
      expect(categorizer.categorizeParameter('OSC Filter Freq')).toBe('oscillator'); // oscillator comes before filter
      expect(categorizer.categorizeParameter('Filter Envelope Attack')).toBe('filter'); // filter comes before envelope
    });
  });

  describe('createParameterCategorizer factory', () => {
    it('should create a valid categorizer instance', () => {
      const categorizerInstance = createParameterCategorizer();

      expect(categorizerInstance).toBeDefined();
      expect(typeof categorizerInstance.categorizeParameter).toBe('function');
    });

    it('should create independent instances', () => {
      const cat1 = createParameterCategorizer();
      const cat2 = createParameterCategorizer();

      expect(cat1).not.toBe(cat2);
      expect(cat1).toBeInstanceOf(ParameterCategorizer);
      expect(cat2).toBeInstanceOf(ParameterCategorizer);
    });
  });

  describe('comprehensive keyword coverage', () => {
    // Testing specific keyword detection patterns
    it('should detect Q parameter correctly', () => {
      expect(categorizer.categorizeParameter('Q ')).toBe('filter');
      expect(categorizer.categorizeParameter('Q Factor')).toBe('filter');
      expect(categorizer.categorizeParameter('Filter Q')).toBe('filter');
    });

    it('should not false positive on partial Q matches', () => {
      expect(categorizer.categorizeParameter('Quality')).toBe('misc'); // 'q ' not matched
      expect(categorizer.categorizeParameter('Quantize')).toBe('misc'); // 'q ' not matched
    });

    it('should handle common abbreviations', () => {
      expect(categorizer.categorizeParameter('VCO1')).toBe('oscillator');
      expect(categorizer.categorizeParameter('VCF Cutoff')).toBe('filter');
      expect(categorizer.categorizeParameter('VCA Level')).toBe('amplifier');
      expect(categorizer.categorizeParameter('ADSR')).toBe('envelope');
    });
  });
});