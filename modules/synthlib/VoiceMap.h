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
        struct voice_data {
            Voice *voice = nullptr;
            uint8_t channel = 0;
            uint8_t note = 0;
        };
        voice_data note2voice[MAP_SIZE] = {};
        voice_data channel2voice[16] = {};
        t_sample frame_buffer[CHANNEL_COUNT] = {};

    public:
        void NoteOn(uint8_t note, uint8_t velocity) {
            if (note < 128) {
                Voice *voice = note2voice[note].voice;
                if (voice != nullptr) {
                    voice->NoteOn(note, velocity);
                }
            }
        }

        void NoteOff(uint8_t note, uint8_t velocity) {
            if (note < 128) {
                Voice *voice = note2voice[note].voice;
                if (voice != nullptr) {
                    voice->NoteOff(note, velocity);
                }
            }
        }

        void SetVoice(uint8_t channel, uint8_t note, ol::synth::Voice *voice) {
            // XXX: This is super janky
            int channel_index = channel - 1;
            if (note < 128 && channel > 0 && channel_index < 16) {
                voice_data &d = note2voice[note];
                d.channel = channel;
                d.note = note;
                d.voice = voice;

                d = channel2voice[channel_index];
                d.channel = channel;
                d.note = note;
                d.voice = voice;
            }
        }

        void Init(t_sample sample_rate) {
            for (auto &v: note2voice) { if (v.voice != nullptr) v.voice->Init(sample_rate); }
        }

        void Process(t_sample *frame_out) {
            for (auto &v: note2voice) {
                if (v.voice != nullptr) {
                    v.voice->Process(frame_buffer);
                    for (int i = 0; i < CHANNEL_COUNT; i++) {
                        frame_out[i] += frame_buffer[i];
                    }
                }
            }
        }

        void UpdateMidiControl(uint8_t channel, uint8_t control, uint8_t value) {
            if (channel > 0 && channel <= 16) {
                Voice *voice = channel2voice[channel - 1].voice;
                if (voice != nullptr) {
                    voice->UpdateMidiControl(control, value);
                }
            }
        }

    };
}

#endif //OL_DSP_VOICEMAP_H
