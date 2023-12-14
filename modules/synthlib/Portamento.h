//
// Created by Orion Letizi on 12/13/23.
//

#ifndef OL_DSP_PORTAMENTO_H
#define OL_DSP_PORTAMENTO_H

#include "Utility/port.h"
#include "corelib/ol_corelib.h"
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
