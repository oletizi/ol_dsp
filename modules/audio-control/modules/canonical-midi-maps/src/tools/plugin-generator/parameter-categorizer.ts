/**
 * Parameter categorization utility for plugin specifications
 */

/**
 * Interface for parameter categorization service
 */
export interface IParameterCategorizer {
  categorizeParameter(paramName: string): string;
}

/**
 * Default implementation of parameter categorization
 */
export class ParameterCategorizer implements IParameterCategorizer {
  /**
   * Categorize a parameter based on its name
   *
   * Order matters! More specific terms should be checked first to handle
   * compound names like "Filter Env Attack" correctly.
   */
  categorizeParameter(paramName: string): string {
    const name = paramName.toLowerCase();

    // Check more specific terms first to handle compound parameter names correctly

    // Envelope parameters (specific ADSR terms)
    if (this.isEnvelopeParameter(name)) {
      return 'envelope';
    }

    // Effects parameters (specific effect names)
    if (this.isEffectsParameter(name)) {
      return 'effects';
    }

    // Filter parameters (cutoff, resonance, Q are quite specific)
    if (this.isFilterParameter(name)) {
      return 'filter';
    }

    // LFO parameters (modulation terms)
    if (this.isLfoParameter(name)) {
      return 'lfo';
    }

    // Oscillator parameters
    if (this.isOscillatorParameter(name)) {
      return 'oscillator';
    }

    // Amplifier parameters
    if (this.isAmplifierParameter(name)) {
      return 'amplifier';
    }

    // Master/Global controls (broader terms, checked later)
    if (this.isMasterParameter(name)) {
      return 'master';
    }

    return 'misc';
  }

  private isMasterParameter(name: string): boolean {
    return name.includes('master') ||
           name.includes('volume') ||
           name.includes('gain') ||
           name.includes('tune') ||
           name.includes('octave') ||
           name.includes('output') ||
           name.includes('main') ||
           name.match(/\bout\b/) !== null; // Match "out" as whole word
  }

  private isOscillatorParameter(name: string): boolean {
    return name.includes('osc') ||
           name.includes('vco') ||
           name.includes('wave') ||
           name.includes('pitch') ||
           name.includes('detune') ||
           name.includes('sync');
  }

  private isFilterParameter(name: string): boolean {
    return name.includes('filter') ||
           name.includes('vcf') ||
           name.includes('cutoff') ||
           name.includes('resonance') ||
           name.includes('q ') ||
           name.includes('freq') ||
           name.match(/\blp\b/) !== null; // Match "lp" for "LP Cutoff"
  }

  private isEnvelopeParameter(name: string): boolean {
    return name.includes('env') ||
           name.includes('attack') ||
           name.includes('decay') ||
           name.includes('sustain') ||
           name.includes('release') ||
           name.includes('adsr');
  }

  private isLfoParameter(name: string): boolean {
    return name.includes('lfo') ||
           name.includes('mod') ||
           name.includes('rate') ||
           name.includes('depth') ||
           name.includes('speed');
  }

  private isEffectsParameter(name: string): boolean {
    return name.includes('chorus') ||
           name.includes('delay') ||
           name.includes('reverb') ||
           name.includes('phaser') ||
           name.includes('flanger') ||
           name.includes('distortion');
  }

  private isAmplifierParameter(name: string): boolean {
    return name.includes('amp') ||
           name.includes('vca') ||
           name.includes('velocity');
  }
}

/**
 * Factory function to create parameter categorizer
 */
export function createParameterCategorizer(): IParameterCategorizer {
  return new ParameterCategorizer();
}