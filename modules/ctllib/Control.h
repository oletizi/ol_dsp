//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_CONTROL_H
#define OL_DSP_CONTROL_H

#include <cstdint>
#include "corelib/ol_corelib.h"

#define OL_ADC_MAX 4096
#define OL_MIDI_MAX 127
#define OL_FLOAT_MAX 1

namespace ol::ctl {
    class Control {

    public:
        typedef uint16_t CONTROLLER_TYPE;
        typedef uint16_t ADC_TYPE;
        typedef uint8_t MIDI_TYPE;
        typedef t_sample FLOAT_TYPE;

        Control() {}

        Control(CONTROLLER_TYPE controller, MIDI_TYPE midi_value)
                : controller_(controller),
                  midi_value_(midi_value),
                  float_value_(MIDI_TO_FLOAT(midi_value)),
                  adc_value_(MIDI_TO_ADC(midi_value)) {}

        Control(CONTROLLER_TYPE controller, FLOAT_TYPE float_value)
                : controller_(controller),
                  midi_value_(FLOAT_TO_MIDI(float_value)),
                  float_value_(float_value),
                  adc_value_(FLOAT_TO_ADC(float_value)) {}

        Control(CONTROLLER_TYPE controller, ADC_TYPE adc_value)
                : controller_(controller),
                  midi_value_(ADC_TO_MIDI(adc_value)),
                  float_value_(ADC_TO_FLOAT(adc_value)),
                  adc_value_(adc_value) {}


        void SetController(CONTROLLER_TYPE c) {
            controller_ = c;
        }

        [[nodiscard]] CONTROLLER_TYPE GetController() const {
            return controller_;
        }

        [[nodiscard]] MIDI_TYPE GetMidiValue() const {
            return midi_value_;
        }

        void SetMidiValue(MIDI_TYPE v) {
            midi_value_ = v;
            adc_value_ = MIDI_TO_ADC(v);
            float_value_ = MIDI_TO_FLOAT(v);
        }

        [[nodiscard]] FLOAT_TYPE GetFloatValue() const {
            return float_value_;
        }

        void SetFloatValue(FLOAT_TYPE v) {
            float_value_ = v;
            adc_value_ = FLOAT_TO_ADC(v);
            midi_value_ = FLOAT_TO_MIDI(v);
        }

        [[nodiscard]] ADC_TYPE GetAdcValue() const {
            return adc_value_;
        }

        void SetAdcValue(ADC_TYPE v) {
            adc_value_ = v;
            midi_value_ = ADC_TO_MIDI(v);
            float_value_ = ADC_TO_FLOAT(v);
        }

    private:
        CONTROLLER_TYPE controller_ = 0;
        ADC_TYPE adc_value_ = 0;
        FLOAT_TYPE float_value_ = 0;
        MIDI_TYPE midi_value_ = 0;

        static MIDI_TYPE ADC_TO_MIDI(ADC_TYPE v) {
            return MIDI_TYPE(ol::core::scale(v, 0, OL_ADC_MAX, 0, OL_MIDI_MAX, 1));
        }

        static FLOAT_TYPE ADC_TO_FLOAT(ADC_TYPE v) {
            return ol::core::scale(v, 0, OL_ADC_MAX, 0, OL_FLOAT_MAX, 1);
        }

        static ADC_TYPE MIDI_TO_ADC(MIDI_TYPE v) {
            return ADC_TYPE(ol::core::scale(v, 0, OL_MIDI_MAX, 0, OL_ADC_MAX, 1));
        }

        static FLOAT_TYPE MIDI_TO_FLOAT(MIDI_TYPE v) {
            return ol::core::scale(v, 0, OL_MIDI_MAX, 0, OL_FLOAT_MAX, 1);
        }

        static ADC_TYPE FLOAT_TO_ADC(FLOAT_TYPE v) {
            return ADC_TYPE(ol::core::scale(v, 0, OL_FLOAT_MAX, 0, OL_ADC_MAX, 1));
        }

        static MIDI_TYPE FLOAT_TO_MIDI(FLOAT_TYPE v) {
            return MIDI_TYPE(ol::core::scale(v, 0, OL_FLOAT_MAX, 0, OL_MIDI_MAX, 1));
        }
    };
}

#endif //OL_DSP_CONTROL_H
