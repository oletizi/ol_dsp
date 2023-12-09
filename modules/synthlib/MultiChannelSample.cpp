//
// Created by Orion Letizi on 12/6/23.
//
#include "corelib/ol_corelib.h"
#include "MultiChannelSample.h"

void ol::synth::MultiChannelSample::Process(t_sample *frame_out) {
    if (playing) {
        uint64_t frames_read = data_source_.Read(frame_out);
        current_frame_ += frames_read;
        if (play_mode_ == Loop) {
            if (!frames_read || (loop_end_ && current_frame_ > loop_end_)) {
                // if:
                // * we're at the end of the sample (!frames_read)
                // * OR, the loop_end_ is set and the current frame is past the loop end
                // * seek back to the loop start
                Seek(loop_start_);
            }
        }
    }
}

void ol::synth::MultiChannelSample::Seek(uint64_t frame_index) {
    data_source_.Seek(frame_index);
    current_frame_ = frame_index;
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

void ol::synth::MultiChannelSample::SetPlayMode(ol::synth::SamplePlayMode mode) {
    play_mode_ = mode;
}

void ol::synth::MultiChannelSample::TogglePlay() {
    playing = !playing;
}

void ol::synth::MultiChannelSample::Play() {
    playing = true;
}

void ol::synth::MultiChannelSample::Pause() {
    playing = false;
}

uint64_t ol::synth::MultiChannelSample::GetChannelCount() {
    return data_source_.GetChannelCount();
}

