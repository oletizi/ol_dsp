//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCONTROLPANEL_H
#define OL_DSP_FXCONTROLPANEL_H

#define CC_REVERB_TIME 32
#define CC_REVERB_CUTOFF 33
#define CC_REVERB_BALANCE 34

#define CC_DELAY_TIME 35
#define CC_DELAY_FEEDBACK 36
#define CC_DELAY_CUTOFF 37
#define CC_DELAY_RESONANCE 38
#define CC_DELAY_BALANCE 39

#include "ol_corelib.h"
#include "ol_ctllib.h"

namespace ol::fx {
    class FxControlPanel {
    public:
        FxControlPanel() {
            reverb_time.UpdateValueHardware(0.25);
            reverb_cutoff.UpdateValueHardware(.4);
            reverb_balance.UpdateValueHardware(0.3);
            delay_feedback.UpdateValueHardware(0.4);
            delay_time.UpdateValueHardware(0.5);
            delay_cutoff.UpdateValueHardware(0.4);
            delay_balance.UpdateValueHardware(0.25);
        }
        void UpdateMidi(int controller_number, int controller_value);

        ctl::Control reverb_time = ctl::Control();
        ctl::Control reverb_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1),
                core::Scale(0, 127, 0, 20000, 1));
        ctl::Control reverb_balance = ctl::Control();
        ctl::Control delay_time = ctl::Control(
                core::Scale(0, 1, 0, 48000, 1),
                core::Scale(0, 127, 0, 48000, 1));
        ctl::Control delay_feedback = ctl::Control();
        ctl::Control delay_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1),
                core::Scale(0, 127, 0, 20000, 1));
        ctl::Control delay_resonance = ctl::Control();
        ctl::Control delay_balance = ctl::Control();

    };
}

#endif //OL_DSP_FXCONTROLPANEL_H
