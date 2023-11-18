//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_REVERBCONTROLPANEL_H
#define OL_DSP_REVERBCONTROLPANEL_H

#include "cc_map.h"
#include "Control.h"
#include "cc_map.h"

namespace ol::fx {
    class ReverbControlPanel {
    public:
        ReverbControlPanel() {
            reverb_time.UpdateValueHardware(0.5);
            reverb_cutoff.UpdateValueHardware(0.4);
            reverb_balance.UpdateValueHardware(0.5);
        };
        void UpdateMidi(uint16_t control_number, uint16_t control_value);
        ctl::Control reverb_time = ctl::Control();
        ctl::Control reverb_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1),
                core::Scale(0, 127, 0, 20000, 1));
        ctl::Control reverb_balance = ctl::Control();
    };
}


#endif //OL_DSP_REVERBCONTROLPANEL_H
