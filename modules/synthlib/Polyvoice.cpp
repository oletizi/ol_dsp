//
// Created by Orion Letizi on 12/2/23.
//

#include "Polyvoice.h"

namespace ol::synth {

    void Polyvoice::Init(t_sample sample_rate) {
        for (int i = 0; i < voice_count; i++) {
            voices_[i]->Init(sample_rate);
        }
        initialized = true;
    }

    void Polyvoice::Process(t_sample *frame_out) {
        for (int i = 0; i < voice_count; i++) {
            t_sample s = 0;
            voices_[i]->Process(&s);
            *frame_out += s;
        }
    }

    void Polyvoice::NoteOn(const uint8_t note, const uint8_t velocity) {
        for (int i = 0; i < voice_count; i++) {
            auto v = voices_[i];
            if (!v->Playing()) {
                v->NoteOn(note, velocity);
                break;
            }
        }
    }

    void Polyvoice::NoteOff(const uint8_t note, const uint8_t velocity) {
        for (int i = 0; i < voice_count; i++) {
            auto v = voices_[i];
            if (v->Playing() == note) {
                v->NoteOff(note, velocity);
                break;
            }
        }
    }

    void Polyvoice::UpdateMidiControl(uint8_t control, uint8_t value) {
        for (int i = 0; i < voice_count; i++) {
            voices_[i]->UpdateMidiControl(control, value);
        }
    }

}
