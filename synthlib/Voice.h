//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_VOICE_H
#define JUCE_TEST_VOICE_H

#include <daisysp.h>
#include "ol_synthlib_core.h"
#include "ControlPanel.h"

#define OSCILLATOR_COUNT 4

namespace ol::synthlib {

    class Voice {

    public:
        explicit Voice(ControlPanel *control_panel) : control_panel_(control_panel) {
        }

        void Init(t_sample sample_rate) {
            sample_rate_ = sample_rate;
            slop_lfo_1.Init(sample_rate_);
            slop_lfo_2.Init(sample_rate_);
            slop_lfo_1.SetFreq(0.0001f);
            slop_lfo_2.SetFreq(0.00009f);
            for (auto &oscillator: oscillators) {
                oscillator.Init(sample_rate);
            }

            filt_1_.Init(sample_rate);
            env_filt_.Init(sample_rate);
            env_amp_.Init(sample_rate);
            portamento_.Init(sample_rate, control_panel_->portamento.Value());
        }

        // XXX: This should be separated into note and gate
        void NoteOn(uint8_t midi_note, uint8_t velocity) {
            notes_on_++;
            freq_ = daisysp::mtof(midi_note);
            env_amp_.Retrigger(false);
            env_filt_.Retrigger(false);
        }

        void NoteOff(uint8_t midi_note) {
            if (notes_on_ > 0) {
                notes_on_--;
            }
        }

        t_sample Process();

    private:
        t_sample sample_rate_ = 0;

        uint8_t notes_on_ = 0;

        daisysp::Oscillator slop_lfo_1;
        daisysp::Oscillator slop_lfo_2;

        // Oscillator
        daisysp::Oscillator oscillators[4] = {daisysp::Oscillator(), daisysp::Oscillator(), daisysp::Oscillator(),
                                              daisysp::Oscillator()};
        t_sample freq_ = 0;

        // Filter
        daisysp::Adsr env_filt_;
        daisysp::Svf filt_1_;

        // Amplifier
        daisysp::Adsr env_amp_;

        // Portamento
        daisysp::Port portamento_;

        // Controls
        ControlPanel *control_panel_;
        int counter_ = 0;

        void updateWaveform();

        void setOscillatorFrequency(t_sample freq);

        t_sample processOscillators();
    };
}
#endif //JUCE_TEST_VOICE_H
