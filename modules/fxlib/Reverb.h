//
// Created by Orion Letizi on 12/16/23.
//

#ifndef OL_DSP_REVERB_H
#define OL_DSP_REVERB_H

#include "daisysp.h"
#include "corelib/ol_corelib.h"

namespace ol::fx {
    class Reverb {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual void Process(const t_sample *frame_in, t_sample *frame_out) = 0;

        virtual void SetPredelay(t_sample) = 0;

        virtual void SetPrefilter(t_sample) = 0;

        virtual void SetEarlyPredelay(t_sample) = 0;

        virtual void SetInputDiffusion1(t_sample) = 0;

        virtual void SetInputDiffusion2(t_sample) = 0;

        virtual void SetDecayDiffusion(t_sample) = 0;

        virtual void SetCutoff(t_sample) = 0;

        virtual void SetTime(t_sample) = 0;
    };

    template<int CHANNEL_COUNT>
    class DaisyVerb : public Reverb {
    private:
        daisysp::ReverbSc verb;
        t_sample in1 = 0;
        t_sample in2 = 0;
        t_sample out1 = 0;
        t_sample out2 = 0;
    public:
        void Init(t_sample sample_rate) override {
            verb.Init(sample_rate);
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) override {
            in1 = frame_in[0];
            in2 = CHANNEL_COUNT > 1 ? frame_in[1] : in1;
            out1 = out2 = 0;
            verb.Process(in1, in2, &out1, &out2);
            if (CHANNEL_COUNT > 1) {
                frame_out[0] = out1;
                frame_out[2] = out2;
            } else {
                *frame_out = out1 + out2 / 2;
            }
        }

        void SetPredelay(t_sample sample) override {

        }

        void SetPrefilter(t_sample sample) override {

        }

        void SetEarlyPredelay(t_sample sample) override {

        }

        void SetInputDiffusion1(t_sample sample) override {

        }

        void SetInputDiffusion2(t_sample sample) override {

        }

        void SetDecayDiffusion(t_sample sample) override {

        }

        void SetCutoff(t_sample freq) override {
            verb.SetLpFreq(freq);
        }

        void SetTime(t_sample time) override {
            verb.SetFeedback(time);
        }
    };
}

#endif //OL_DSP_REVERB_H
