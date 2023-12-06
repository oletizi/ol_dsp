//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_PITCHEDSOUNDSOURCE_H
#define OL_DSP_PITCHEDSOUNDSOURCE_H
class PitchedSoundSource {
public:
    virtual void Init(t_sample sample_rate) = 0;

    virtual t_sample Process() = 0;

    virtual void SetFreq(t_sample freq) = 0;
};
#endif //OL_DSP_PITCHEDSOUNDSOURCE_H
