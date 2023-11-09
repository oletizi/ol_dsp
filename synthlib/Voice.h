//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_VOICE_H
#define JUCE_TEST_VOICE_H

#include <daisysp.h>
#include "ol_synthlib_core.h"
#include "ControlPanel.h"

namespace ol::synthlib {

    class Voice {
    private:
        t_sample sample_rate_{};
        daisysp::Oscillator osc1_;
        ControlPanel *control_panel_;
    public:
        explicit Voice(ControlPanel *control_panel) : control_panel_(control_panel) {}

        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            osc1_.Init(sample_rate);
        }

        t_sample Process();
    };
}
#endif //JUCE_TEST_VOICE_H
