//
// Created by Orion Letizi on 11/11/23.
//

#include "ol_fxlib_core.h"
#include "Delay.h"

sample_t ol::fxlib::Delay::Process(sample_t in) {
    counter_++;
    sample_t out;
    delay_.SetDelay(delay_time_);
    filt_.SetFreq(cutoff_);
    filt_.SetRes(resonance_);

    out = delay_.Read();
    filt_.Process(out);
    sample_t filt_out = filt_.Low();
    // sample_t delay_input = (feedback_ * out) + in;
    sample_t delay_input = (feedback_ * filt_out) + in;

    delay_.Write(delay_input);
    out = (feedback_ * out) + ((1.0f - feedback_) * in);

    if (counter_ % 30000 == 0) {
        std::cout << "Delay input, delay out,  filt out: " << delay_input << ", " << out << ", " << filt_out << std::endl;
        counter_ = 0;
    }

    return out;
}

void ol::fxlib::Delay::UpdateCutoff(sample_t cutoff) {
    cutoff_ = cutoff;
}

void ol::fxlib::Delay::UpdateResonance(sample_t resonance) {
    resonance_ = resonance;
}

void ol::fxlib::Delay::UpdateDelayTime(float delay_time) {
    delay_time_ = delay_time;
}

void ol::fxlib::Delay::UpdateFeedback(float feedback) {
    feedback_ = feedback;
}
