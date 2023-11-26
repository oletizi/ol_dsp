//
// Created by Orion Letizi on 11/15/23.
//

#include "Reverb.h"

namespace ol::fx::reverb {


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

    void Dattoro_Init(ReverbFx *fx, t_sample sample_rate) {
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

    int ReverbSc_Process(ReverbFx *reverbfx, const float &in1, const float &in2, float *out1, float *out2) {
        return ReverbSc_get(reverbfx)->Process(in1, in2, out1, out2);
    }

    void ReverbSc_Update(ReverbFx *reverbfx) {
        auto *verb = ReverbSc_get(reverbfx);
        verb->SetFeedback(reverbfx->decay_time);
        verb->SetLpFreq(reverbfx->cutoff);
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
}

