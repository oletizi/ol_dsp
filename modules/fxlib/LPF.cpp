//
// Created by Orion Letizi on 11/13/23.
//

#include "LPF.h"

using namespace ol::fx;

void LPF::Init(float sample_rate) {
    sfv_.Init(sample_rate);
    moog_ladder_.Init(sample_rate);
    for (auto &biquad: biquads_) {
        biquad.Init(sample_rate);
    }
}

void LPF::SetFreq(float frequency) {
    cutoff_prev_ = cutoff_;
    cutoff_ = frequency;
    updateFilters();
}

void LPF::SetRes(float resonance) {
    resonance_prev_ = resonance_;
    resonance_ = resonance;
    updateFilters();
}

void LPF::UpdateFilterType(LPF::FilterType type) {
    type_ = type;
    updateFilters();
}

t_sample LPF::Process(const t_sample in) {
    t_sample out = in;
    sfv_.Process(in);
    t_sample moog_out = moog_ladder_.Process(in);
    t_sample biquad_out = in;
    for (auto &biquad: biquads_) {
        biquad_out = biquad.Process(biquad_out);
    }
    switch (type_) {
        case SVF:
            out = sfv_.Low();
            break;
        case MOOG_LADDER:
            out = moog_out;
            break;
        case BIQUAD:
            out = biquad_out;
            break;
    }
    if (count_ % 48000 == 0) {
        //std::cout << "LPF: in: " << in << "; out: " << out << std::endl;
        //std::cout << "LPF: cutoff: " << cutoff_ << ", q: " << resonance_ << "; type: " << type_ << std::endl;
        count_ = 0;
    }
    count_++;
    return out;
}

void LPF::updateFilters() {
    if (cutoff_ != cutoff_prev_) {
        sfv_.SetFreq(cutoff_);
        moog_ladder_.SetFreq(cutoff_);
        for (auto &biquad: biquads_) {
            biquad.SetCutoff(cutoff_);
        }
    }
    if (resonance_ != resonance_prev_) {
        sfv_.SetRes(resonance_);
        moog_ladder_.SetRes(resonance_);
        for (auto &biquad: biquads_) {
            biquad.SetRes(resonance_);
        }
    }
}
