//
// Created by Orion Letizi on 11/11/23.
//

#include "Fx.h"

namespace ol::fx {
    void Delay_init(DelayFx *fx, t_sample sample_rate) {
        fx->delay_line->Init();
        fx->filter->Init(fx->filter, sample_rate);
        fx->Update(fx);
    }

    int Delay_process(DelayFx *fx, const t_sample &in, t_sample *out) {
        t_sample delay_out = fx->delay_line->Read();
        t_sample delay_in = in + (fx->feedback * delay_out);
        fx->delay_line->Write(delay_in);
        FilterFx *filter = fx->filter;
        t_sample filter_out = 0;
        filter->Process(filter, delay_out, &filter_out);

        *out = (filter_out * fx->balance) + (in * (1 - fx->balance));
        return 0;
    }

    void Delay_update(DelayFx *fx) {
        // TODO: try using a read index for multi-tap support.
        fx->delay_line->SetDelay(ol::core::scale(fx->time, 0, 1, 0, MAX_DELAY, 1));
        fx->filter->Update(fx->filter);
    }

    void Delay_Config(DelayFx *fx, daisysp::DelayLine<t_sample, MAX_DELAY> *delay_line, FilterFx *filter) {
        fx->delay_line = delay_line;
        fx->filter = filter;
        fx->Init = Delay_init;
        fx->Process = Delay_process;
        fx->Update = Delay_update;
    }

    void Delay_UpdateHardwareControl(DelayFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        switch (control) {
            case CC_DELAY_TIME:
                fx->time = value;
                break;
            case CC_DELAY_FEEDBACK:
                fx->feedback = value;
                break;
            case CC_DELAY_BALANCE:
                fx->balance = value;
                break;
            default:
                update = false;
        }
        Filter_UpdateHardwareControl(fx->filter, control, value);
        if (update) {
            fx->Update(fx);
        }

    }

    void Delay_UpdateMidiControl(DelayFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);

        switch (control) {
            case CC_DELAY_TIME:
                fx->time = scaled;
                break;
            case CC_DELAY_FEEDBACK:
                fx->feedback = scaled;
                break;
            case CC_DELAY_BALANCE:
                fx->balance = scaled;
                break;
            case CC_DELAY_CUTOFF:
                Filter_UpdateMidi(fx->filter, CC_FILTER_CUTOFF, value);
                break;
            case CC_DELAY_RESONANCE:
                Filter_UpdateMidi(fx->filter, CC_FILTER_RESONANCE, value);
                break;
            default:
                update = false;
        }
        //Filter_UpdateMidi(fx->filter, control, value);
        if (update) {
            fx->Update(fx);
        }
    }
}
