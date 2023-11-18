//
// Created by Orion Letizi on 11/10/23.
//
#include "FxControlPanel.h"

void ol::fx::FxControlPanel::UpdateMidi(int controller_number, int value) {
    reverb_control_->UpdateMidi(controller_number, value);
    delay_control_->UpdateMidi(controller_number, value);
    switch (controller_number) {
        case CC_LPF_CUTOFF:
            lpf_cutoff.UpdateValueMidi(value);
            break;
        case CC_LPF_RESONANCE:
            lpf_resonance.UpdateValueMidi(value);
            break;
        case CC_LPF_TYPE:
            lpf_type.UpdateValueMidi(value);
            break;
        default:
            break;
    }
}
