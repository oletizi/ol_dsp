//
// Created by Orion Letizi on 11/10/23.
//
#include "FxControlPanel.h"

void ol::fx::FxControlPanel::UpdateMidi(int controller_number, int value) {
    switch (controller_number) {
        case CC_REVERB_TIME:
            reverb_control_->reverb_time.UpdateValueMidi(value);
            break;
        case CC_REVERB_CUTOFF:
            reverb_control_->reverb_cutoff.UpdateValueMidi(value);
            break;
        case CC_REVERB_BALANCE:
            reverb_control_->reverb_balance.UpdateValueMidi(value);
            break;
        case CC_DELAY_TIME:
            delay_time.UpdateValueMidi(value);
            break;
        case CC_DELAY_FEEDBACK:
            delay_feedback.UpdateValueMidi(value);
            break;
        case CC_DELAY_CUTOFF:
            delay_cutoff.UpdateValueMidi(value);
            break;
        case CC_DELAY_RESONANCE:
            delay_resonance.UpdateValueMidi(value);
            break;
        case CC_DELAY_BALANCE:
            delay_balance.UpdateValueMidi(value);
            break;
        case CC_DELAY_FILTER_TYPE:
            delay_filter_type.UpdateValueMidi(value);
            break;
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
