//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_OSCILLATORSOUNDSOURCE_H
#define OL_DSP_OSCILLATORSOUNDSOURCE_H

#include <daisysp.h>
#include "synthlib/ol_synthlib.h"

namespace ol::synth {
    class OscillatorSoundSource : public SoundSource<1> {

    private:
        daisysp::Oscillator osc_;
        float freq_ = 0;

    public:

        InitStatus Init(t_sample sample_rate) override {
            osc_.Init(sample_rate);
            osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
            return InitStatus::Ok;
        }

        void Process(t_sample *frame) override {
            frame[0] = osc_.Process();
        }

        inline void GateOn() override {};

        inline void GateOff() override {};

        void SetFreq(t_sample freq) override {
            osc_.SetFreq(freq);
        }

    };
}
#endif //OL_DSP_OSCILLATORSOUNDSOURCE_H
