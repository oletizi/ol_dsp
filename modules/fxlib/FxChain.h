//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "Delay.h"
#include "FxControlPanel.h"
#include "Effects/reverbsc.h"
#include "Fx.h"
#include "Reverb.h"
#include "LPF.h"

namespace ol::fx {
    class FxChain : public Fx {
    public:
        explicit FxChain(FxControlPanel *control_panel, Delay *delay1, Delay *delay2, Reverb *reverb, LPF *lpf1, LPF *lpf2) :
                control_panel_(control_panel),
                delay1_(delay1),
                delay2_(delay2),
                verb_(reverb),
                lpf1_(lpf1),
                lpf2_(lpf2)
                        {}

        void Init(t_sample sample_rate) override {
            verb_->Init(sample_rate);
            delay1_->Init(sample_rate);
            delay2_->Init(sample_rate);
            lpf1_->Init(sample_rate);
            lpf2_->Init(sample_rate);
        }

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) override;

    private:

        Reverb *verb_;
        Delay *delay1_;
        Delay *delay2_;
        FxControlPanel *control_panel_;
        uint64_t counter_ = 0;
        LPF *lpf1_;
        LPF *lpf2_;
    };

}


#endif //OL_DSP_FXCHAIN_H
