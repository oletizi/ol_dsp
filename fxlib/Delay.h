//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_DELAY_H
#define OL_DSP_DELAY_H

#include "ol_corelib.h"
#include "Utility/delayline.h"
#include "Filters/moogladder.h"
#include "Filters/svf.h"
#include "DelayControlPanel.h"

namespace ol::fx {
    class Delay {
    public:
        enum FilterType {
            SVF,
            MOOG_LADDER
        };

        explicit Delay(DelayControlPanel *cp) : cp_(cp) {}

        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            cp_->time.UpdateValueHardware(0.5);
            cp_->feedback.UpdateValueHardware(0.2);
            cp_->cutoff.UpdateValueHardware(0.5);
            cp_->resonance.UpdateValueHardware(0);

            delay_.Init();
            delay_.SetDelay(cp_->time.Value());
            filt_svf_.Init(sample_rate);
            filt_svf_.SetFreq(cp_->cutoff.Value());
            filt_svf_.SetRes(cp_->resonance.Value());
        }
        t_sample Process(t_sample in);

    private:
        uint64_t counter_ = 0;
        t_sample sample_rate_ = 0;
        DelayControlPanel *cp_;
        daisysp::DelayLine<t_sample, 48000> DSY_SDRAM_BSS delay_;
        daisysp::Svf filt_svf_;
        t_sample time_ = 0;
        float feedback_ = 0;
        float cutoff_ = 0;
        float resonance_ = 0;
    };
}
#endif //OL_DSP_DELAY_H
