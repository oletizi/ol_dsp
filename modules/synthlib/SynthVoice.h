//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SYNTHVOICE_H
#define OL_DSP_SYNTHVOICE_H

#include "Voice.h"

namespace ol::synth {
    class SynthVoice : public Voice {
    public:
        explicit SynthVoice(SoundSource &sound_source,
                            daisysp::Svf &filter,
                            daisysp::Adsr &filter_envelope,
                            daisysp::Adsr &amp_envelope,
                            daisysp::Port &portamento,
                            int frame_offset = 0)
                : sound_source(sound_source),
                  filter(filter),
                  filter_envelope(filter_envelope),
                  amp_envelope(amp_envelope),
                  portamento(portamento),
                  frame_offset(frame_offset) {};

        void Init(t_sample sample_rate) override;

        void Update() override;

        void Process(t_sample *frame_out) override;

        void UpdateMidiControl(uint8_t control, uint8_t value) override;

        void GateOn() override;

        void GateOff() override;

        bool Gate() override;

        void NoteOn(uint8_t midi_note, uint8_t velocity) override;

        void NoteOff(uint8_t midi_note, uint8_t velocity) override;

        uint8_t Playing() override;

    private:
        bool initialized = false;

        t_sample portamento_htime = 0;

        t_sample master_volume = 0.8f;

        t_sample sample_rate = 0;

        // Oscillator, sample player, etc.
        SoundSource &sound_source;

        t_sample freq_ = 0;

        // Filter
        daisysp::Adsr &filter_envelope;
        daisysp::Svf &filter;

        // Amplifier
        daisysp::Adsr &amp_envelope;

        // Portamento
        daisysp::Port &portamento;

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

        // Oscillator mixer
        t_sample osc_1_mix = 0.8f;
        uint8_t playing = 0;
        int frame_offset = 0;
        bool gate = false;
    };
}
#endif //OL_DSP_SYNTHVOICE_H