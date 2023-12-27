//
// Created by Orion Letizi on 12/25/23.
//

#ifndef OL_DSP_POLYVOICEINPUTS_H
#define OL_DSP_POLYVOICEINPUTS_H

#include <vector>
#include "daisy.h"
#include "GpioPool.h"
#include "synthlib/ol_synthlib.h"

namespace ol_daisy::ui {

    // XXX: This should probably live somewhere else.

    t_sample cv_to_frequency(t_sample cv_value) {

        t_sample referenceVoltage = 0;
        t_sample baseFrequency = 130.81; // C3
        t_sample voct = ol::core::scale(cv_value, 0, 1, 0, 3.283,
                                        1); // 3.283 is some weird magic number I got by hand tuning.

        t_sample frequency = baseFrequency * pow(2, voct -
                                                    referenceVoltage);//baseFrequency * pow(2, (voltsPerOctave - referenceVoltage));
        return frequency;
    }

    uint8_t cv_pitch_to_midi(t_sample cv_pitch) {
        auto frequency = cv_to_frequency(cv_pitch);
        auto midi_nn = uint8_t(daisysp::fclamp(frequency, 0.f, 127.f));
        return uint8_t(midi_nn);
    }

    struct VoiceInput {
    public:
        daisy::Switch gate_cv;
        daisy::AnalogControl pitch_cv;
        t_sample previous_pitch_cv = 0;
        t_sample noise_window = 0.01f;
    };

    class VoiceInputListener {
    public:

        virtual void PitchCv(int channel, t_sample pitch_cv) = 0;

        virtual void GateOn(int channel) = 0;

        virtual void GateOff(int channel) = 0;
    };

    template<int VOICE_COUNT>
    class PolyvoiceInputs {
    private:
        VoiceInput voice_inputs_[VOICE_COUNT]{};
        GpioPool<VOICE_COUNT> &pool_;
        VoiceInputListener &listener_;

    public:
        explicit PolyvoiceInputs(GpioPool<VOICE_COUNT> &pool, VoiceInputListener &listener) : pool_(pool),
                                                                                              listener_(listener) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                auto &vi = voice_inputs_[i];
//                vi.pitch_cv = pool.AddInput();
                pool.AddInput(&vi.pitch_cv);
                vi.gate_cv = pool.AddSwitch();
            }
        }

        void Process() {
//            for (auto &vi: voice_inputs_) {
            for (int channel = 0; channel < VOICE_COUNT; channel++) {
                auto &vi = voice_inputs_[channel];
                // XXX: the interface of InputHandle is pretty different than the interface of Switch. They should
                // probably be more similar.
                auto &gate_cv = vi.gate_cv;
                // XXX: the channel should be on the InputHandle
                //auto channel = vi.pitch_cv.channel_index;
                auto pitch_cv_value = vi.pitch_cv.Process();
                gate_cv.Debounce();
                if (pitch_cv_value < vi.previous_pitch_cv - vi.noise_window ||
                    pitch_cv_value > vi.previous_pitch_cv + vi.noise_window) {
                    listener_.PitchCv(channel, pitch_cv_value);
                    vi.previous_pitch_cv = pitch_cv_value;
                }
                if (gate_cv.RisingEdge()) {
                    listener_.GateOff(channel);
                }
                if (gate_cv.FallingEdge()) {
                    listener_.GateOn(channel);
                }
            }

        }

    };
}

#endif //OL_DSP_POLYVOICEINPUTS_H
