//
// Created by Orion Letizi on 11/8/23.
//

#ifndef OL_SYNTH_VOICE
#define OL_SYNTH_VOICE

#include "corelib/ol_corelib.h"

#define MAX_VOICES 8

namespace ol::synth {
    class Voice {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual void Update() = 0;

        virtual t_sample Process() = 0;

        virtual void UpdateMidiControl(uint8_t control, uint8_t value) = 0;

        virtual void NoteOn(uint8_t midi_note, uint8_t velocity) = 0;

        virtual void NoteOff(uint8_t midi_note, uint8_t velocity) = 0;

        virtual uint8_t Playing() = 0;
    };

}
#endif //OL_SYNTH_VOICE