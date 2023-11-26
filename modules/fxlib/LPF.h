//
// Created by Orion Letizi on 11/13/23.
//

#ifndef OL_DSP_LPF_H
#define OL_DSP_LPF_H

#include "daisysp.h"
#include "corelib/ol_corelib.h"
#include "LpfControlPanel.h"

#define BIQUAD_COUNT 1
namespace ol::fx::filt {
    class FiltFx {
    public:
        t_sample cutoff = 0.5;
        t_sample resonance = 0.2;
        void *filt = nullptr;

        void (*Init)(FiltFx *, t_sample sample_rate) = nullptr;

        int (*Process)(FiltFx *, const float &in, float *out) = nullptr;

        void (*Update)(FiltFx *) = nullptr;
    };

    void UpdateMidi(FiltFx*, uint8_t control, uint8_t value);
    void Biquad_Config(FiltFx *, daisysp::Biquad *);

}

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
        float cutoff_ = 0;
        float resonance_ = 0;
    };
}


#endif //OL_DSP_LPF_H
