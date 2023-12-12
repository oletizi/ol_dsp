//
// Created by Orion Letizi on 12/6/23.
//
#include "corelib/ol_corelib.h"
#include "Sample.h"
#include "SoundSource.h"

namespace ol::synth {
    void ol::synth::Sample::Process(t_sample *frame_out) {
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

    void ol::synth::Sample::Seek(uint64_t frame_index) {
        data_source_.Seek(frame_index);
        current_frame_ = frame_index;
    }

    InitStatus ol::synth::Sample::Init(t_sample sample_rate) {
        sample_rate = sample_rate;
        //return data_source_.Init(sample_rate);
    }

    void ol::synth::Sample::SetLoopStart(uint64_t frame_index) {
        loop_start_ = frame_index;
    }

    void ol::synth::Sample::SetLoopEnd(uint64_t frame_index) {
        loop_end_ = frame_index;
    }

    void ol::synth::Sample::SetPlayMode(ol::synth::SamplePlayMode mode) {
        play_mode_ = mode;
    }

    void ol::synth::Sample::TogglePlay() {
        playing = !playing;
    }

    void ol::synth::Sample::Play() {
        playing = true;
    }

    void ol::synth::Sample::Pause() {
        playing = false;
    }

    uint64_t ol::synth::Sample::GetChannelCount() {
        return data_source_.GetChannelCount();
    }
}
