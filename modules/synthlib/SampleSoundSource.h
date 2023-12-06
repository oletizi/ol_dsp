//
// Created by Orion Letizi on 12/5/23.
//
#ifndef OL_DSP_SAMPLESOUNDSOURCE_H
#define OL_DSP_SAMPLESOUNDSOURCE_H

#include "PitchedSoundSource.h"
#include "Sample.h"

namespace ol::synth {
    class SampleSoundSource : public PitchedSoundSource {
    public:
        SampleSoundSource(Sample &s) : sample_(s), freq_(0) {}

        void Init(t_sample sample_rate) override;

        t_sample Process() override;

        void SetFreq(t_sample freq) override;

    private:
        Sample sample_;
        t_sample freq_;
    };
}

#endif //OL_DSP_SAMPLESOUNDSOURCE_H
