//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fxlib::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet_in_1 = 0, wet_in_2 = 0, wet_out_1 = 0, wet_out_2 = 0;
    int rv = 0;
    delay1_.UpdateDelayTime(delay_time_);
    delay1_.UpdateResonance(delay_resonance_);
    delay1_.UpdateCutoff(delay_cutoff_);
    delay1_.UpdateResonance(delay_resonance_);
    *out1 = 0;
    *out2 = 0;

    wet_in_1 += (delay1_.Process(in1) * delay_balance_) + (in1 * (1 - delay_balance_));
    wet_in_2 += in2;
    rv += verb_.Process(wet_in_1, wet_in_2, &wet_out_1, &wet_out_2);
    *out1 += (wet_out_1 * verb_balance_) + (wet_in_1 * (1 - verb_balance_));
    *out2 += (wet_out_2 * verb_balance_) + (wet_in_2 * (1 - verb_balance_));

    return rv;
}
