//
// Created by Orion Letizi on 11/11/23.
//

#include "Delay.h"

t_sample ol::fx::Delay::Process(t_sample in) {
    t_sample out;

    // Update parameters
    if (time_ != cp_->time.Value()) {
        time_ = cp_->time.Value();
        delay_.SetDelay(time_);
    }
    if (feedback_ != cp_->feedback.Value()) {
        feedback_ = cp_->feedback.Value();
    }
    if (cutoff_ != cp_->cutoff.Value()) {
        cutoff_ = cp_ ->cutoff.Value();
        filt_svf_.SetFreq(cutoff_);
    }
    if (resonance_ != cp_->resonance.Value()) {
        resonance_ = cp_->resonance.Value();
        filt_svf_.SetRes(resonance_);
    }
    // assign current delay value to output
    out = delay_.Read();

    // prepare feedback
    filt_svf_.Process(out);
    t_sample delay_input = (feedback_* filt_svf_.Low()) + in;

    // write feedback into delay
    delay_.Write(delay_input);

    if (counter_ % 30000 == 0) {
        counter_ = 0;
    }
    counter_++;
    return out;
}
