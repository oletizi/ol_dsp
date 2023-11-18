//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fx::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet_out_1 = 0, wet_out_2 = 0;
    int rv = 0;
    updateLpfs();

    // process delay lines
    t_sample delay_value_1 = delay1_.Process(in1);
    t_sample delay_value_2 = delay2_.Process(in2);

    *out1 += delay_value_1;
    *out2 += delay_value_2;

    // process reverb
    verb_.SetFeedback(control_panel_->reverb_control_->reverb_time.Value());
    verb_.SetLpFreq(control_panel_->reverb_control_->reverb_cutoff.Value());

    t_sample verb_balance = control_panel_->reverb_control_->reverb_balance.Value();
    rv += verb_.Process(*out1, *out2, &wet_out_1, &wet_out_2);
    *out1 += (wet_out_1 * verb_balance);
    *out2 += (wet_out_2 * verb_balance);

    *out1 = lpfs_[0].Process(*out1);
    *out2 = lpfs_[1].Process(*out2);
    if (counter_ % 48000 == 0) {
        counter_ = 0;
    }
    counter_++;
    return rv;
}

void ol::fx::FxChain::updateLpfs() {
    for (auto &filter: lpfs_) {
        filter.SetFreq(control_panel_->lpf_cutoff.Value());
        filter.SetRes(control_panel_->lpf_resonance.Value());
        t_sample type_val = control_panel_->lpf_type.Value();
        if (type_val > 0.6) {
            filter.UpdateFilterType(LPF::MOOG_LADDER);
        } else if (type_val > 0.3) {
            filter.UpdateFilterType(LPF::BIQUAD);
        } else {
            filter.UpdateFilterType(LPF::SVF);
        }
    }

}
