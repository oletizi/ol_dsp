//
// Created by Orion Letizi on 12/4/23.
//

#include "Sample.h"

namespace ol::synth {

    void Sample_init(Sample *sample, t_sample sample_rate) {
        sample->sample_rate = sample_rate;
    }
    uint64_t Sample_process(Sample *sample, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
        uint64_t frames_read = 0;
        t_sample output_frame[2] = {0, 0};
        sample->sample_source->Read(sample->sample_source, output_frame, 1, &frames_read);
        *out1 = output_frame[0];
        *out2 = output_frame[1];
        return frames_read;
    }

    void Sample_seek(Sample *sample, uint64_t frame_index) {
        sample->sample_source->Seek(sample->sample_source, frame_index);
    }

    void Sample_update(Sample *sample) {}


    void Sample_Config(Sample *sample, ol::synth::SampleSource *sample_source) {
        sample->sample_source = sample_source;
        sample->Init = Sample_init;
        sample->Seek = Sample_seek;
        sample->Process = Sample_process;
        sample->Update = Sample_update;
    }
}
