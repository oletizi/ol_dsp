//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_SYNTH_VOICE
#define OL_SYNTH_VOICE

#include <queue>
#include <daisysp.h>
#include "synthlib/ol_synthlib.h"

#define MAX_VOICES 8

namespace ol::synth {

    class PitchedSoundSource {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual t_sample Process() = 0;

        virtual void SetFreq(t_sample freq) = 0;
    };

    class OscillatorSoundSource : public PitchedSoundSource {
    public:
        explicit OscillatorSoundSource(daisysp::Oscillator *o) : o_(o) {};

        void Init(t_sample sample_rate) override;

        t_sample Process() override;

        void SetFreq(t_sample freq) override;

    private:
        daisysp::Oscillator *o_;
        const float freq_ = 0;
    };

    struct Voice {

        typedef
        void (*Init_function)(Voice *, t_sample sample_rate);

        typedef
        void (*Update_function)(Voice *);

        typedef
        t_sample (*Process_function)(Voice *);


        Init_function Init = nullptr;
        Update_function Update = nullptr;
        Process_function Process = nullptr;

        void (*UpdateMidiControl)(Voice *, uint8_t control, uint8_t value) = nullptr;

        // XXX: This should be separated into note and gate
        void (*NoteOn)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;

        void (*NoteOff)(Voice *, uint8_t midi_note, uint8_t velocity) = nullptr;

        bool initialized = false;

        t_sample portamento_htime = 0;

        t_sample master_volume = 0.8f;

        t_sample sample_rate = 0;

        // Oscillator, sample player, etc.
        PitchedSoundSource *sound_source = nullptr;

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

        // Oscillator mixer
        t_sample osc_1_mix = 0.8f;
        uint8_t playing = 0;
    };

    void Voice_Config(Voice *,
                      PitchedSoundSource *sound_source,
                      daisysp::Svf *filter,
                      daisysp::Adsr *filter_envelope,
                      daisysp::Adsr *amp_envelope,
                      daisysp::Port *portamento);

    inline void Voice_Config(Voice *v) {
        Voice_Config(v, new OscillatorSoundSource(new daisysp::Oscillator()),
                     new daisysp::Svf(), new daisysp::Adsr(),
                     new daisysp::Adsr(), new daisysp::Port());
    }

    struct Polyvoice {

        void (*Init)(Polyvoice *, t_sample sample_rate) = nullptr;

        t_sample (*Process)(Polyvoice *) = nullptr;

        void (*NoteOn)(Polyvoice *, uint8_t note, uint8_t velocity) = nullptr;

        void (*NoteOff)(Polyvoice *, uint8_t note, uint8_t velocity) = nullptr;

        void (*UpdateMidiControl)(Polyvoice *, uint8_t control, uint8_t value) = nullptr;

        Voice *voices[MAX_VOICES] = {};
        bool initialized = false;
        uint8_t voice_count = 0;
    };

    void Polyvoice_Config(Polyvoice *m, Voice *voices[], uint8_t voice_count);

}
#endif //OL_SYNTH_VOICE
