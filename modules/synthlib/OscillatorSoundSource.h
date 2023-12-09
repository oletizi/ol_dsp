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
        explicit OscillatorSoundSource(daisysp::Oscillator &osc,
                                       const int frame_offset = 0)
                : osc_(osc), frame_offset_(frame_offset) {
        }

        void Init(t_sample sample_rate) override;

        void Process(t_sample *frame) override;

        void SetFreq(t_sample freq) override;

    private:
        daisysp::Oscillator &osc_;
        float freq_ = 0;
        int frame_offset_ = 0;
    };
}
#endif //OL_DSP_OSCILLATORSOUNDSOURCE_H
