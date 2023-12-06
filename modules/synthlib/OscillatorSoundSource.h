//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_OSCILLATORSOUNDSOURCE_H
#define OL_DSP_OSCILLATORSOUNDSOURCE_H

#include <daisysp.h>
#include "synthlib/ol_synthlib.h"

namespace ol::synth {
    class OscillatorSoundSource : public PitchedSoundSource {
    public:
        explicit OscillatorSoundSource(const daisysp::Oscillator &osc) : osc_(osc) {};

        void Init(t_sample sample_rate) override;

        t_sample Process() override;

        void SetFreq(t_sample freq) override;

    private:
        daisysp::Oscillator osc_;
        float freq_ = 0;
    };
}
#endif //OL_DSP_OSCILLATORSOUNDSOURCE_H
