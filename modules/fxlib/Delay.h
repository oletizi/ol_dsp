//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_DELAY_H
#define OL_DSP_DELAY_H
#define MAX_DELAY 48000

#include "daisysp.h"
#include "corelib/ol_corelib.h"
#include "fxlib/cc_map.h"
#include "Utility/delayline.h"
#include "Filters/moogladder.h"
#include "Filters/svf.h"
#include "DelayControlPanel.h"

namespace ol::fx {
    namespace delay {
        class DelayFx {
        public:
            t_sample time = 0.5;
            t_sample feedback = 0.5;
            t_sample cutoff = 0.5;
            t_sample resonance = 0.2;
            t_sample balance = 0.5;

            void (*Init)(DelayFx *, t_sample sample_rate) = nullptr;

            int (*Process)(DelayFx *, const t_sample &in, t_sample *out) = nullptr;

            void (*Update)(DelayFx *reverb) = nullptr;

            daisysp::DelayLine<t_sample, MAX_DELAY> *delay_line = nullptr;
        };

        void Delay_Config(DelayFx *, daisysp::DelayLine<t_sample, 48000> *);
        void UpdateMidi(DelayFx *, uint8_t control, uint8_t value);
    }
    class Delay {
    public:
        Delay(ol::fx::DelayControlPanel *cp, daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> *delay_line) :
                cp_(cp),
                delay_line_(delay_line) {}

        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            cp_->time.UpdateValueHardware(0.5);
            cp_->feedback.UpdateValueHardware(0.2);
            cp_->cutoff.UpdateValueHardware(0.5);
            cp_->resonance.UpdateValueHardware(0);

            delay_line_->Init();
            delay_line_->SetDelay(cp_->time.Value());
            filt_svf_.Init(sample_rate);
            filt_svf_.SetFreq(cp_->cutoff.Value());
            filt_svf_.SetRes(cp_->resonance.Value());
        }

        t_sample Process(t_sample in);

    private:
        uint64_t counter_ = 0;
        t_sample sample_rate_ = 0;
        DelayControlPanel *cp_;
        daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> *delay_line_;
        daisysp::Svf filt_svf_;
        t_sample time_ = 0;
        float feedback_ = 0;
        float cutoff_ = 0;
        float resonance_ = 0;
        float balance_ = 0;
    };
}
#endif //OL_DSP_DELAY_H
