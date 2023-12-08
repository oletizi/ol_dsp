//
// Created by Orion Letizi on 11/10/23.
//

#include "Fx.h"

namespace ol::fx {

    void FxRack::Init(t_sample sample_rate) {
        delay1.Init(sample_rate);
        delay2.Init(sample_rate);
        reverb.Init(sample_rate);
        filter1.Init(sample_rate);
        filter2.Init(sample_rate);
        saturator1.Init(sample_rate);
        saturator2.Init(sample_rate);
        interstage_saturator.Init(sample_rate);
    };

    void FxRack::Process(const t_sample *frame_in, t_sample *frame_out) {

        delay1.Process(frame_in, frame_out);
        delay2.Process(frame_in, frame_out);
        reverb.Process(frame_in, frame_out);

        saturator1.Process(frame_in, frame_out);
        saturator2.Process(frame_in, frame_out);

        filter1.Process(frame_in, frame_out);
        filter2.Process(frame_in, frame_out);

        for (int i=0; i<channel_count; i++) {
            frame_out[i] *= master_volume;
        }
    };

    void FxRack::Update() {
        delay1.Update();
        delay2.Update();
        reverb.Update();
        filter1.Update();
        filter2.Update();
        saturator1.Update();
        saturator2.Update();
    }

    void FxRack::UpdateMidiControl(const uint8_t control, const uint8_t value) {
        delay1.UpdateMidiControl(control, value);
        delay2.UpdateMidiControl(control, value);
        reverb.UpdateMidiControl(control, value);
        filter1.UpdateMidiControl(control, value);
        filter2.UpdateMidiControl(control, value);
        saturator1.UpdateMidiControl(control, value);
        saturator2.UpdateMidiControl(control, value);
        if (control == CC_CTL_VOLUME) {
            master_volume = ol::core::scale(value, 0, 127, 0, 1, 1);
        }
    }

}