//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "daisysp.h"

namespace ol::fxlib {
    class FxChain {
    public:
        void Init(float sample_rate) {
            verb_.Init(sample_rate);
            verb_.SetFeedback(0.75f);
            verb_.SetLpFreq(5000.0f);
        }
        int Process(const float &in1, const float &in2, float * out1, float * out2);
    private:
        daisysp::ReverbSc verb_;
        float verb_balance_ = 0.5f;
    };

}


#endif //OL_DSP_FXCHAIN_H
