//
// Created by Orion Letizi on 12/6/23.
//

#ifndef OL_DSP_MULTICHANNELSAMPLE_H
#define OL_DSP_MULTICHANNELSAMPLE_H

#include "synthlib/ol_synthlib.h"

namespace ol::synth {
    class MultiChannelSample {
    public:
        enum PlayMode {
            OneShot,
            Loop
        };

        MultiChannelSample(SampleDataSource &data_source) : data_source_(data_source) {}

        void Process(t_sample *frame_out);

        void Seek(uint64_t frame_index);

        void Init(t_sample sample_rate);

        void SetLoopStart(uint64_t frame_index);

        void SetLoopEnd(uint64_t frame_index);

        void SetPlayMode(PlayMode mode);

        void TogglePlay();

        void Play();

    private:
        SampleDataSource &data_source_;
        PlayMode play_mode_ = OneShot;
        bool playing = false;
        uint64_t start_ = 0;
        uint64_t end_ = 0;
        uint64_t loop_start_ = 0;
        uint64_t loop_end_ = 0;
        uint64_t current_frame_ = 0;
    };
}

#endif //OL_DSP_MULTICHANNELSAMPLE_H
