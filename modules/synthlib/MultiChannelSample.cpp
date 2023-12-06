//
// Created by Orion Letizi on 12/6/23.
//

#include "MultiChannelSample.h"

void ol::synth::MultiChannelSample::Process(t_sample *frame_out) {
    data_source_.Read(frame_out);
}

void ol::synth::MultiChannelSample::Seek(uint64_t frame_index) {
    data_source_.Seek(frame_index);
}

void ol::synth::MultiChannelSample::Init(t_sample sample_rate) {
    sample_rate = sample_rate;
}

void ol::synth::MultiChannelSample::SetLoopStart(uint64_t frame_index) {
    loop_start_ = frame_index;
}

void ol::synth::MultiChannelSample::SetLoopEnd(uint64_t frame_index) {
    loop_end_ = frame_index;
}
