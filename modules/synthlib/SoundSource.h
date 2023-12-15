//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SOUNDSOURCE_H
#define OL_DSP_SOUNDSOURCE_H

namespace ol::synth {
    enum InitStatus {
        Ok, Error
    };

    template<int SOUND_SOURCE>
    class SoundSource {
    public:


        virtual InitStatus Init(t_sample sample_rate) = 0;

        virtual void Process(t_sample *frame) = 0;

        virtual void GateOn() = 0;

        virtual void GateOff() = 0;

        virtual void SetFreq(t_sample freq) = 0;
    };
}
#endif //OL_DSP_SOUNDSOURCE_H
