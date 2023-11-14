//
// Created by Orion Letizi on 11/11/23.
//

#include "ol_fxlib_core.h"
#include "Delay.h"

t_sample ol::fx::Delay::Process(t_sample in) {
    counter_++;
    t_sample out;
    delay_.SetDelay(delay_time_);
//    filt_.SetFreq(cutoff_);
//    filt_.SetRes(resonance_);
    updateFilter();

    out = delay_.Read();
    t_sample delay_input = (feedback_ * out) + in;

    filt_svf_.Process(delay_input);
    switch (filter_type_) {
        case SVF:
            delay_input = filt_svf_.Low();
            break;
        case MOOG_LADDER:
            delay_input = filt_moog_.Process(delay_input);
            break;
    }
    //delay_input = filt_svf_.Low();

    delay_.Write(delay_input);

    if (counter_ % 30000 == 0) {
        std::cout << "Filter type: " << filter_type_ << std::endl;
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
