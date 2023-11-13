//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCHAIN_H
#define OL_DSP_FXCHAIN_H

#include "ol_fxlib_core.h"
#include "Delay.h"
#include "FxControlPanel.h"

#define DELAY_COUNT 2
namespace ol::fx {
    class FxChain {
    public:
        explicit FxChain(FxControlPanel *control_panel) : control_panel_(control_panel) {}

        void Init(t_sample sample_rate) {
            verb_.Init(sample_rate);
            for (auto &delay: delays) {
                delay = Delay();
                delay.Init(sample_rate);
            }
            updateDelays();
        }

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2);

    private:

        /**
         * Update parameters on the delay lines.
         */
        void updateDelays();

        daisysp::ReverbSc verb_;
        Delay delays[DELAY_COUNT];
        FxControlPanel *control_panel_;
        uint64_t counter_ = 0;
    };

}


#endif //OL_DSP_FXCHAIN_H
