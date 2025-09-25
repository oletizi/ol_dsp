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
   */
  categorizeParameter(paramName: string): string {
    const name = paramName.toLowerCase();

    // Master/Global controls
    if (this.isMasterParameter(name)) {
      return 'master';
    }

    // Oscillator parameters
    if (this.isOscillatorParameter(name)) {
      return 'oscillator';
    }

    // Filter parameters
    if (this.isFilterParameter(name)) {
      return 'filter';
    }

    // Envelope parameters
    if (this.isEnvelopeParameter(name)) {
      return 'envelope';
    }

    // LFO parameters
    if (this.isLfoParameter(name)) {
      return 'lfo';
    }

    // Effects parameters
    if (this.isEffectsParameter(name)) {
      return 'effects';
    }

    // Amplifier parameters
    if (this.isAmplifierParameter(name)) {
      return 'amplifier';
    }

    return 'misc';
  }

  private isMasterParameter(name: string): boolean {
    return name.includes('master') ||
           name.includes('volume') ||
           name.includes('gain') ||
           name.includes('tune') ||
           name.includes('octave') ||
           name.includes('output');
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
           name.includes('freq');
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