//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_DSP_CONTROL_H
#define OL_DSP_CONTROL_H

#include <cstdint>
#include "ol_synthlib_core.h"

namespace ol::synthlib {
    /**
 * Abstraction for control over a parameter. Probably rename.
 */
    class Control {
    public:
        explicit Control(Scale hardware_scale = Scale(0, 1, 0, 1, 1), Scale midi_scale = Scale(0, 127, 0, 1, 1),
                t_sample initial_value = 0)
                : hardware_scale_(hardware_scale), midi_scale_(midi_scale), value_(initial_value) {}

        [[nodiscard]] t_sample Value() const {
            return value_ + (cv_value_ * cv_amount_);
        }

        void UpdateValueMidi(uint8_t new_midi_value) {
            t_sample scaled_value = midi_scale_.Process(new_midi_value);
            // Since MIDI updates are event based, a new MIDI value is an intent to
            // update the value, so always update to incoming MIDI values.
            UpdateValue(scaled_value);
        }

        void UpdateValueHardware(t_sample new_hardware_value) {
            t_sample scaled_value = hardware_scale_.Process(new_hardware_value);
            // Since hardware values are continuous, only update the control value if hardware input has
            // changed.
            if (hardware_value_ <= (scaled_value - 0.25) || hardware_value_ >= (scaled_value + 0.25)) {
                // Hardware value has changed (within a margin above the noise). Update value to new hardware value.
                UpdateValue(scaled_value);
            }
            // Record hardware value
            hardware_value_ = scaled_value;
        }

        void UpdateCv(t_sample new_cv_value) {
            cv_value_ = new_cv_value;
        }

        void UpdateCvAmountMidi(uint8_t new_cv_amount) {
            cv_amount_ = midi_scale_.Process(new_cv_amount);
        }

        t_sample GetCv() const { return cv_value_; }

        t_sample GetCvAmount() const { return cv_amount_; }


    private:
        void UpdateValue(t_sample new_value) {
            value_ = new_value;
        }

        Scale hardware_scale_;
        Scale midi_scale_;
        t_sample hardware_value_ = 0;
        t_sample cv_value_ = 0;
        t_sample cv_amount_ = 0;
        t_sample value_;
    };

}

#endif //OL_DSP_CONTROL_H
