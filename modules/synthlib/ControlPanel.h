//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_DSP_CONTROLPANEL_H
#define OL_DSP_CONTROLPANEL_H

#include "Control.h"
#include "corelib/cc_map.h"


using namespace ol::ctl;
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
