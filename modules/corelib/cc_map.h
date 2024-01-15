//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_CC_MAP_H
#define OL_DSP_CC_MAP_H

#define CC_REVERB_TIME 32
#define CC_REVERB_CUTOFF 33
#define CC_REVERB_BALANCE 34

#define CC_DELAY_TIME 35
#define CC_DELAY_FEEDBACK 36
#define CC_DELAY_CUTOFF 37
#define CC_DELAY_RESONANCE 38
#define CC_DELAY_BALANCE 39
#define CC_DELAY_FILTER_TYPE 40

// Voice Filter
#define CC_FILTER_CUTOFF 41
#define CC_FILTER_RESONANCE 42
#define CC_FILTER_TYPE 43
#define CC_FILTER_DRIVE 44

// FX Filter
#define CC_FX_FILTER_CUTOFF 45
#define CC_FX_FILTER_RESONANCE 46
#define CC_FX_FILTER_TYPE 47
#define CC_FX_FILTER_DRIVE 48

// Reverb
#define CC_EARLY_PREDELAY 50
#define CC_REVERB_TYPE 51
#define CC_REVERB_PREDELAY 52
#define CC_REVERB_PREFILTER 53
#define CC_REVERB_INPUT_DIFFUSION_1 54
#define CC_REVERB_INPUT_DIFFUSION_2 55
#define CC_REVERB_DECAY_DIFFUSION 56

#define CC_SATURATOR_DRIVE 60


// Synth voice
#define FILTER_CUTOFF_MAX 20000
#define DEFAULT_HARDWARE_SCALE Scale(0, 1, 0, 1, 1)
#define DEFAULT_MIDI_SCALE Scale(0, 127, 0, 1, 1)
#define CC_CTL_MOD_WHEEL 1
#define CC_CTL_PORTAMENTO 5
#define CC_CTL_VOLUME 7
#define CC_VOICE_GATE_ON 70
#define CC_VOICE_GATE_OFF 71
#define CC_VOICE_PITCH 72
#define CC_ENV_FILT_AMT 73
#define CC_ENV_FILT_A 74
#define CC_ENV_FILT_D 75
#define CC_ENV_FILT_S 76
#define CC_ENV_FILT_R 77
#define CC_ENV_AMP_A 108
#define CC_ENV_AMP_D 109
#define CC_ENV_AMP_S 110
#define CC_ENV_AMP_R 111
#define CC_VOICE_GAIN 112
#define CC_OSC_1_WAVEFORM 113
#define CC_OSC_1_VOLUME 114
#define CC_OSC_2_VOLUME 115
#define CC_OSC_3_VOLUME 116
#define CC_OSC_4_VOLUME 117
#define CC_OSC_1_SLOP 118
#define CC_OSC_2_SLOP 119
#define CC_OSC_3_SLOP 120
#define CC_OSC_4_SLOP 121

#endif //OL_DSP_CC_MAP_H
