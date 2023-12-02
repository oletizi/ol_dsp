//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_VOICE_H
#define JUCE_TEST_VOICE_H

#include <daisysp.h>
#include "ol_synthlib_core.h"

#define OSCILLATOR_COUNT 4

namespace ol::synthlib {

    struct Voice {

        void (*Init)(Voice *, t_sample sample_rate) = nullptr;
        void (*Update)(Voice *) = nullptr;
        t_sample (*Process)(Voice *) = nullptr;
        void (*UpdateMidiControl)(Voice *, int control, int value) = nullptr;

        // XXX: This should be separated into note and gate
        void (*NoteOn)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;

        void (*NoteOff)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;


        t_sample portamento_htime = 0;

        t_sample filt_freq = 20000;
        t_sample filt_res = 0;
        t_sample filt_drive = 0;
        t_sample master_volume = 0.8f;

        t_sample sample_rate = 0;

        uint8_t notes_on = 0;

        daisysp::Oscillator slop_lfo_1;
        daisysp::Oscillator slop_lfo_2;
        daisysp::Oscillator slop_lfo_3;
        daisysp::Oscillator slop_lfo_4;

        // Oscillator
        daisysp::Oscillator oscillators[4] = {daisysp::Oscillator(), daisysp::Oscillator(), daisysp::Oscillator(),
                                              daisysp::Oscillator()};
        t_sample freq_ = 0;

        // Filter
        daisysp::Adsr filter_envelope;
        daisysp::Svf filt_1;

        // Amplifier
        daisysp::Adsr amp_envelope;

        // Portamento
        daisysp::Port portamento_;

        // Filter envelope
        t_sample filter_attack = 0;
        t_sample filter_decay = 0;
        t_sample filter_sustain = 1;
        t_sample filter_release = 0;
        t_sample filter_envelope_amount = 1;

        // Amp envelope
        t_sample amp_attack = 0;
        t_sample amp_decay = 0;
        t_sample amp_sustain = 1;
        t_sample amp_release = 0;

        // Oscillator slop
        t_sample slop_factor = 0.5;
        t_sample osc_1_slop = 0.25f;
        t_sample osc_2_slop = 0.25f;
        t_sample osc_3_slop = 0.1f;
        t_sample osc_4_slop = 0.07f;

        // Oscillator mixer
        t_sample osc_1_mix = 0.25;
        t_sample osc_2_mix = 0.25;
        t_sample osc_3_mix = 0.25;
        t_sample osc_4_mix = 0.25;
    };

    void Voice_Config(Voice *);

}
#endif //JUCE_TEST_VOICE_H
