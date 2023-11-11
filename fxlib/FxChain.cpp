//
// Created by Orion Letizi on 11/10/23.
//

#include "FxChain.h"

int ol::fxlib::FxChain::Process(const float &in1, const float &in2, float *out1, float *out2) {
    float wet1, wet2;
    int rv = 0;
    rv += verb_.Process(in1, in2, &wet1, &wet2);
    *out1 = (wet1 * verb_balance_) + (in1 * (1 - verb_balance_));
    *out2 = (wet2 * verb_balance_) + (in2 * (1 - verb_balance_));

    delay1_.UpdateDelayTime(delay_time_);
    delay1_.UpdateResonance(delay_resonance_);
    delay1_.UpdateCutoff(delay_cutoff_);
    delay1_.UpdateResonance(delay_resonance_);
    *out1 += delay1_.Process(in1) * delay_balance_;
    return rv;
}
