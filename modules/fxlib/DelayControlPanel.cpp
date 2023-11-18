//
// Created by Orion Letizi on 11/17/23.
//

#include "DelayControlPanel.h"

namespace ol::fx {
    void DelayControlPanel::UpdateMidi(int control_number, int control_value) {
        switch (control_number) {
            case CC_DELAY_TIME:
                time.UpdateValueMidi(control_value);
                break;
            case CC_DELAY_FEEDBACK:
                feedback.UpdateValueMidi(control_value);
                break;
            case CC_DELAY_CUTOFF:
                cutoff.UpdateValueMidi(control_value);
                break;
            case CC_DELAY_RESONANCE:
                resonance.UpdateValueMidi(control_value);
                break;
            case CC_DELAY_BALANCE:
                balance.UpdateValueMidi(control_value);
                break;
            default:
                break;
        }
    }
} // ol
// fx