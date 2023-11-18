//
// Created by Orion Letizi on 11/17/23.
//

#ifndef OL_DSP_DELAYCONTROLPANEL_H
#define OL_DSP_DELAYCONTROLPANEL_H

#include "ol_corelib.h"
#include "Control.h"
#include "cc_map.h"
#include "Delay.h"

namespace ol::fx {

    class DelayControlPanel {
    public:
        void UpdateMidi(uint16_t control_number, uint16_t control_value);

        ctl::Control time = ctl::Control(
                core::Scale(0, 1, 0, Delay::MAX_DELAY_SAMPLES, 1),
                core::Scale(0, 127, 0, Delay::MAX_DELAY_SAMPLES, 1)
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
