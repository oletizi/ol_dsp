//
// Created by Orion Letizi on 12/4/23.
//

#ifndef OL_DSP_SAMPLE_H
#define OL_DSP_SAMPLE_H

#include "SampleDataSource.h"

namespace ol::synth {

    class Sample {

    public:
        explicit Sample(SampleDataSource &s) : sample_source(s) {}

        void Init(t_sample sample_rate);

        void Update();

        void Seek(uint64_t frame_index);

        t_sample Process();

    private:
        SampleDataSource &sample_source;
        t_sample sample_rate_ = 0;
        t_sample start = 0;
        t_sample end = 0;
    };
}


#endif //OL_DSP_SAMPLE_H
