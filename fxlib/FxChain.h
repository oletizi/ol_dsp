//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "Delay.h"
#include "LPF.h"
#include "FxControlPanel.h"
#include "Effects/reverbsc.h"
#include "Fx.h"

#define CHANNEL_COUNT 2
namespace ol::fx {
    class FxChain : public Fx {
    public:
        explicit FxChain(FxControlPanel *control_panel) :
                control_panel_(control_panel),
                delay1_(Delay(control_panel->delay_control_)),
                delay2_((Delay(control_panel->delay_control_))){}

        void Init(t_sample sample_rate) override {
            verb_.Init(sample_rate);
            delay1_.Init(sample_rate);
            delay2_.Init(sample_rate);
            for (auto &lpf: lpfs_) {
                lpf.Init(sample_rate);
            }
            updateLpfs();
        }

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) override;

    private:

        /**
         * Update parameters on the delay lines.
         */
        void updateLpfs();

        daisysp::ReverbSc verb_;
        Delay delay1_;
        Delay delay2_;
        LPF lpfs_[CHANNEL_COUNT];
        FxControlPanel *control_panel_;
        uint64_t counter_ = 0;
    };

}


#endif //OL_DSP_FXCHAIN_H
