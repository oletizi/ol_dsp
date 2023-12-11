//
// Created by Orion Letizi on 12/8/23.
//

#ifndef OL_DSP_VOICEMAP_H
#define OL_DSP_VOICEMAP_H
#include "corelib/ol_corelib.h"
#include "Voice.h"
#define MAP_SIZE 128
namespace ol::synth {

    class VoiceMap {
    public:
        VoiceMap() {
            for (auto & i : note2voice) {
                i = nullptr;
            }
        }
        void NoteOn(uint8_t note, uint8_t velocity);
        void NoteOff(uint8_t note, uint8_t velocity);
        void SetVoice(uint8_t note, Voice *voice);

        void Init(t_sample sample_rate);

        void Process(t_sample *frame_out);

        void UpdateMidiControl(uint8_t control, uint8_t value);

    private:
        Voice *note2voice[MAP_SIZE];
    };
}

#endif //OL_DSP_VOICEMAP_H
