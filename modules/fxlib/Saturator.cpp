//
// Created by Orion Letizi on 11/29/23.
//
#include "fxlib/Fx.h"

namespace ol::fx {
    void Saturator_init(SaturatorFx *fx, t_sample sample_rate) {
        fx->spdata->sr = uint16_t(sample_rate);
        fx->spdata->len = 1;
        fx->spdata->nchan = 1;
        sp_saturator_init(fx->spdata, fx->saturator);
    }

    int Saturator_process(SaturatorFx *fx, const t_sample &in, t_sample *out) {
        t_sample saturator_in = in;
        sp_saturator_compute(fx->spdata, fx->saturator, &saturator_in, out);
        return 0;
    }

    void Saturator_Config(SaturatorFx *fx, sp_saturator *saturator, sp_data *spdata) {
        fx->saturator = saturator;
        fx->spdata = spdata;
        fx->Init = Saturator_init;
        fx->Process = Saturator_process;
    }
}