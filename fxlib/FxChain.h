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

            delay_1_.Init(sample_rate);
            delay_2_.Init(sample_rate);
            updateDelays();
        }

        int Process(const sample_t &in1, const sample_t &in2, sample_t *out1, sample_t *out2);

    private:

        /**
         * Update parameters on the delay lines.
         */
        void updateDelays() {
            delay_1_.UpdateDelayTime(delay_time_);
            delay_2_.UpdateDelayTime(delay_time_);

            delay_1_.UpdateFeedback(delay_feedback_);
            delay_2_.UpdateFeedback(delay_feedback_);

            delay_1_.UpdateCutoff(delay_cutoff_);
            delay_2_.UpdateCutoff(delay_cutoff_);
        }
        daisysp::ReverbSc verb_;
        sample_t verb_feedback_ = 0.75;
        sample_t verb_cutoff_ = 5000;
        sample_t verb_balance_ = 0.5;

        Delay delay_1_;
        Delay delay_2_;
        sample_t delay_time_ = 30000;
        sample_t delay_feedback_ = 0.3;
        sample_t delay_cutoff_ = 5000;
        sample_t delay_resonance_ = 0.3f;
        sample_t delay_balance_ = 0.7;
    };

}


#endif //OL_DSP_FXCHAIN_H
