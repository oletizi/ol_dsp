//
// Created by Orion Letizi on 11/8/23.
//

#include "Voice.h"

t_sample ol::synthlib::Voice::Process() {
    osc1_.SetFreq(control_panel_->osc_frequency.Value());
    return osc1_.Process();
}
