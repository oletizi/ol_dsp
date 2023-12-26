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

    t_sample cv_volts_per_octave_to_frequency(t_sample cv_volts_per_octave) {
        auto voct = daisysp::fmap(cv_volts_per_octave, 0.f, 40.f);
        t_sample coarse_tune = 30.93; // XXX: parameterize this
        return coarse_tune * voct;
    }

    uint8_t cv_pitch_to_midi(t_sample cv_pitch) {
        auto frequency = cv_volts_per_octave_to_frequency(cv_pitch);
        auto midi_nn = uint8_t(daisysp::fclamp(frequency, 0.f, 127.f));
        return uint8_t(midi_nn);
    }

    struct VoiceInput {
    public:
        daisy::Switch gate_cv;
        InputHandle pitch_cv;
        t_sample previous_pitch_cv;
    };

    class VoiceInputListener {
    public:

        virtual void PitchCv(int channel, t_sample volt_per_octave) = 0;

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
                vi.pitch_cv = pool.AddInput();
                vi.gate_cv = pool.AddSwitch();
            }
        }

        void Process() {
            for (auto &vi: voice_inputs_) {
                // XXX: the interface of InputHandle is pretty different than the interface of Switch. They should
                // probably be more similar.
                auto &gate_cv = vi.gate_cv;
                // XXX: the channel should be on the InputHandle
                auto channel = vi.pitch_cv.channel_index;
                gate_cv.Debounce();
                listener_.PitchCv(channel,pool_.GetFloat(vi.pitch_cv));
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
