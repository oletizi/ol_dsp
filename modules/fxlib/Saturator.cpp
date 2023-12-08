//
// Created by Orion Letizi on 11/29/23.
//
#include "fxlib/Fx.h"

namespace ol::fx {
    void SaturatorFx::Init(const t_sample sample_rate) { /* nop */ }

    void SaturatorFx::Process(const t_sample *frame_in, t_sample *frame_out) {
        frame_out[frame_offset] = transferFunction(this, frame_in[frame_offset]);
    }

    void SaturatorFx::Update() { /* nop */ }

    void SaturatorFx::UpdateMidiControl(uint8_t control, uint8_t value) {
        if (control == CC_SATURATOR_DRIVE) {
            drive = ol::core::scale(value, 0, 127, 1, 128, 1.5);
            Update();
        }
    }
}