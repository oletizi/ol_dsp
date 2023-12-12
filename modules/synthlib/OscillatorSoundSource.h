//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_OSCILLATORSOUNDSOURCE_H
#define OL_DSP_OSCILLATORSOUNDSOURCE_H

#include <daisysp.h>
#include "synthlib/ol_synthlib.h"

namespace ol::synth {
    template<int CHANNEL_COUNT>
    class OscillatorSoundSource : public SoundSource {

    private:
        daisysp::Oscillator &osc_;
        float freq_ = 0;

    public:
        explicit OscillatorSoundSource(daisysp::Oscillator &osc)
                : osc_(osc) {
        }

        InitStatus Init(t_sample sample_rate) override {
            osc_.Init(sample_rate);
            osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
            return InitStatus::Ok;
        }

        void Process(t_sample *frame) override {
            t_sample out = osc_.Process();
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame[i] = out;
            }
        }

        inline void GateOn() override {};

        inline void GateOff() override {};

        void SetFreq(t_sample freq) override {
            osc_.SetFreq(freq);
        }

    };
}
#endif //OL_DSP_OSCILLATORSOUNDSOURCE_H
