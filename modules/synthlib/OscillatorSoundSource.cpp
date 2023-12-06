//
// Created by Orion Letizi on 12/5/23.
//
#include "synthlib/ol_synthlib.h"
#include "Voice.h"


namespace ol::synth {

    void OscillatorSoundSource::Init(t_sample sample_rate) {
        osc_.Init(sample_rate);
    }

    void OscillatorSoundSource::SetFreq(t_sample freq) {
        osc_.SetFreq(freq);
    }

    t_sample OscillatorSoundSource::Process() {
        return osc_.Process();
    }

}
