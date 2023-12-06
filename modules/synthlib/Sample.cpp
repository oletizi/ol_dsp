//
// Created by Orion Letizi on 12/4/23.
//

#include "Sample.h"

namespace ol::synth {

    void Sample::Init(const t_sample sample_rate) {}

    t_sample Sample::Process() {
        return sample_source.Read();
    }

    void Sample::Seek(uint64_t frame_index) {
        sample_source.Seek(frame_index);
    }

    void Sample::Update() {}
}
