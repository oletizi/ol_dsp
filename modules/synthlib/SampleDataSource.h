//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SAMPLEDATASOURCE_H
#define OL_DSP_SAMPLEDATASOURCE_H

#include "corelib/ol_corelib.h"

namespace ol::synth {
    class SampleDataSource {
    public:

        virtual uint64_t GetChannelCount() = 0;

        virtual void Seek(uint64_t frame_index) = 0;

        /**
         * @param out frame to read into
         * @return frames read
         */
        virtual uint64_t Read(t_sample *out) = 0;
    };
}

#endif //OL_DSP_SAMPLEDATASOURCE_H
