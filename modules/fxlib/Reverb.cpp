//
// Created by Orion Letizi on 11/15/23.
//
#include <iostream>
#include "Fx.h"

namespace ol::fx {

    // Dattorro

    sDattorroVerb *Dattorro_get(ReverbFx *reverbfx) {
        return static_cast<sDattorroVerb *>(reverbfx->reverb);
    }

    int Dattorro_Process(ReverbFx *fx, const t_sample &in1, const t_sample &in2, t_sample *out1, float *out2) {
        sDattorroVerb *verb = Dattorro_get(fx);
        DattorroVerb_process(verb,(in1 + in2)/2);
        *out1 = DattorroVerb_getLeft(verb);
        *out2 = DattorroVerb_getRight(verb);
        return 0;
    }

    void Dattorro_Update(ReverbFx *fx) {
        sDattorroVerb *verb = Dattorro_get(fx);
        DattorroVerb_setPreFilter(verb,fx->pre_cutoff);
        DattorroVerb_setInputDiffusion1(verb, fx->input_diffusion1);
        DattorroVerb_setInputDiffusion2(verb, fx->input_diffusion2);
        DattorroVerb_setDecay(verb, fx->decay_time);
        DattorroVerb_setDamping(verb, fx->cutoff);
        DattorroVerb_setDecayDiffusion(verb, fx->decay_diffusion);
    }

    void Dattoro_Init([[maybe_unused]] ReverbFx *fx, [[maybe_unused]] t_sample sample_rate) {
        // nop
    }

    void Dattorro_Config(ReverbFx *fx, sDattorroVerb *reverb) {
        fx->reverb = reverb;
        fx->Init = Dattoro_Init;
        fx->Process = Dattorro_Process;
        fx->Update = Dattorro_Update;
    }


    // ReverbSc

    daisysp::ReverbSc *ReverbSc_get(ReverbFx *reverbfx) {
        return static_cast<daisysp::ReverbSc *>(reverbfx->reverb);
    }

    int ReverbSc_Process(ReverbFx *fx, const float &in1, const float &in2, float *out1, float *out2) {

        int rv = ReverbSc_get(fx)->Process(in1, in2, out1, out2);
        if (*out1 > 1 || *out1 < -1) {
            std::cout << "YIKES!!!!!! This shouldn't happen" << std::endl;
        }
        return rv;
    }

    void ReverbSc_Update(ReverbFx *fx) {
        auto *verb = ReverbSc_get(fx);
        verb->SetFeedback(fx->decay_time);
        verb->SetLpFreq(fx->cutoff);
    }

    void ReverbSc_Init(ReverbFx *fx, t_sample sample_rate) {
        ReverbSc_get(fx)->Init(sample_rate);
    }

    void ReverbSc_Config(ReverbFx *fx, daisysp::ReverbSc *reverb) {
        fx->reverb = reverb;
        fx->Init = ReverbSc_Init;
        fx->Process = ReverbSc_Process;
        fx->Update = ReverbSc_Update;
    }

    void Reverb_UpdateMidiControl(ReverbFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = core::scale(value, 0, 127, 0, 1, 1);
        switch (control) {
            case CC_REVERB_DECAY_DIFFUSION:
                fx->decay_diffusion = scaled;
                break;
            case CC_REVERB_INPUT_DIFFUSION_1:
                fx->input_diffusion1 = scaled;
                break;
            case CC_REVERB_INPUT_DIFFUSION_2:
                fx->decay_diffusion = scaled;
                break;
            case CC_REVERB_CUTOFF:
                fx->cutoff = scaled;
                break;
            case CC_REVERB_BALANCE:
                fx->balance = scaled;
                break;
            case CC_REVERB_PREDELAY:
                fx->predelay = scaled;
                break;
            case CC_EARLY_PREDELAY:
                fx->early_predelay = scaled;
                break;
            case CC_REVERB_PREFILTER:
                fx->pre_cutoff = scaled;
                break;
            case CC_REVERB_TIME:
                fx->decay_time = scaled;
                break;
            default:
                update = false;
                break;
        }
        if (update) {
            fx->Update(fx);
        }
    }
}

