//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_FX_H
#define OL_DSP_FX_H

#include "ol_corelib.h"

namespace ol::fx {
    class Fx {
    public:
        virtual void Init(t_sample sample_rate) = 0;
        virtual int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) = 0;
    };
}
#endif //OL_DSP_FX_H
