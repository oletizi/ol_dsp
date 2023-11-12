//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "ol_fxlib_core.h"
#include "Delay.h"
#include "FxControlPanel.h"

namespace ol::fx {
    class FxChain {
    public:
        FxChain(FxControlPanel * control_panel) : control_panel_(control_panel){}
        void Init(t_sample sample_rate) {
            verb_.Init(sample_rate);
            delay_1_.Init(sample_rate);
            delay_2_.Init(sample_rate);
            updateDelays();
        }

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2);

    private:

        /**
         * Update parameters on the delay lines.
         */
        void updateDelays();

        daisysp::ReverbSc verb_;
        Delay delay_1_;
        Delay delay_2_;
        FxControlPanel * control_panel_;
    };

}


#endif //OL_DSP_FXCHAIN_H
