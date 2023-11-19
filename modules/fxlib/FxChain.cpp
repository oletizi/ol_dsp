//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fx::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet_out_1 = 0, wet_out_2 = 0;
    int rv = 0;
    // process delay lines
    float delay_balance = control_panel_->delay_control_->balance.Value();
    t_sample delay_value_1 = (delay1_->Process(in1) * delay_balance) + (in1 * (1-delay_balance));
    t_sample delay_value_2 = (delay2_->Process(in2) * delay_balance) + (in2 * (1-delay_balance));

    *out1 += delay_value_1;
    *out2 += delay_value_2;

    // process reverb
    t_sample verb_balance = control_panel_->reverb_control_->reverb_balance.Value();
    rv += verb_->Process(*out1, *out2, &wet_out_1, &wet_out_2);
    *out1 += (wet_out_1 * verb_balance);
    *out2 += (wet_out_2 * verb_balance);
    return rv;
}