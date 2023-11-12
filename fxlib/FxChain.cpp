//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fx::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet_in_1 = 0, wet_in_2 = 0, wet_out_1 = 0, wet_out_2 = 0;
    int rv = 0;
    updateDelays();
    *out1 = 0;
    *out2 = 0;

    // process delay lines
    t_sample delay_balance = control_panel_->delay_balance.Value();
    wet_in_1 += (delay_1_.Process(in1) * delay_balance) + (in1 * (1 - delay_balance));
    wet_in_2 += (delay_2_.Process(in2) * delay_balance) + (in1 * (1 - delay_balance));

    // process reverb
    verb_.SetFeedback(control_panel_->reverb_time.Value());
    verb_.SetLpFreq(control_panel_->reverb_cutoff.Value());

    t_sample verb_balance = control_panel_->reverb_balance.Value();
    rv += verb_.Process(wet_in_1, wet_in_2, &wet_out_1, &wet_out_2);
    *out1 += (wet_out_1 * verb_balance) + (wet_in_1 * (1 - verb_balance));
    *out2 += (wet_out_2 * verb_balance) + (wet_in_2 * (1 - verb_balance));

    return rv;
}

void ol::fx::FxChain::updateDelays() {
    delay_1_.UpdateDelayTime(control_panel_->delay_time.Value());
    delay_2_.UpdateDelayTime(control_panel_->delay_time.Value());

    delay_1_.UpdateFeedback(control_panel_->delay_feedback.Value());
    delay_2_.UpdateFeedback(control_panel_->delay_feedback.Value());

    delay_1_.UpdateCutoff(control_panel_->delay_cutoff.Value());
    delay_2_.UpdateCutoff(control_panel_->delay_cutoff.Value());

    delay_1_.UpdateResonance(control_panel_->delay_resonance.Value());
    delay_2_.UpdateResonance(control_panel_->delay_resonance.Value());
}
