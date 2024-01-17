//
// Created by Orion Letizi on 1/15/24.
//

#ifndef OL_DSP_SYNTHENGINE_H
#define OL_DSP_SYNTHENGINE_H

#include "ctllib/ol_ctllib.h"
#include "fxlib/ol_fxlib.h"
#include "synthlib/ol_synthlib.h"

using namespace ol::ctl;

namespace ol::app::synth {
    // XXX: This probably belongs in ol_teensy


    template<int CHANNEL_COUNT>
    class SynthEngine {
    public:
        SynthEngine(ol::synth::Voice &voice, ol::fx::FxRack<CHANNEL_COUNT> &fxrack) : voice_(voice),
                                                                                      fxrack_(fxrack) {}

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            t_sample voice_out = 0;
            voice_.Process(&voice_out);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                // mix input with synth output
                // frame_buffer[i] += frame_in[i];
                frame_out[i] = voice_out;
            }
            // fxrack_.Process(frame_buffer, frame_out);

        }

        void Init(t_sample sample_rate) {
            voice_.Init(sample_rate);
            fxrack_.Init(sample_rate);
        }

    private:
        ol::synth::Voice &voice_;
        ol::fx::FxRack<CHANNEL_COUNT> &fxrack_;
    };

}

#endif //OL_DSP_SYNTHENGINE_H
