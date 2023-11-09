//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_VOICE_H
#define JUCE_TEST_VOICE_H
#include <daisysp.h>
#include "ol_synthlib_core.h"

namespace ol::synthlib {

    class Voice {
    private:
        t_sample sample_rate_;
        daisysp::Oscillator osc1_;
    public:
        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            osc1_.Init(sample_rate);
        }
        inline void SetFrequency(t_sample frequency) {
            osc1_.SetFreq(frequency);
        }
        t_sample Process();
    };
}
#endif //JUCE_TEST_VOICE_H
