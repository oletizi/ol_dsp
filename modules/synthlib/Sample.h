//
// Created by Orion Letizi on 12/4/23.
//

#ifndef OL_DSP_SAMPLE_H
#define OL_DSP_SAMPLE_H

#include "SampleDataSource.h"

namespace ol::synth {

    class Sample {

    public:
        /**
         * Mono sample constructor. Assumes the
         * @param s
         */
        explicit Sample(SampleDataSource &s) : Sample(s, 0) {}

        /**
         * Multi-channel sample constructor. Will read from the nth sample in every frame, where n=frame_offset.
         * @param s
         * @param frame_offset
         */
        Sample(SampleDataSource &s, uint64_t frame_offset) : sample_source(s),
                                                             frame_offset_(frame_offset) {}

        void Init(t_sample sample_rate);

        void Update();

        void Seek(uint64_t frame_index);

        t_sample Process();

        void SetLoopStart(uint64_t frame_index);

        void SetLoopEnd(uint64_t frame_index);

    private:
        SampleDataSource &sample_source;
        t_sample sample_rate_ = 0;
        t_sample start = 0;
        t_sample end = 0;
        uint64_t loop_start_ = -1;
        uint64_t loop_end_ = -1;
        uint64_t frame_offset_ = 0;
        uint64_t channel_count_;
    };
}


#endif //OL_DSP_SAMPLE_H
