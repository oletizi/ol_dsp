//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_PITCHEDSOUNDSOURCE_H
#define OL_DSP_PITCHEDSOUNDSOURCE_H

class PitchedSoundSource {
public:
    virtual void Init(t_sample sample_rate) = 0;

    virtual void Process(t_sample *frame) = 0;

    virtual void GateOn() = 0;

    virtual void GateOff() = 0;

    virtual void SetFreq(t_sample freq) = 0;
};
#endif //OL_DSP_PITCHEDSOUNDSOURCE_H
