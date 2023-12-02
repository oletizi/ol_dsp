//
// Created by Orion Letizi on 12/2/23.
//

#include "Voice.h"

namespace ol::synth {

    void Multivoice_init(Multivoice *m, t_sample sample_rate) {
        for (auto v: m->voices) {
            v->Init(v, sample_rate);
        }
    }

    t_sample Multivoice_process(Multivoice *m) {
        t_sample rv = 0;
        for (auto v: m->voices) {
            rv += v->Process(v);
        }
        return rv;
    }

    void Multivoice_noteOn(Multivoice *m, uint8_t note, uint8_t velocity) {
        for (int i = 0; i < m->unison_count && !m->pool.empty(); i++) {
            Voice *v = m->pool.front();
            m->pool.pop();
            m->playing[note].push(v);
            v->NoteOn(v, note, velocity);
        }
    }

    void Multivoice_noteOff(Multivoice *m, uint8_t note, uint8_t velocity) {
        for (int i = 0; i < m->unison_count && !m->playing[note].empty(); i++) {
            Voice *v = m->playing[note].front();
            v->NoteOff(v, note, velocity);
            m->playing[note].pop();
            m->pool.push(v);
        }
    }

    void Multivoice_updateMidiControl(Multivoice *m, uint8_t control, uint8_t value) {
        for (auto v: m->voices) {
            v->UpdateMidiControl(v, control, value);
        }
    }

    void Multivoice_Config(Multivoice *m, const std::vector<Voice *> *voices) {
        m->Init = Multivoice_init;
        m->Process = Multivoice_process;
        m->NoteOn = Multivoice_noteOn;
        m->NoteOff = Multivoice_noteOff;
        m->UpdateMidiControl = Multivoice_updateMidiControl;
        for (auto v: *voices) {
            m->voices.push_back(v);
            m->pool.push(v);
        }
    }

}
