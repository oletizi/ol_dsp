//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_REVERB_H
#define OL_DSP_REVERB_H

#include "ol_corelib.h"
#include "ReverbControlPanel.h"
#include "Fx.h"

namespace ol::fx {
    class Reverb : public Fx {
    public:
        explicit Reverb(ol::fx::ReverbControlPanel *control_panel) : control_panel_(control_panel) {};

        void Init(t_sample sample_rate) override;

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) override;

    private:
        void updateReverb();

        daisysp::ReverbSc verb_;
        ReverbControlPanel *control_panel_;
        t_sample previous_time_ = 0;
        t_sample previous_cutoff_ = 0;
        t_sample previous_balance_ = 0;
    };
}

#endif //OL_DSP_REVERB_H
