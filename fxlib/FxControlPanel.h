//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCONTROLPANEL_H
#define OL_DSP_FXCONTROLPANEL_H

#include "Control.h"
#include "ReverbControlPanel.h"
#include "cc_map.h"
#include "DelayControlPanel.h"

namespace ol::fx {
    class FxControlPanel {
    public:
        explicit FxControlPanel(ReverbControlPanel * reverb_control, DelayControlPanel * delay_control)
            : reverb_control_(reverb_control), delay_control_(delay_control) {
            lpf_cutoff.UpdateValueHardware(0.4);
            lpf_resonance.UpdateValueHardware(0);
        }

        void UpdateMidi(int controller_number, int controller_value);

        // Reverb
        ReverbControlPanel *reverb_control_;

        // Delay
        DelayControlPanel * delay_control_;

        // Filter
        ctl::Control lpf_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1.1),
                core::Scale(0, 127, 0, 20000, 1.1));
        ctl::Control lpf_resonance  = ctl::Control();
        ctl::Control lpf_type = ctl::Control();
    };
}

#endif //OL_DSP_FXCONTROLPANEL_H
