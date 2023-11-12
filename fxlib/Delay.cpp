//
// Created by Orion Letizi on 11/11/23.
//

#include "ol_fxlib_core.h"
#include "Delay.h"

t_sample ol::fxlib::Delay::Process(t_sample in) {
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
        std::cout << "Delay input, delay out,  filt out: " << delay_input << ", " << out << ", " << filt_out << std::endl;
        counter_ = 0;
    }

    return out;
}

void ol::fxlib::Delay::UpdateCutoff(t_sample cutoff) {
    cutoff_ = cutoff;
}

void ol::fxlib::Delay::UpdateResonance(t_sample resonance) {
    resonance_ = resonance;
}

void ol::fxlib::Delay::UpdateDelayTime(float delay_time) {
    delay_time_ = delay_time;
}

void ol::fxlib::Delay::UpdateFeedback(float feedback) {
    feedback_ = feedback;
}
