//
// Created by Orion Letizi on 12/13/23.
//

#ifndef OL_DSP_FILTER_H
#define OL_DSP_FILTER_H

#include "daisysp.h"
#include "corelib/ol_corelib.h"

namespace ol::synth {
    class Filter {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual void SetFreq(t_sample freq) = 0;

        virtual void SetRes(t_sample res) = 0;

        virtual void SetDrive(t_sample drive) = 0;

        virtual void Process(const t_sample *frame_in) = 0;

        virtual void Low(t_sample *frame_out) = 0;

        virtual void High(t_sample *frame_out) = 0;

        virtual void Band(t_sample *frame_out) = 0;

        virtual void Notch(t_sample *frame_out) = 0;

        virtual void Peak(t_sample *frame_out) = 0;
    };

    class MoogFilter : public Filter {
    private:
        daisysp::LadderFilter flt_;
    public:
        void Init(t_sample sample_rate) override { flt_.Init(sample_rate); }

        void SetFreq(t_sample freq) override { flt_.SetFreq(freq); }

        void SetRes(t_sample res) override { flt_.SetRes(res); }

        void Low(t_sample *frame_out) override { *frame_out = flt_.Process(*frame_out); }

        void SetDrive(t_sample drive) override {}

        void Process(const t_sample *frame_in) override {}

        void High(t_sample *frame_out) override {}

        void Band(t_sample *frame_out) override {}

        void Notch(t_sample *frame_out) override {}

        void Peak(t_sample *frame_out) override {}

    private:
        daisysp::LadderFilter moog;
    public:

    };

    class SvfFilter : public Filter {
    private:
        daisysp::Svf svf;
    public:
        void Init(t_sample sample_rate) override {
            svf.Init(sample_rate);
        }

        void SetFreq(t_sample freq) override {
            svf.SetFreq(freq);
        }

        void SetRes(t_sample res) override {
            svf.SetRes(res);
        }

        void SetDrive(t_sample drive) override {
            svf.SetDrive(drive);
        }

        void Process(const t_sample *frame_in) override {
            svf.Process(frame_in[0]);
        }

        void Low(t_sample *frame_out) override {
            frame_out[0] = svf.Low();
        }

        void High(t_sample *frame_out) override {
            frame_out[0] = svf.High();
        }

        void Band(t_sample *frame_out) override {
            frame_out[0] = svf.Band();
        }

        void Notch(t_sample *frame_out) override {
            frame_out[0] = svf.Notch();
        }

        void Peak(t_sample *frame_out) override {
            frame_out[0] = svf.Peak();
        }
    };
}

#endif //OL_DSP_FILTER_H
