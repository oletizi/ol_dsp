//
// Created by Orion Letizi on 12/2/23.
//

#include "Polyvoice.h"

namespace ol::synth {

    void Polyvoice::Init(t_sample sample_rate) {
        init(this, sample_rate);
    }

    void Polyvoice::Process(t_sample *frame_out) {
        process(this, frame_out);
    }

    void Polyvoice::NoteOn(const uint8_t note, const uint8_t velocity) {
        note_on(this, note, velocity);
    }

    void Polyvoice::NoteOff(const uint8_t note, const uint8_t velocity) {
        note_off(this, note, velocity);
    }

    void Polyvoice::UpdateMidiControl(uint8_t control, uint8_t value) {
        update_midi_control(this, control, value);
    }

    void Polyvoice::voices_init(Polyvoice *p, t_sample sample_rate) {
        for (int i = 0; i < p->voice_count; i++) {
            p->voices_[i]->Init(sample_rate);
        }
        p->initialized = true;
    }

    void Polyvoice::map_init(Polyvoice *p, t_sample sample_rate) {
        p->voice_map_->Init(sample_rate);
    }

    void Polyvoice::voices_process(Polyvoice *p, t_sample *frame_out) {
        for (int i = 0; i < p->voice_count; i++) {
            t_sample s = 0;
            p->voices_[i]->Process(&s);
            *frame_out += s;
        }
    }

    void Polyvoice::map_process(Polyvoice *p, t_sample *frame_out) {
        p->voice_map_->Process(frame_out);
    }

    void Polyvoice::voices_note_on(Polyvoice *p, uint8_t note, uint8_t velocity) {
        for (int i = 0; i < p->voice_count; i++) {
            auto v = p->voices_[i];
            if (!v->Playing()) {
                v->NoteOn(note, velocity);
                break;
            }
        }
    }

    void Polyvoice::map_note_on(Polyvoice *p, uint8_t note, uint8_t velocity) {
        p->voice_map_->NoteOn(note, velocity);
    }

    void Polyvoice::voices_note_off(Polyvoice *p, uint8_t note, uint8_t velocity) {
        for (int i = 0; i < p->voice_count; i++) {
            auto v = p->voices_[i];
            if (v->Playing() == note) {
                v->NoteOff(note, velocity);
                break;
            }
        }
    }

    void Polyvoice::map_note_off(Polyvoice *p, uint8_t note, uint8_t velocity) {
        p->voice_map_->NoteOff(note, velocity);
    }

    void Polyvoice::voices_controller_function(Polyvoice *p, uint8_t control, uint8_t value) {
        for (int i = 0; i < p->voice_count; i++) {
            p->voices_[i]->UpdateMidiControl(control, value);
        }
    }

    void Polyvoice::map_controller_function(Polyvoice *p, uint8_t control, uint8_t value) {
        p->voice_map_->UpdateMidiControl(control, value);
    }
}
