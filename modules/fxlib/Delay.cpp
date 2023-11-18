//
// Created by Orion Letizi on 11/11/23.
//

#include "Delay.h"

t_sample ol::fx::Delay::Process(t_sample in) {
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
    if (balance_ != cp_->balance.Value()) {
        balance_ = cp_->balance.Value();
    }
    // assign current delay value to output
    out = (delay_line_->Read() * balance_) + (in * (1 - balance_));

    // prepare feedback
    filt_svf_.Process(out);
    t_sample delay_input = (feedback_ * filt_svf_.Low()) + in;

    // write feedback into delay
    delay_line_->Write(delay_input);

    if (counter_ % 30000 == 0) {
        counter_ = 0;
    }
    counter_++;
    return out;
}
