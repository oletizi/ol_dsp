//
// Created by Orion Letizi on 11/11/23.
//

#include "Fx.h"

namespace ol::fx {
    void DelayFx::Init(const t_sample sample_rate) {
        delay_line.Init();
        filter.Init(sample_rate);
        Update();
    }

    void DelayFx::Process(const t_sample *frame_in, t_sample *frame_out) {
        t_sample delay_out = delay_line.Read();
        t_sample delay_in = frame_in[frame_offset_] + (feedback * delay_out);
        delay_line.Write(delay_in);
        t_sample filter_out = 0;
        filter.Process(&delay_out, &filter_out);

        frame_out[frame_offset_] = (filter_out * balance) + (frame_in[frame_offset_] * (1 - balance));
    }

    void DelayFx::Update() {
        // TODO: try using a read index for multi-tap support.
        delay_line.SetDelay(ol::core::scale(time, 0, 1, 0, MAX_DELAY, 1));
        filter.Update();
    }

    void DelayFx::UpdateHardwareControl(uint8_t control, uint8_t value) {
        bool update = true;
        switch (control) {
            case CC_DELAY_TIME:
                time = value;
                break;
            case CC_DELAY_FEEDBACK:
                feedback = value;
                break;
            case CC_DELAY_BALANCE:
                balance = value;
                break;
            default:
                update = false;
        }
        filter.UpdateHardwareControl(control, value);
        if (update) {
            Update();
        }

    }

    void DelayFx::UpdateMidiControl(uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);

        switch (control) {
            case CC_DELAY_TIME:
                time = scaled;
                break;
            case CC_DELAY_FEEDBACK:
                feedback = scaled;
                break;
            case CC_DELAY_BALANCE:
                balance = scaled;
                break;
            case CC_DELAY_CUTOFF:
                filter.UpdateMidiControl(CC_FILTER_CUTOFF, value);
                break;
            case CC_DELAY_RESONANCE:
                filter.UpdateMidiControl(CC_FILTER_RESONANCE, value);
                break;
            default:
                update = false;
        }
        if (update) {
            Update();
        }
    }
}
