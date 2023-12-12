//
// Created by Orion Letizi on 12/5/23.
//
#ifndef OL_DSP_SAMPLESOUNDSOURCE_H
#define OL_DSP_SAMPLESOUNDSOURCE_H

#include "synthlib/ol_synthlib.h"
#include "Sample.h"

namespace ol::synth {

    template<int CHANNEL_COUNT>
    class SampleSoundSource : public SoundSource {
    public:
        explicit SampleSoundSource(Sample *s) : sample_(s), freq_(0) {}

        void Process(t_sample *frame_out) override {
            sample_->Process(frame_out);
        }

        inline void GateOn() override {
            sample_->Seek(0);
            sample_->Play();
        }

        void GateOff() override { sample_->Pause(); }

        void SetFreq(t_sample freq) override {
            freq_ = freq;
        }

        InitStatus Init(t_sample sample_rate) override {
            return sample_->Init(sample_rate);
        }

    private:
        Sample *sample_;
        t_sample freq_;
        t_sample *frame_buffer_[CHANNEL_COUNT] = {};
    };

}

#endif //OL_DSP_SAMPLESOUNDSOURCE_H
