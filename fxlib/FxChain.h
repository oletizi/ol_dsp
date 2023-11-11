//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "ol_fxlib_core.h"
#include "Delay.h"

namespace ol::fxlib {
    class FxChain {
    public:
        void Init(sample_t sample_rate) {
            verb_.Init(sample_rate);
            verb_.SetFeedback(verb_feedback_);
            verb_.SetLpFreq(verb_cutoff_);

            delay1_.Init(sample_rate);
            delay1_.UpdateDelayTime(delay_time_);
            delay1_.UpdateFeedback(delay_feedback_);
            delay1_.UpdateCutoff(delay_cutoff_);
        }

        int Process(const sample_t &in1, const sample_t &in2, sample_t *out1, sample_t *out2);

    private:
        daisysp::ReverbSc verb_;
        sample_t verb_feedback_ = 0.75;
        sample_t verb_cutoff_ = 5000;
        sample_t verb_balance_ = 0.5;

        Delay delay1_;
        sample_t delay_time_ = 30000;
        sample_t delay_feedback_ = 0.5;
        sample_t delay_cutoff_ = 5000;
        sample_t delay_resonance_ = 0.0f;
        sample_t delay_balance_ = 0.7;
    };

}


#endif //OL_DSP_FXCHAIN_H
