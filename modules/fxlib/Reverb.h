//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_REVERB_H
#define OL_DSP_REVERB_H

#include "daisysp.h"
#include "verb.h"
#include "corelib/ol_corelib.h"
#include "fxlib/ol_fxlib.h"

namespace ol::fx::reverb {

    class ReverbFx {
    public:
        t_sample sample_rate = 44100;
        t_sample decay_time = 0.5;
        t_sample cutoff = 0.5;
        t_sample early_predelay = 0.1;
        t_sample predelay = 0.2;
        t_sample pre_cutoff = 0.5;
        t_sample input_diffusion1 = 0.5;
        t_sample input_diffusion2 = 0.5;
        t_sample decay_diffusion = 0.5;
        t_sample balance = 0.25;

        void (*Init)(ReverbFx *, t_sample sample_rate) = nullptr;

        int (*Process)(ReverbFx *, const float &in1, const float &in2, float *out1, float *out2) = nullptr;

        void (*Update)(ReverbFx *reverb) = nullptr;

        void *reverb = nullptr;
    };

    /* Call one of these Init functions first.*/
    void ReverbSc_Config(ReverbFx *fx, daisysp::ReverbSc *reverbsc);

    void Dattorro_Config(ReverbFx *fx, sDattorroVerb *verb);

    void UpdateMidi(ReverbFx *fx, uint8_t control, uint8_t value);

}

#endif //OL_DSP_REVERB_H
