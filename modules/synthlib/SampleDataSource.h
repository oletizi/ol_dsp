//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SAMPLEDATASOURCE_H
#define OL_DSP_SAMPLEDATASOURCE_H

#include "SoundSource.h"

namespace ol::synth {
    class SampleDataSource {
    public:

        virtual uint64_t GetChannelCount() = 0;

        virtual InitStatus Init(t_sample sample_rate, const char * sample_path) = 0;

        virtual void Seek(uint64_t frame_index) = 0;

        /**
         * @param out frame to read into
         * @return frames read
         */
        virtual uint64_t Read(t_sample *out) = 0;
    };
}

#endif //OL_DSP_SAMPLEDATASOURCE_H
