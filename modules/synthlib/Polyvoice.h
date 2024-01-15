//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_POLYVOICE_H
#define OL_DSP_POLYVOICE_H

#include "Voice.h"
#include "VoiceMap.h"

namespace ol::synth {
    template<int CHANNEL_COUNT>
    class Polyvoice : public Voice {
    private:
        std::vector<Voice *> &voices_;
        bool initialized = false;
//        t_sample frame_buffer[CHANNEL_COUNT]{};

    public:
        explicit Polyvoice(std::vector<Voice *> &voices) : voices_(voices) {}

        void Init(t_sample sample_rate) override {
            for (auto &voice: voices_) {
                voice->Init(sample_rate);
            }
            initialized = true;
        }

        void Process(t_sample *frame_out) override {
            t_sample frame_buffer[CHANNEL_COUNT]{};


            for (auto voice: voices_) {
                voice->Process(frame_buffer);
                for (int i = 0; i < CHANNEL_COUNT; i++) {
                    frame_out[i] += frame_buffer[i];
                }
            }
        }

        void NoteOn(uint8_t note, uint8_t velocity) override {
            for (auto voice: voices_) {
                if (!voice->Playing()) {
                    voice->NoteOn(note, velocity);
                    break;
                }
            }
        }

        void NoteOff(uint8_t note, uint8_t velocity) override {
            for (auto voice: voices_) {
                if (voice->Playing() == note) {
                    voice->NoteOff(note, velocity);
                    break;
                }
            }
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) override {
            for (auto voice: voices_) { voice->UpdateMidiControl(control, value); }
        }

        void UpdateHardwareControl(uint8_t control, t_sample value) override {
            for (auto voice: voices_) { voice->UpdateHardwareControl(control, value); }
        }

        void Update() override {
            for (auto voice: voices_) { voice->Update(); }
        }

        void UpdateConfig(Config &config) override {
            for (auto voice: voices_) { voice->UpdateConfig(config); }
        }

        void GateOn() override {
            for (auto voice: voices_) { voice->GateOn(); }
        }

        void GateOff() override {
            for (auto voice: voices_) { voice->GateOff(); }
        }

        void SetFrequency(t_sample sample) override {/* nop */}

        uint8_t Playing() override {
            return 0;
        }

        bool Gate() override {
            return false;
        }
    };
}
#endif //OL_DSP_POLYVOICE_H
