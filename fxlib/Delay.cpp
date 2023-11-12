//
// Created by Orion Letizi on 11/11/23.
//

#include "ol_fxlib_core.h"
#include "Delay.h"

t_sample ol::fx::Delay::Process(t_sample in) {
    counter_++;
    t_sample out;
    delay_.SetDelay(delay_time_);
    filt_.SetFreq(cutoff_);
    filt_.SetRes(resonance_);

    out = delay_.Read();
    filt_.Process(out);
    t_sample filt_out = filt_.Low();
    // t_sample delay_input = (feedback_ * out) + in;
    t_sample delay_input = (feedback_ * filt_out) + in;

    delay_.Write(delay_input);
    out = (feedback_ * out) + ((1.0f - feedback_) * in);

    if (counter_ % 30000 == 0) {
        counter_ = 0;
    }

    return out;
}

void ol::fx::Delay::UpdateCutoff(t_sample cutoff) {
    cutoff_ = cutoff;
}

void ol::fx::Delay::UpdateResonance(t_sample resonance) {
    resonance_ = resonance;
}

void ol::fx::Delay::UpdateDelayTime(float delay_time) {
    delay_time_ = delay_time;
}

void ol::fx::Delay::UpdateFeedback(float feedback) {
    feedback_ = feedback;
}
