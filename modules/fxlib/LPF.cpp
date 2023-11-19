//
// Created by Orion Letizi on 11/13/23.
//

#include "LPF.h"

using namespace ol::fx;

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