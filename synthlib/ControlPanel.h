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

namespace ol::synthlib {
    class ControlPanel {
    public:

        // Oscillator
        enum WaveForm {
            WAVE_SIN,
            WAVE_SAW
        };

        // TODO: Move this to .cpp file and implement swappable mapping. Maybe even midi learn?
        void UpdateMidi(int ctl, int val) {
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
                default:
                    std::cout << "CC not mapped: " << ctl << std::endl;
                    break;
            }
        }

        WaveForm wave_form = WAVE_SAW;

        // Filter
        Control filter_cutoff = Control(Scale(0, 1, 0, FILTER_CUTOFF_MAX, 1),
                                        Scale(0, 127, 0, FILTER_CUTOFF_MAX, 1), FILTER_CUTOFF_MAX);
        Control filter_resonance = Control();
        Control filter_drive = Control();

        // Filter envelope
        Control env_filt_A = Control();
        Control env_filt_D = Control();
        Control env_filt_S = Control(DEFAULT_HARDWARE_SCALE, DEFAULT_MIDI_SCALE, 1);
        Control env_filt_R = Control();

        // Amp envelope
        Control env_amp_A = Control();
        Control env_amp_D = Control();
        Control env_amp_S = Control(DEFAULT_HARDWARE_SCALE, DEFAULT_MIDI_SCALE, 1);
        Control env_amp_R = Control();

        Control portamento = Control(Scale(0, 1, 0, 0.1, 1.1f),
                                     Scale(0, 127, 0, 0.1, 1.1f)
        );
        Control master_volume = Control();

        ControlPanel() {
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

            // default master volume
            master_volume.UpdateValueHardware(0.8f);
        }

    };
}
#endif //OL_DSP_CONTROLPANEL_H
