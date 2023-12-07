//
// Created by Orion Letizi on 12/5/23.
//
#include "SampleSoundSource.h"

namespace ol::synth {

    void SampleSoundSource::SetFreq(t_sample freq) {
        freq_ = freq;
    }

    void SampleSoundSource::Process(t_sample *frame_out) {
        sample_.Process(frame_buffer_);
    }

    void SampleSoundSource::Init(t_sample sample_rate) {
        sample_.Init(sample_rate);
    }

}
