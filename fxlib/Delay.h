//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_DELAY_H
#define OL_DSP_DELAY_H
#include "ol_fxlib_core.h"

namespace ol::fxlib {
    class Delay {
    public:
        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            delay_.Init();
            delay_.SetDelay(delay_time_);
            filt_.Init(sample_rate);
            filt_.SetFreq(cutoff_);
            filt_.SetRes(resonance_);
        }

        void UpdateDelayTime(t_sample delay_time);
        void UpdateFeedback(t_sample feedback);
        t_sample Process(t_sample in);

        void UpdateCutoff(t_sample cutoff);
        void UpdateResonance(t_sample resonance);

    private:
        uint64_t counter_;
        t_sample sample_rate_;
        t_sample delay_time_;
        t_sample feedback_;
        daisysp::DelayLine<t_sample, 48000> delay_;
        daisysp::Svf filt_;
        t_sample cutoff_;
        t_sample resonance_;
    };
}


#endif //OL_DSP_DELAY_H
