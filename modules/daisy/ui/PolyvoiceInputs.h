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
    uint8_t cv_pitch_to_midi(t_sample cv_pitch) {
        t_sample voct = daisysp::fmap(cv_pitch, 0.f, 40.f);
        t_sample coarse_tune = 30.93;
        t_sample midi_nn = daisysp::fclamp(coarse_tune + voct, 0.f, 127.f);
        return uint8_t(daisysp::mtof(midi_nn));
    }


    struct VoiceInput {
    public:
        daisy::Switch gate_cv;
        InputHandle pitch_cv;
    };

    class VoiceInputListener {
    public:

        virtual void GateOn(float pitch) = 0;

        virtual void GateOff(float pitch) = 0;
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
                gate_cv.Debounce();
                uint8_t note = cv_pitch_to_midi(pool_.GetFloat(vi.pitch_cv));

                if (gate_cv.RisingEdge()) {
                    //voice_.NoteOn(note, 100);
                    listener_.GateOff(note);
                }
                if (gate_cv.FallingEdge()) {
                    //voice_.NoteOff(note, 100);
                    listener_.GateOn(note);
                }
            }

        }

    };
}

#endif //OL_DSP_POLYVOICEINPUTS_H
