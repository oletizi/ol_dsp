//
// Created by Orion Letizi on 11/13/23.
//

#include "LPF.h"

using namespace ol::fx;

namespace ol::fx::filt {

    daisysp::Biquad *Biquad_get(FiltFx *fx) {
        return static_cast<daisysp::Biquad *>(fx->filt);
    }

    void Biquad_init(FiltFx *fx, t_sample sample_rate) {
        Biquad_get(fx)->Init(sample_rate);
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

    void UpdateMidi(FiltFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
        switch (control) {
            case CC_LPF_RESONANCE:
                fx->resonance = scaled;
                break;
            case CC_LPF_CUTOFF:
                fx->cutoff = scaled;
                break;
            default:
                update = false;
                break;
        }
        if (update) {
            fx->Update(fx);
        }
    }

    void Biquad_Config(FiltFx *fx, daisysp::Biquad *filt) {
        fx->filt = filt;
        fx->Init = Biquad_init;
        fx->Process = Biquad_process;
        fx->Update = Biquad_update;
    }
}

void LPF::Init(float sample_rate) {
    svf_.Init(sample_rate);
    moog_ladder_.Init(sample_rate);
    for (auto &biquad: biquads_) {
        biquad.Init(sample_rate);
    }
}

t_sample LPF::Process(const t_sample in) {

    t_sample out = in;

    t_sample cutoff = control_panel_->cutoff.Value();
    t_sample resonance = control_panel_->resonance.Value();
    if (cutoff_ != cutoff) {
        cutoff_ = cutoff;
        svf_.SetFreq(cutoff_);
        moog_ladder_.SetFreq(cutoff_);
        for (auto &biquad: biquads_) {
            biquad.SetCutoff(cutoff_);
        }
    }

    if (resonance_ != resonance) {
        resonance_ = resonance;
        svf_.SetRes(resonance_);
        moog_ladder_.SetRes(resonance_);
        for (auto &biquad: biquads_) {
            biquad.SetRes(resonance_);
        }
    }

    svf_.Process(in);
    t_sample moog_out = moog_ladder_.Process(in);
    t_sample biquad_out = in;
    for (auto &biquad: biquads_) {
        biquad_out = biquad.Process(biquad_out);
    }

    t_sample type_val = control_panel_->type.Value();

    if (type_val > 0.6) {
        out = biquad_out;
    } else if (type_val > 0.3) {
        out = moog_out;
    } else {
        out = svf_.Low();
    }

    return out;
}

