//
// Created by Orion Letizi on 12/5/23.
//
#include "synthlib/ol_synthlib.h"
#include "Voice.h"
#include "OscillatorSoundSource.h"


namespace ol::synth {
    void OscillatorSoundSource::Init(t_sample sample_rate) {
        osc_.Init(sample_rate);
    }

    void OscillatorSoundSource::SetFreq(t_sample freq) {
        osc_.SetFreq(freq);
    }

    void OscillatorSoundSource::Process(t_sample *frame) {
        t_sample out = osc_.Process();
        for (int i = 0; i < channel_count_; i++) {
            frame[i] = out;
        }
    }
}
