//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCONTROLPANEL_H
#define OL_DSP_FXCONTROLPANEL_H

#include "Control.h"
#include "ReverbControlPanel.h"

#define CC_REVERB_TIME 32
#define CC_REVERB_CUTOFF 33
#define CC_REVERB_BALANCE 34

#define CC_DELAY_TIME 35
#define CC_DELAY_FEEDBACK 36
#define CC_DELAY_CUTOFF 37
#define CC_DELAY_RESONANCE 38
#define CC_DELAY_BALANCE 39
#define CC_DELAY_FILTER_TYPE 40

#define CC_LPF_CUTOFF 41
#define CC_LPF_RESONANCE 42
#define CC_LPF_TYPE 43

namespace ol::fx {
    class FxControlPanel {
    public:
        explicit FxControlPanel(ReverbControlPanel * reverb_control) : reverb_control_(reverb_control) {
            reverb_control_->reverb_time.UpdateValueHardware(0.25);
            reverb_control_->reverb_cutoff.UpdateValueHardware(.4);
            reverb_control_->reverb_balance.UpdateValueHardware(0.3);
            delay_feedback.UpdateValueHardware(0.4);
            delay_time.UpdateValueHardware(0.5);
            delay_cutoff.UpdateValueHardware(0.4);
            delay_balance.UpdateValueHardware(0.25);
            delay_filter_type.UpdateValueHardware(0);

            lpf_cutoff.UpdateValueHardware(0.4);
            lpf_resonance.UpdateValueHardware(0);
        }

        void UpdateMidi(int controller_number, int controller_value);

        // Reverb
        ReverbControlPanel *reverb_control_;

        // Delay
        ctl::Control delay_time = ctl::Control(
                core::Scale(0, 1, 0, 48000, 1),
                core::Scale(0, 127, 0, 48000, 1));
        ctl::Control delay_feedback = ctl::Control();
        ctl::Control delay_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1),
                core::Scale(0, 127, 0, 20000, 1));
        ctl::Control delay_resonance = ctl::Control();
        ctl::Control delay_balance = ctl::Control();
        ctl::Control delay_filter_type = ctl::Control();

        // Filter
        ctl::Control lpf_cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1.1),
                core::Scale(0, 127, 0, 20000, 1.1));
        ctl::Control lpf_resonance  = ctl::Control();
        ctl::Control lpf_type = ctl::Control();
    };
}

#endif //OL_DSP_FXCONTROLPANEL_H
