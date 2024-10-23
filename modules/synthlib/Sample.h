//
// Created by Orion Letizi on 12/6/23.
//

#ifndef OL_DSP_SAMPLE_H
#define OL_DSP_SAMPLE_H

#include "SampleDataSource.h"

namespace ol::synth {
    enum SamplePlayMode {
        OneShot,
        Loop
    };

    class Sample {
    public:

        explicit Sample(SampleDataSource &data_source) : data_source_(data_source) {}

        void Process(t_sample *frame_out);

        void Seek(uint64_t frame_index);

        InitStatus Init(t_sample sample_rate);

        void SetLoopStart(uint64_t frame_index);

        void SetLoopEnd(uint64_t frame_index);

        void SetPlayMode(SamplePlayMode mode);

        void TogglePlay();

        void Play();

        void Pause();

        uint64_t GetChannelCount();

    private:
        SampleDataSource &data_source_;
        SamplePlayMode play_mode_ = OneShot;
        bool playing = false;
        uint64_t start_ = 0;
        uint64_t end_ = 0;
        uint64_t loop_start_ = 0;
        uint64_t loop_end_ = 0;
        uint64_t current_frame_ = 0;
        t_sample sample_rate_ = 0;
    };
}

#endif //OL_DSP_SAMPLE_H
