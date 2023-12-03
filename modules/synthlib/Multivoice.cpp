//
// Created by Orion Letizi on 12/2/23.
//

#include "Voice.h"

namespace ol::synth {

    void Multivoice_init(Multivoice *m, t_sample sample_rate) {
        for (int i = 0; i < m->voice_count; i++) {
            m->voices[i]->Init(m->voices[i], sample_rate);
        }
        m->initialized = true;
    }

    t_sample Multivoice_process(Multivoice *m) {
        t_sample rv = 0;
        for (int i = 0; i < m->voice_count; i++) {
            rv += m->voices[i]->Process(m->voices[i]);
        }
        return rv;
    }

    void Multivoice_noteOn(Multivoice *m, uint8_t note, uint8_t velocity) {
        Voice *v;
        for (int i=0; i< m->voice_count; i++) {
            v = m->voices[i];
            if (! v->playing) {
                v->NoteOn(v, note, velocity);
                break;
            }
        }
    }

    void Multivoice_noteOff(Multivoice *m, uint8_t note, uint8_t velocity) {
        Voice *v;
        for (int i=0; i<m->voice_count; i++) {
            v = m->voices[i];
            if (v->playing == note) {
                v->NoteOff(v, note, velocity);
                break;
            }
        }
    }

    void Multivoice_updateMidiControl(Multivoice *m, uint8_t control, uint8_t value) {
        for (int i = 0; i < m->voice_count; i++) {
            m->voices[i]->UpdateMidiControl(m->voices[i], control, value);
        }
    }

    void Multivoice_Config(Multivoice *m, Voice *voices[], uint8_t voice_count) {
        m->Init = Multivoice_init;
        m->Process = Multivoice_process;
        m->NoteOn = Multivoice_noteOn;
        m->NoteOff = Multivoice_noteOff;
        m->UpdateMidiControl = Multivoice_updateMidiControl;
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
