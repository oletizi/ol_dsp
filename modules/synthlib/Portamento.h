//
// Created by Orion Letizi on 12/13/23.
//

#ifndef OL_DSP_PORTAMENTO_H
#define OL_DSP_PORTAMENTO_H

#include "daisysp.h"
#include "corelib/ol_corelib.h"

// Simple portamento/smoothing implementation to replace missing daisysp::Port
namespace daisysp {
    class Port {
    private:
        float sample_rate_;
        float htime_;
        float coeff_;
        float z1_;

    public:
        Port() : sample_rate_(48000.0f), htime_(0.01f), coeff_(0.0f), z1_(0.0f) {}

        void Init(float sample_rate, float htime) {
            sample_rate_ = sample_rate;
            SetHtime(htime);
            z1_ = 0.0f;
        }

        float Process(float in) {
            z1_ = in + coeff_ * (z1_ - in);
            return z1_;
        }

        void SetHtime(float htime) {
            htime_ = htime;
            coeff_ = expf(-1.0f / (htime_ * sample_rate_));
        }

        float GetHtime() const {
            return htime_;
        }
    };
}

namespace ol::synth {
    class Portamento {
    public:
        virtual void Init(t_sample sample_rate, t_sample htime) = 0;

        virtual t_sample Process(t_sample in) = 0;

        virtual void SetHtime(t_sample htime) = 0;

        virtual t_sample GetHtime() = 0;
    };

    class DaisyPortamento : public Portamento {
    private:
        daisysp::Port port_;
    public:
        void Init(t_sample sample_rate, t_sample htime) override {
            port_.Init(sample_rate, htime);
        }

        t_sample Process(t_sample in) override {
            return port_.Process(in);
        }

        void SetHtime(t_sample htime) override {
            port_.SetHtime(htime);
        }

        t_sample GetHtime() override {
            return port_.GetHtime();
        }
    };
}
#endif //OL_DSP_PORTAMENTO_H
