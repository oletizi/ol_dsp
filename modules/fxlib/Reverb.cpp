//
// Created by Orion Letizi on 11/15/23.
//
#include "Reverb.h"

void ol::fx::ReverbScWrapper::Init(t_sample sample_rate) {
    reverb_->Init(sample_rate);
}

int ol::fx::ReverbScWrapper::Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) {
    int rv = 0;
    rv += reverb_->Process(in1, in2, out1, out2);
    return rv;
}

void ol::fx::ReverbScWrapper::SetReverbTime(t_sample t60) {
    reverb_->SetFeedback(t60);
}

void ol::fx::ReverbScWrapper::SetCutoff(t_sample cutoff) {
    reverb_->SetLpFreq(cutoff);
}

void ol::fx::ReverbScWrapper::SetEarlyPredelay(t_sample t60) {
    // noop
}

void ol::fx::ReverbScWrapper::SetPredelay(t_sample t60) {
    // noop
}

void ol::fx::ReverbScWrapper::SetPreCutoff(t_sample freq) {
    // noop
}

void ol::fx::ReverbScWrapper::SetInputDiffusion1(t_sample val) {
    // noop
}

void ol::fx::ReverbScWrapper::SetInputDiffusion2(t_sample val) {
    // nop
}

void ol::fx::ReverbScWrapper::SetDecayDiffusion(t_sample vale) {
    // nop
}

void ol::fx::ReverbFx::Init(t_sample sample_rate) {
    reverb_->Init(sample_rate);
}

int ol::fx::ReverbFx::Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) {
    return reverb_->Process(in1, in2, out1, out2);
}


