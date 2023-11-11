//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_DELAY_H
#define OL_DSP_DELAY_H
#include "ol_fxlib_core.h"

namespace ol::fxlib {
    class Delay {
    public:
        void Init(sample_t sample_rate) {
            sample_rate_ = sample_rate;
            delay_.Init();
            delay_.SetDelay(delay_time_);
            filt_.Init(sample_rate);
            filt_.SetFreq(cutoff_);
            filt_.SetRes(resonance_);
        }

        void UpdateDelayTime(sample_t delay_time);
        void UpdateFeedback(sample_t feedback);
        sample_t Process(sample_t in);

        void UpdateCutoff(sample_t cutoff);
        void UpdateResonance(sample_t resonance);

    private:
        uint64_t counter_;
        sample_t sample_rate_;
        sample_t delay_time_;
        sample_t feedback_;
        daisysp::DelayLine<sample_t, 48000> delay_;
        daisysp::Svf filt_;
        sample_t cutoff_;
        sample_t resonance_;
    };
}


#endif //OL_DSP_DELAY_H
