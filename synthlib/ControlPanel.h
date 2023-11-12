//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_DSP_CONTROLPANEL_H
#define OL_DSP_CONTROLPANEL_H

#include "Control.h"

#define FILTER_CUTOFF_MAX 20000
#define DEFAULT_HARDWARE_SCALE Scale(0, 1, 0, 1, 1)
#define DEFAULT_MIDI_SCALE Scale(0, 127, 0, 1, 1)
#define CC_CTL_MOD_WHEEL 1
#define CC_CTL_PORTAMENTO 5
#define CC_CTL_VOLUME 7
#define CC_FILT_CUTOFF 70
#define CC_FILT_Q 71
#define CC_FILT_DRIVE 72
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

namespace ol::synthlib {
    class ControlPanel {
    public:

        // Oscillator
        enum WaveForm {
            WAVE_SIN,
            WAVE_TRI,
            WAVE_SAW,
            WAVE_SQUARE
        };

        // TODO: Move this to .cpp file and implement swappable mapping. Maybe even midi learn?
        void UpdateMidi(int ctl, int val) {
            std::cout << "ctl: " << ctl << "; val: " << val << std::endl;
            switch (ctl) {
                case CC_CTL_VOLUME:
                    master_volume.UpdateValueMidi(val);
                    break;
                case CC_CTL_PORTAMENTO:
                    portamento.UpdateValueMidi(val);
                    break;
                case CC_FILT_CUTOFF:
                    filter_cutoff.UpdateValueMidi(val);
                    break;
                case CC_FILT_Q:
                    filter_resonance.UpdateValueMidi(val);
                    break;
                case CC_FILT_DRIVE:
                    filter_drive.UpdateValueMidi(val);
                    break;
                case CC_ENV_FILT_AMT:
                    filter_cutoff.UpdateCvAmountMidi(val);
                    break;
                case CC_ENV_FILT_A:
                    env_filt_A.UpdateValueMidi(val);
                    break;
                case CC_ENV_FILT_D:
                    env_filt_D.UpdateValueMidi(val);
                    break;
                case CC_ENV_FILT_S:
                    env_filt_S.UpdateValueMidi(val);
                    break;
                case CC_ENV_FILT_R:
                    env_filt_R.UpdateValueMidi(val);
                    break;
                case CC_ENV_AMP_A:
                    env_amp_A.UpdateValueMidi(val);
                    break;
                case CC_ENV_AMP_D:
                    env_amp_D.UpdateValueMidi(val);
                    break;
                case CC_ENV_AMP_S:
                    env_amp_S.UpdateValueMidi(val);
                    break;
                case CC_ENV_AMP_R:
                    env_amp_R.UpdateValueMidi(val);
                    break;
                case CC_VOICE_GAIN:
                    voice_gain.UpdateValueMidi(val);
                    break;
                case CC_OSC_1_WAVEFORM:
                    osc_1_waveform.UpdateValueMidi(val);
                    break;
                case CC_OSC_1_VOLUME:
                    osc_1_volume.UpdateValueMidi(val);
                    break;
                case CC_OSC_2_VOLUME:
                    osc_2_volume.UpdateValueMidi(val);
                    break;
                case CC_OSC_3_VOLUME:
                    osc_3_volume.UpdateValueMidi(val);
                    break;
                case CC_OSC_4_VOLUME:
                    osc_4_volume.UpdateValueMidi(val);
                    break;
                case CC_OSC_1_SLOP:
                    osc_1_slop.UpdateValueMidi(val);
                    break;
                case CC_OSC_2_SLOP:
                    osc_2_slop.UpdateValueMidi(val);
                    break;
                case CC_OSC_3_SLOP:
                    osc_3_slop.UpdateValueMidi(val);
                    break;
                case CC_OSC_4_SLOP:
                    osc_4_slop.UpdateValueMidi(val);
                    break;
                default:
                    std::cout << "CC not mapped: " << ctl << std::endl;
                    break;
            }
        }

        [[nodiscard]] WaveForm GetOsc1Waveform() const {
            WaveForm rv = WAVE_SAW;
            int val = (int) osc_1_waveform.Value() % 4;
            switch (val) {
                case 0:
                    rv = WAVE_SAW;
                    break;
                case 1:
                    rv = WAVE_SIN;
                    break;
                case 2:
                    rv = WAVE_TRI;
                    break;
                case 3:
                    rv = WAVE_SQUARE;
                    break;
                default:
                    break;
            }
            return rv;
        }

        // Oscillator waveforms
        Control osc_1_waveform = Control(core::Scale(0, 1, 0, 4, 1),
                                         core::Scale(0, 127, 0, 127, 1));
        // Oscillator mix
        Control osc_1_volume = Control();
        Control osc_2_volume = Control();
        Control osc_3_volume = Control();
        Control osc_4_volume = Control();

        // Oscillator slop
        Control osc_1_slop = Control();
        Control osc_2_slop = Control();
        Control osc_3_slop = Control();
        Control osc_4_slop = Control();

        // Filter
        Control filter_cutoff = Control(core::Scale(0, 1, 0, FILTER_CUTOFF_MAX, 1),
                                        core::Scale(0, 127, 0, FILTER_CUTOFF_MAX, 1), FILTER_CUTOFF_MAX);
        Control filter_resonance = Control();
        Control filter_drive = Control();

        // Filter envelope
        Control env_filt_A = Control();
        Control env_filt_D = Control();
        Control env_filt_S = Control();
        Control env_filt_R = Control();

        // Amp envelope
        Control env_amp_A = Control();
        Control env_amp_D = Control();
        Control env_amp_S = Control();
        Control env_amp_R = Control();

        Control portamento = Control(core::Scale(0, 1, 0, 0.1f, 1.1f),
                                     core::Scale(0, 127, 0, 0.1f, 1.1f));
        Control voice_gain = Control();
        Control master_volume = Control();

        ControlPanel() {
            // Oscillator waveform defaults
            osc_1_waveform.UpdateValueHardware(0);

            // Oscillator mix defaults
            osc_1_volume.UpdateValueHardware(0.25f);
            osc_2_volume.UpdateValueHardware(0.25f);
            osc_3_volume.UpdateValueHardware(0.25f);
            osc_4_volume.UpdateValueHardware(0.25f);

            // Oscillator slop defaults
            osc_1_slop.UpdateValueHardware(0.5f);
            osc_2_slop.UpdateValueHardware(0.5f);
            osc_3_slop.UpdateValueHardware(0.5f);
            osc_4_slop.UpdateValueHardware(0.5f);

            // default filter settings
            filter_cutoff.UpdateValueHardware(0.5);
            filter_resonance.UpdateValueHardware(0);

            // default envelope settings
            env_filt_A.UpdateValueHardware(0.2f);
            env_filt_D.UpdateValueHardware(0.3f);
            env_filt_S.UpdateValueHardware(0);
            env_filt_R.UpdateValueHardware(0.3f);

            env_amp_A.UpdateValueHardware(0.2f);
            env_amp_D.UpdateValueHardware(0);
            env_amp_S.UpdateValueHardware(1);
            env_amp_R.UpdateValueHardware(0.3f);

            // default portamento
            portamento.UpdateValueHardware(0.5f);

            // default voice gain
            voice_gain.UpdateValueHardware(0.1f);

            // default master volume
            master_volume.UpdateValueHardware(0.8f);
        }

    };
}
#endif //OL_DSP_CONTROLPANEL_H
