//
// Created by Orion Letizi on 12/2/23.
//

#include "Voice.h"

namespace ol::synth {

    void Polyvoice_init(Polyvoice *m, t_sample sample_rate) {
        for (int i = 0; i < m->voice_count; i++) {
            m->voices[i]->Init(m->voices[i], sample_rate);
        }
        m->initialized = true;
    }

    t_sample Polyvoice_process(Polyvoice *m) {
        t_sample rv = 0;
        for (int i = 0; i < m->voice_count; i++) {
            rv += m->voices[i]->Process(m->voices[i]);
        }
        return rv;
    }

    void Polyvoice_noteOn(Polyvoice *m, uint8_t note, uint8_t velocity) {
        Voice *v;
        for (int i=0; i< m->voice_count; i++) {
            v = m->voices[i];
            if (! v->playing) {
                v->NoteOn(v, note, velocity);
                break;
            }
        }
    }

    void Polyvoice_noteOff(Polyvoice *m, uint8_t note, uint8_t velocity) {
        Voice *v;
        for (int i=0; i<m->voice_count; i++) {
            v = m->voices[i];
            if (v->playing == note) {
                v->NoteOff(v, note, velocity);
                break;
            }
        }
    }

    void Polyvoice_updateMidiControl(Polyvoice *m, uint8_t control, uint8_t value) {
        for (int i = 0; i < m->voice_count; i++) {
            m->voices[i]->UpdateMidiControl(m->voices[i], control, value);
        }
    }

    void Polyvoice_Config(Polyvoice *m, Voice *voices[], uint8_t voice_count) {
        m->Init = Polyvoice_init;
        m->Process = Polyvoice_process;
        m->NoteOn = Polyvoice_noteOn;
        m->NoteOff = Polyvoice_noteOff;
        m->UpdateMidiControl = Polyvoice_updateMidiControl;
        m->voice_count = voice_count;
        for (int i = 0; i < MAX_VOICES; i++) {
            if (i < voice_count) {
                m->pool[i] = voices[i];
                m->voices[i] = voices[i];
            } else {
                m->voices[i] = nullptr;
                m->pool[i] = nullptr;
            }
        }

        for (int i = 0; i < 128; i++) {
            for (int j = 0; j < MAX_VOICES; j++) {
                m->playing[i][j] = nullptr;
            }
        }
    }

}
