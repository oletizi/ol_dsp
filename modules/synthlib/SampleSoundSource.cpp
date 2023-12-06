//
// Created by Orion Letizi on 12/5/23.
//
#include "synthlib/ol_synthlib.h"
#include "SampleSoundSource.h"

namespace ol::synth {

    void SampleSoundSource::Init(t_sample sample_rate) {
        sample_.Init(sample_rate);
    }

    void SampleSoundSource::SetFreq(t_sample freq) {
        freq_ = freq;
    }

   t_sample SampleSoundSource::Process() {
        // TODO: modify playback speed based on frequency.
        return sample_.Process();
    }

}
