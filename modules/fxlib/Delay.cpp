//
// Created by Orion Letizi on 11/11/23.
//

#include "Delay.h"

t_sample ol::fx::Delay::Process(t_sample in) {
    profile_->Start();
    profile_->In1(in);
    t_sample out;

    // Update parameters
    if (time_ != cp_->time.Value()) {
        time_ = cp_->time.Value();
        delay_line_->SetDelay(time_);
    }
    if (feedback_ != cp_->feedback.Value()) {
        feedback_ = cp_->feedback.Value();
    }
    if (cutoff_ != cp_->cutoff.Value()) {
        cutoff_ = cp_->cutoff.Value();
        filt_svf_.SetFreq(cutoff_);
    }
    if (resonance_ != cp_->resonance.Value()) {
        resonance_ = cp_->resonance.Value();
        filt_svf_.SetRes(resonance_);
    }
    // assign current delay value to output
    out = delay_line_->Read();

    // prepare feedback
    filt_svf_.Process((feedback_ * out) + in);

    // write feedback into delay
    float delay_input = filt_svf_.Low();
    delay_line_->Write(delay_input);
    profile_->ValA(delay_input);

    //float delay_input = (feedback_ * out) + (in * 0.5f);
    //delay_line_->Write(delay_input);

    if (counter_ % 512 == 0) {
        counter_ = 0;
    }
    counter_++;
    profile_->Out1(out);
    profile_->End();
    return out;
}
