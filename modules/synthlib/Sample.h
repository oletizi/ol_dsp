//
// Created by Orion Letizi on 12/4/23.
//

#ifndef OL_DSP_SAMPLE_H
#define OL_DSP_SAMPLE_H

#include "synthlib/ol_synthlib.h"

namespace ol::synth {

    struct SampleSource {
        typedef
        void (*SeekFunction)(void *sample_source, const uint64_t frame_index);

        typedef
        void (*ReadFunction)(void *sample_source, t_sample *frames_out, const uint64_t frame_count,
                             uint64_t *frames_read);

        SeekFunction Seek = nullptr;
        ReadFunction Read = nullptr;
    };

    struct Sample {
        void (*Init)(Sample *, t_sample sample_rate) = nullptr;

        void (*Update)(Sample *) = nullptr;

        void (*Seek)(Sample *, uint64_t frame_index) = nullptr;

        // XXX: I don't love that the return value means something else here than elsewhere.
        uint64_t (*Process)(Sample *, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) = nullptr;

        SampleSource *sample_source = nullptr;
        t_sample sample_rate = 0;
        t_sample start = 0;
        t_sample end = 0;
    };

    void Sample_Config(Sample *, SampleSource *);
}


#endif //OL_DSP_SAMPLE_H
