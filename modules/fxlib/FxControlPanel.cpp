//
// Created by Orion Letizi on 11/10/23.
//
#include "FxControlPanel.h"

void ol::fx::FxControlPanel::UpdateMidi(int controller_number, int value) const {
    reverb_control_->UpdateMidi(controller_number, value);
    delay_control_->UpdateMidi(controller_number, value);
    lpf_control_->UpdateMidi(controller_number, value);
}
