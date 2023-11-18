//
// Created by Orion Letizi on 11/17/23.
//

#ifndef OL_DSP_DELAYCONTROLPANEL_H
#define OL_DSP_DELAYCONTROLPANEL_H

#include "ol_corelib.h"
#include "Control.h"
#include "cc_map.h"

#define MAX_TIME 48000.0f
namespace ol::fx {

    class DelayControlPanel {
    public:
        void UpdateMidi(uint16_t control_number, uint16_t control_value);
        ctl::Control time = ctl::Control(
                core::Scale(0, 1, 0, MAX_TIME, 1),
                core::Scale(0, 127, 0, MAX_TIME, 1)
        );
        ctl::Control feedback = ctl::Control();
        ctl::Control cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1),
                core::Scale(0, 127, 0, 20000, 1)
        );
        ctl::Control resonance = ctl::Control();
        ctl::Control balance = ctl::Control();
    };

} // ol
// fx

#endif //OL_DSP_DELAYCONTROLPANEL_H
