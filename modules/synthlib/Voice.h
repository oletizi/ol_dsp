//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_SYNTH_VOICE
#define OL_SYNTH_VOICE

#include <queue>
#include <daisysp.h>
#include "synthlib/ol_synthlib.h"

#define MAX_VOICES 5

#define OSCILLATOR_COUNT 4

namespace ol::synth {

    struct Voice {

        void (*Init)(Voice *, t_sample sample_rate) = nullptr;

        void (*Update)(Voice *) = nullptr;

        t_sample (*Process)(Voice *) = nullptr;

        void (*UpdateMidiControl)(Voice *, uint8_t control, uint8_t value) = nullptr;

        // XXX: This should be separated into note and gate
        void (*NoteOn)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;

        void (*NoteOff)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;

        bool initialized = false;

        t_sample portamento_htime = 0;

        t_sample master_volume = 0.8f;

        t_sample sample_rate = 0;

        daisysp::Oscillator slop_lfo_1;
        daisysp::Oscillator slop_lfo_2;
        daisysp::Oscillator slop_lfo_3;
        daisysp::Oscillator slop_lfo_4;

        // Oscillator
        daisysp::Oscillator oscillators[4] = {daisysp::Oscillator(), daisysp::Oscillator(), daisysp::Oscillator(),
                                              daisysp::Oscillator()};
        t_sample freq_ = 0;

        // Filter
        daisysp::Adsr *filter_envelope;
        daisysp::Svf *filter;

        // Amplifier
        daisysp::Adsr *amp_envelope;

        // Portamento
        daisysp::Port *portamento_;

        // Filter
        t_sample filt_freq = 0;
        t_sample filt_res = 0;
        t_sample filt_drive = 0;

        t_sample filter_attack = 0;
        t_sample filter_decay = 0.2f;
        t_sample filter_sustain = 0;
        t_sample filter_release = 0;
        t_sample filter_envelope_amount = 1;

        // Amp envelope
        t_sample amp_attack = 0.01f; // a little lag on attack and release help reduce clicking
        t_sample amp_decay = 0;
        t_sample amp_sustain = 1;
        t_sample amp_release = 0.01f;

        // Oscillator slop
        t_sample slop_factor = 0.1;
        t_sample osc_1_slop = 0.25f;
        t_sample osc_2_slop = 0.25f;
        t_sample osc_3_slop = 0.1f;
        t_sample osc_4_slop = 0.07f;

        // Oscillator mixer
        t_sample osc_1_mix = 0.8f;
        t_sample osc_2_mix = 0;
        t_sample osc_3_mix = 0;
        t_sample osc_4_mix = 0;
        uint8_t playing = 0;
    };

    void Voice_Config(Voice * ,
                      daisysp::Svf * filter,
                      daisysp::Adsr * filter_envelope,
                      daisysp::Adsr * amp_envelope,
                      daisysp::Port * portamento);

    inline void Voice_Config(Voice * v) {
        Voice_Config(v, new daisysp::Svf(), new daisysp::Adsr(), new daisysp::Adsr(), new daisysp::Port());
    }

    struct Polyvoice {

        void (*Init)(Polyvoice *, t_sample sample_rate) = nullptr;

        t_sample (*Process)(Polyvoice *) = nullptr;

        void (*NoteOn)(Polyvoice *, uint8_t note, uint8_t velocity) = nullptr;

        void (*NoteOff)(Polyvoice *, uint8_t note, uint8_t velocity) = nullptr;

        void (*UpdateMidiControl)(Polyvoice *, uint8_t control, uint8_t value) = nullptr;

        Voice *voices[MAX_VOICES] = {};
        Voice *pool[MAX_VOICES] = {};
        Voice *playing[128][MAX_VOICES] = {};

        bool initialized = false;
        uint8_t unison_count = 1;
        uint8_t voice_count = 0;
    };

    void Polyvoice_Config(Polyvoice * m, Voice * voices[], uint8_t voice_count);

}
#endif //OL_SYNTH_VOICE
