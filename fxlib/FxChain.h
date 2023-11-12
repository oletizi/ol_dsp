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
        void Init(t_sample sample_rate) {
            verb_.Init(sample_rate);
            verb_.SetFeedback(verb_feedback_);
            verb_.SetLpFreq(verb_cutoff_);

            delay_1_.Init(sample_rate);
            delay_2_.Init(sample_rate);
            updateDelays();
        }

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2);

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
        t_sample verb_feedback_ = 0.75;
        t_sample verb_cutoff_ = 5000;
        t_sample verb_balance_ = 0.5;

        Delay delay_1_;
        Delay delay_2_;
        t_sample delay_time_ = 30000;
        t_sample delay_feedback_ = 0.3;
        t_sample delay_cutoff_ = 5000;
        t_sample delay_resonance_ = 0.3f;
        t_sample delay_balance_ = 0.7;
    };

}


#endif //OL_DSP_FXCHAIN_H
