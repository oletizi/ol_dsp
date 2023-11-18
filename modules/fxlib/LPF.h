//
// Created by Orion Letizi on 11/13/23.
//

#ifndef OL_DSP_LPF_H
#define OL_DSP_LPF_H

#include <iostream>
#include "daisysp.h"
#include "ol_corelib.h"

#define BIQUAD_COUNT 1
namespace ol::fx {
    class LPF {
    public:
        enum FilterType {
            SVF,
            MOOG_LADDER,
            BIQUAD
        };

        LPF() {
            for (auto &biquad: biquads_) {
                biquad = daisysp::Biquad();
            }
            this->updateFilters();
        }

        void Init(t_sample sample_rate);

        t_sample Process(t_sample in);

        void SetFreq(t_sample frequency);

        void SetRes(t_sample resonance);

        void UpdateFilterType(FilterType type);

    private:
        void updateFilters();

        uint64_t count_ = 0;
        t_sample cutoff_prev_ = 0;
        t_sample cutoff_ = 5000;
        t_sample resonance_prev_ = 0;
        t_sample resonance_ = 0;
        FilterType type_ = SVF;
        daisysp::Svf sfv_;
        daisysp::MoogLadder moog_ladder_;
        daisysp::Biquad biquads_[BIQUAD_COUNT];
    };
}


#endif //OL_DSP_LPF_H
