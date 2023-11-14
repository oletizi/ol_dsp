//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_DELAY_H
#define OL_DSP_DELAY_H

#include "ol_fxlib_core.h"

namespace ol::fx {
    class Delay {
    public:
        enum FilterType {
            SVF,
            MOOG_LADDER
        };

        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            delay_time_ = sample_rate / 2;
            delay_.Init();
            delay_.SetDelay(delay_time_);

            filt_svf_.Init(sample_rate);
            filt_moog_.Init(sample_rate);

            updateFilter();
        }

        void UpdateFilterType(FilterType type) { filter_type_ = type; }

        void UpdateDelayTime(t_sample delay_time);

        void UpdateFeedback(t_sample feedback);

        t_sample Process(t_sample in);

        void UpdateCutoff(t_sample cutoff);

        void UpdateResonance(t_sample resonance);

    private:
        void updateFilter() {
            filt_svf_.SetFreq(cutoff_);
            filt_moog_.SetFreq(cutoff_);
            filt_svf_.SetRes(resonance_);
            filt_moog_.SetRes(resonance_);
        }

        uint64_t counter_ = 0;
        t_sample sample_rate_;
        t_sample delay_time_ = 48000 / 2;
        t_sample feedback_ = 0.25;
        FilterType filter_type_ = SVF;
        daisysp::DelayLine<t_sample, 48000> delay_;
        daisysp::Svf filt_svf_;
        daisysp::MoogLadder filt_moog_;
        t_sample cutoff_ = 0.5f;
        t_sample resonance_ = 0;
    };
}


#endif //OL_DSP_DELAY_H
