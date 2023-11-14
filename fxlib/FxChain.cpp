//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fx::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet_out_1 = 0, wet_out_2 = 0;
    int rv = 0;
    updateDelays();

    // process delay lines
    t_sample delay_balance = control_panel_->delay_balance.Value();
    t_sample delay_value  = delays[0].Process(in1) * delay_balance;
    *out1 += delay_value;
    *out2 += delay_value;

    // process reverb
    verb_.SetFeedback(control_panel_->reverb_time.Value());
    verb_.SetLpFreq(control_panel_->reverb_cutoff.Value());

    t_sample verb_balance = control_panel_->reverb_balance.Value();
    rv += verb_.Process(* out1, * out2, &wet_out_1, &wet_out_2);
    *out1 += (wet_out_1 * verb_balance);// + (wet_in_1 * (1 - verb_balance));
    *out2 += (wet_out_2 * verb_balance);// + (wet_in_2 * (1 - verb_balance));

    if (counter_ % 40000 == 0) {
        counter_ = 0;
    }
    counter_++;
    return rv;
}

void ol::fx::FxChain::updateDelays() {
    for (auto &delay: delays) {
        delay.UpdateDelayTime(control_panel_->delay_time.Value());
        delay.UpdateFeedback(control_panel_->delay_feedback.Value());
        delay.UpdateCutoff(control_panel_->delay_cutoff.Value());
        delay.UpdateResonance(control_panel_->delay_resonance.Value());
    }
}
