//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_POLYVOICE_H
#define OL_DSP_POLYVOICE_H

#include "Voice.h"

namespace ol::synth {
    class Polyvoice {
    public:
        Polyvoice(Voice **voices, uint8_t voice_count) : voices_(voices), voice_count(voice_count) {}

        void Init(t_sample sample_rate);

        void Process(t_sample *frame_out);

        void NoteOn(uint8_t note, uint8_t velocity);

        void NoteOff(uint8_t note, uint8_t velocity);

        void UpdateMidiControl(uint8_t control, uint8_t value);

    private:
        Voice **voices_;
        bool initialized = false;
        uint8_t voice_count = 0;
    };

    void Polyvoice_Config(Polyvoice *m, Voice *voices[], uint8_t
    voice_count);
}
#endif //OL_DSP_POLYVOICE_H
