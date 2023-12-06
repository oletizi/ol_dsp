//
// Created by Orion Letizi on 12/2/23.
//

#include "Polyvoice.h"

namespace ol::synth {

    void Polyvoice::Init(t_sample sample_rate) {
        for (int i=0; i< voice_count; i++) {
            voices_[i]->Init(sample_rate);
        }
        initialized = true;
    }

    t_sample Polyvoice::Process() {
        t_sample rv = 0;
        for (int i=0; i<voice_count; i++) {
            rv += voices_[i]->Process();
        }
        return rv;
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
