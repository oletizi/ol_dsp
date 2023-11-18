//
// Created by Orion Letizi on 11/15/23.
//
#include "daisysp.h"
#include "Reverb.h"
#include "ReverbControlPanel.h"

void ol::fx::Reverb::Init(t_sample sample_rate) {
    reverb_->Init(sample_rate);
}

int ol::fx::Reverb::Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) {
    updateReverb();
    int rv = 0;
    rv += reverb_->Process(in1, in2, out1, out2);
    return rv;
}

void ol::fx::Reverb::updateReverb() {
    t_sample time = control_panel_->reverb_time.Value();
    if (time != previous_time_) {
        reverb_->SetFeedback(time);
        previous_time_ = time;
    }
    t_sample cutoff = control_panel_->reverb_cutoff.Value();
    if (cutoff != previous_cutoff_) {
        reverb_->SetLpFreq(cutoff);
        previous_cutoff_ = cutoff;
    }
}

