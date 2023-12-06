//
// Created by Orion Letizi on 12/4/23.
//

#include "Sample.h"

namespace ol::synth {

    void Sample::Init(const t_sample sample_rate) {}

    t_sample Sample::Process() {
        t_sample out[] = {0,0};
        uint64_t samples_read = sample_source.Read(out);
        return out[frame_offset_];
    }

    void Sample::Seek(uint64_t frame_index) {
        sample_source.Seek(frame_index);
    }

    void Sample::Update() {}

    void Sample::SetLoopStart(uint64_t frame_index) {
        loop_start_ = frame_index;
    }

    void Sample::SetLoopEnd(uint64_t frame_index) {
        loop_end_ = frame_index;
    }
}
