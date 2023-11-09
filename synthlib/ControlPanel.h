//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_DSP_CONTROLPANEL_H
#define OL_DSP_CONTROLPANEL_H

#include "Control.h"

#define FILTER_CUTOFF_MAX 20000
#define DEFAULT_HARDWARE_SCALE Scale(0, 1, 0, 1, 1)
#define DEFAULT_MIDI_SCALE Scale(0, 127, 0, 1, 1)

namespace ol::synthlib {
    class ControlPanel {
    public:

        // Oscillator
        enum WaveForm {
            WAVE_SIN,
            WAVE_SAW
        };

        WaveForm wave_form = WAVE_SAW;

        // Filter
        Control filter_cutoff = Control(Scale(0, 1, 0, FILTER_CUTOFF_MAX, 1),
                                        Scale(0, 127, 0, FILTER_CUTOFF_MAX, 1), FILTER_CUTOFF_MAX);
        Control filter_resonance = Control();

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
        }

    };
}
#endif //OL_DSP_CONTROLPANEL_H
