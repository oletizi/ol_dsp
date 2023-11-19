//
// Created by Orion Letizi on 11/13/23.
//

#ifndef OL_DSP_LPF_H
#define OL_DSP_LPF_H

#include "daisysp.h"
#include "corelib/ol_corelib.h"
#include "LpfControlPanel.h"

#define BIQUAD_COUNT 1
namespace ol::fx {
    class LPF {
    public:
        explicit LPF(LpfControlPanel *control_panel) : control_panel_(control_panel) {
            for (auto &biquad: biquads_) {
                biquad = daisysp::Biquad();
            }
        }

        void Init(t_sample sample_rate);

        t_sample Process(t_sample in);

    private:
        daisysp::Svf svf_;
        daisysp::MoogLadder moog_ladder_;
        daisysp::Biquad biquads_[BIQUAD_COUNT];
        LpfControlPanel *control_panel_;
    };
}


#endif //OL_DSP_LPF_H
