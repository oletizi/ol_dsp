//
// Created by Orion Letizi on 12/5/23.
//
#ifndef OL_DSP_SAMPLESOUNDSOURCE_H
#define OL_DSP_SAMPLESOUNDSOURCE_H

#include "synthlib/ol_synthlib.h"
#include "MultiChannelSample.h"

namespace ol::synth {

    class SampleSoundSource : public PitchedSoundSource {
    public:
        SampleSoundSource(MultiChannelSample &s) : sample_(s), freq_(0) {}

        void Init(t_sample sample_rate) override;

        void Process(t_sample *frame) override;

        inline void GateOn() override {
            sample_.Seek(0);
            sample_.Play();
        }

        void GateOff() override { sample_.Pause(); }

        void SetFreq(t_sample freq) override;


    private:
        MultiChannelSample &sample_;
        t_sample freq_;
        t_sample *frame_buffer_;
    };

}

#endif //OL_DSP_SAMPLESOUNDSOURCE_H
