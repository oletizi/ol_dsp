//
// Created by Orion Letizi on 11/13/23.
//

#include "Fx.h"

namespace ol::fx {

    // Biquad
    daisysp::Biquad *Biquad_get(FiltFx *fx) {
        return static_cast<daisysp::Biquad *>(fx->filt);
    }

    int Biquad_process(FiltFx *fx, const float &in, float *out) {
        auto filter = Biquad_get(fx);
        *out = filter->Process(in);
        return 0;
    }

    void Biquad_update(FiltFx *fx) {
        daisysp::Biquad *filt = Biquad_get(fx);
        filt->SetCutoff(fx->cutoff);
        filt->SetRes(fx->resonance);
    }

    void Biquad_init(FiltFx *fx, t_sample sample_rate) {
        Biquad_get(fx)->Init(sample_rate);
        Biquad_update(fx);
    }

    void Filter_Biquad_Config(FiltFx *fx, daisysp::Biquad *filt) {
        fx->filt = filt;
        fx->Init = Biquad_init;
        fx->Process = Biquad_process;
        fx->Update = Biquad_update;
    }

    // SvF
    daisysp::Svf *Svf_get(FiltFx *fx) {
        return static_cast<daisysp::Svf *>(fx->filt);
    }

    int Svf_process(FiltFx *fx, const float &in, float *out) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->Process(in);
        // TODO: add output select based on filter type parameter
        *out = filt->Low();
        return 0;
    }

    void Svf_update(FiltFx *fx) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->SetRes(fx->resonance);
        filt->SetFreq(ol::core::scale(fx->cutoff, 0, 1, 0, 20000, 1));
        filt->SetDrive(fx->drive);
    }

    void Svf_init(FiltFx *fx, t_sample sample_rate) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->Init(sample_rate);
        Svf_update(fx);
    }

    void Filter_Svf_Config(FiltFx *fx, daisysp::Svf *filt) {
        fx->filt = filt;
        fx->Init = Svf_init;
        fx->Process = Svf_process;
        fx->Update = Svf_update;
    }

    void Filter_UpdateMidi(FiltFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
        switch (control) {
            case CC_LPF_RESONANCE:
                fx->resonance = scaled;
                break;
            case CC_LPF_CUTOFF:
                fx->cutoff = scaled;
                break;
            case CC_LPF_DRIVE:
                fx->drive = scaled;
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
