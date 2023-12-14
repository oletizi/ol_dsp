//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_SYNTH_VOICE
#define OL_SYNTH_VOICE

#include "corelib/ol_corelib.h"
#include "synthlib/SoundSource.h"

namespace ol::synth {
class Voice {
    public:
        struct Config {
            t_sample filter_cutoff;
            t_sample filter_resonance;
            t_sample filter_drive;
            t_sample filter_env_amount;
            t_sample filter_attack;
            t_sample filter_attack_shape;
            t_sample filter_decay;
            t_sample filter_sustain;
            t_sample filter_release;
            t_sample amp_env_amount;
            t_sample amp_attack;
            t_sample amp_attack_shape;
            t_sample amp_decay;
            t_sample amp_sustain;
            t_sample amp_release;
            t_sample portamento;
        };

        virtual void Init(t_sample sample_rate) = 0;

        virtual void Update() = 0;

        virtual void Process(t_sample *frame_out) = 0;

        virtual void UpdateMidiControl(uint8_t control, uint8_t value) = 0;

        virtual void UpdateConfig(Config &config) = 0;

        virtual void GateOn() = 0;

        virtual void GateOff() = 0;

        virtual bool Gate() = 0;

        virtual void NoteOn(uint8_t midi_note, uint8_t velocity) = 0;

        virtual void NoteOff(uint8_t midi_note, uint8_t velocity) = 0;

        virtual uint8_t Playing() = 0;
    };

}
#endif //OL_SYNTH_VOICE