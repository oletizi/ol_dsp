//
// Created by Orion Letizi on 12/8/23.
//

#ifndef OL_DSP_VOICEMAP_H
#define OL_DSP_VOICEMAP_H

#include "corelib/ol_corelib.h"
#include "Voice.h"

#define MAP_SIZE 128
namespace ol::synth {

    template<int CHANNEL_COUNT>
    class VoiceMap {
    private:
        Voice *note2voice[MAP_SIZE] = {};
        t_sample frame_buffer[CHANNEL_COUNT] = {};

    public:
        void NoteOn(uint8_t note, uint8_t velocity) {
            if (note < 128) {
                Voice *voice = note2voice[note];
                if (voice != nullptr) {
                    voice->NoteOn(note, velocity);
                }
            }
        }

        void NoteOff(uint8_t note, uint8_t velocity) {
            if (note < 128) {
                Voice *voice = note2voice[note];
                if (voice != nullptr) {
                    voice->NoteOff(note, velocity);
                }
            }
        }

        void SetVoice(uint8_t note, ol::synth::Voice *voice) {
            if (note < 128) {
                note2voice[note] = voice;
            }
        }

        void Init(t_sample sample_rate) {
            for (auto &v: note2voice) { if (v != nullptr) v->Init(sample_rate); }
        }

        void Process(t_sample *frame_out) {
            for (auto &v: note2voice) {
                if (v != nullptr) {
                    v->Process(frame_buffer);
                    for (int i=0; i<CHANNEL_COUNT; i++) {
                        frame_out[i] += frame_buffer[i];
                    }
                }
            }
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            for (auto &v: note2voice) { if (v != nullptr) v->UpdateMidiControl(control, value); }
        }

    };
}

#endif //OL_DSP_VOICEMAP_H
