//
// Created by Orion Letizi on 11/10/23.
//

#include "Fx.h"

namespace ol::fx {

    void FxRack_init(FxRack *fx, t_sample sample_rate) {
        fx->delay1->Init(fx->delay1, sample_rate);
        fx->delay2->Init(fx->delay2, sample_rate);
        fx->reverb->Init(fx->reverb, sample_rate);
        fx->filter1->Init(fx->filter1, sample_rate);
        fx->filter2->Init(fx->filter2, sample_rate);
    };

    int FxRack_process(FxRack *fx, const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) {
        fx->delay1->Process(fx->delay1, in1, out1);
        fx->delay2->Process(fx->delay2, in2, out2);
        fx->reverb->Process(fx->reverb, *out1, *out2, out1, out2);
        fx->filter1->Process(fx->filter1, *out1, out1);
        fx->filter2->Process(fx->filter2, *out2, out2);
        return 0;
    };

    void FxRack_update(FxRack *fx) {
        fx->delay1->Update(fx->delay1);
        fx->delay2->Update(fx->delay2);
        fx->reverb->Update(fx->reverb);
        fx->filter1->Update(fx->filter1);
        fx->filter2->Update(fx->filter2);
    };

    void FxRack_UpdateMidiControl(FxRack *fx, const uint8_t control, const uint8_t value) {
        Reverb_UpdateMidiControl(fx->reverb, control, value);
        Delay_UpdateMidiControl(fx->delay1, control, value);
        Delay_UpdateMidiControl(fx->delay2, control, value);
        Filter_UpdateMidi(fx->filter1, control, value);
        Filter_UpdateMidi(fx->filter2, control, value);
    }

    void FxRack_Config(FxRack *fx, DelayFx *delay1, DelayFx *delay2, ReverbFx *reverb, FilterFx *filter1,
                       FilterFx *filter2) {
        fx->delay1 = delay1;
        fx->delay2 = delay2;
        fx->reverb = reverb;
        fx->filter1 = filter1;
        fx->filter2 = filter2;
        fx->Init = FxRack_init;
        fx->Update = FxRack_update;
        fx->Process = FxRack_process;
    }
}