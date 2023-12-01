//
// Created by Orion Letizi on 11/29/23.
//
#include "fxlib/Fx.h"

namespace ol::fx {
    void Saturator_sp_init(SaturatorFx *fx, t_sample sample_rate) {
        fx->spdata->sr = uint16_t(sample_rate);
        fx->spdata->len = 1;
        fx->spdata->nchan = 1;
        sp_saturator_init(fx->spdata, fx->saturator);
    }

    void Saturator_xfr_init(SaturatorFx *fx, t_sample sample_rate) {
        // nothingto do here. Yet.
    }

    /**
     * The Process() function when using sp_saturator from csound.
     * @param fx
     * @param in
     * @param out
     * @return
     */
    int Saturator_sp_process(SaturatorFx *fx, const t_sample &in, t_sample *out) {
        t_sample saturator_in = in;
        sp_saturator_compute(fx->spdata, fx->saturator, &saturator_in, out);
        return 0;
    }

    /**
     * The Process() function when using a TransferFuction instance.
     * @param fx
     * @param in
     * @param out
     * @return
     */
    int Saturator_xfr_process(SaturatorFx *fx, const t_sample &in, t_sample *out) {
        *out = fx->transferFunction->Process(fx, in);
        return 0;
    }

    void Saturator_sp_update(SaturatorFx *fx) {
        fx->saturator->drive = fx->drive;
    }

    void Saturator_xfr_update(SaturatorFx *fx) {
        // nothing to do here.
    }

    void Saturator_UpdateMidiControl(SaturatorFx *fx, uint8_t control, uint8_t value) {
        bool update = false;
        if (control == CC_SATURATOR_DRIVE) {
            fx->drive = ol::core::scale(value, 0, 127, 1, 128, 1.5);
            update = true;
            fx->Update(fx);
        }
    }

    void Saturator_Config(SaturatorFx *fx, sp_saturator *saturator, sp_data *spdata) {
        fx->saturator = saturator;
        fx->spdata = spdata;
        fx->Init = Saturator_sp_init;
        fx->Process = Saturator_sp_process;
        fx->Update = Saturator_sp_update;
    }

    void Saturator_Config(SaturatorFx *fx, TransferFunction<SaturatorFx *> *xfr) {
        fx->transferFunction = xfr;
        fx->Init = Saturator_xfr_init;
        fx->Process = Saturator_xfr_process;
        fx->Update = Saturator_xfr_update;
    }

    t_sample HyperTan::Process(SaturatorFx *fx, t_sample in) {
        return tanh(fx->drive * in);
    }
}