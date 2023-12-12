//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_POLYVOICE_H
#define OL_DSP_POLYVOICE_H

#include "Voice.h"
#include "VoiceMap.h"

namespace ol::synth {
    template<int CHANNEL_COUNT, int VOICE_COUNT>
    class Polyvoice {
    private:
        // init functions
        typedef void (*init_function)(Polyvoice *, t_sample sample_rate);

        static void voices_init(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, t_sample sample_rate) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                p->voices_[i]->Init(sample_rate);
            }
            p->initialized = true;
        }

        static void map_init(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, t_sample sample_rate) {
            p->voice_map_->Init(sample_rate);
        }

        // process functions
        typedef void (*process_function)(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *, t_sample *frame_out);

        static void voices_process(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, t_sample *frame_out) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                auto voice = p->voices_[i];
                voice->Process(p->frame_buffer);
                for (int j = 0; j < CHANNEL_COUNT; j++) {
                    frame_out[j] += p->frame_buffer[j];
                }
            }
        }

        static void map_process(Polyvoice *p, t_sample *frame_out) {
            p->voice_map_->Process(frame_out);
        }

        // note on functions
        typedef void (*note_on_function)(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *, uint8_t note,
                                         uint8_t velocity);

        static void
        voices_note_on(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, uint8_t note, uint8_t velocity) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                auto v = p->voices_[i];
                if (!v->Playing()) {
                    v->NoteOn(note, velocity);
                    break;
                }
            }
        }

        static void map_note_on(Polyvoice *p, uint8_t note, uint8_t velocity) {
            p->voice_map_->NoteOn(note, velocity);
        }

        // note off functions
        typedef void (*note_off_function)(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *, uint8_t note,
                                          uint8_t velocity);

        static void
        voices_note_off(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, uint8_t note, uint8_t velocity) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                auto v = p->voices_[i];
                if (v->Playing() == note) {
                    v->NoteOff(note, velocity);
                    break;
                }
            }
        }

        static void map_note_off(Polyvoice *p, uint8_t note, uint8_t velocity) {
            p->voice_map_->NoteOff(note, velocity);
        }


        // controller functions
        typedef void(*midi_controller_function)(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *, uint8_t channel, uint8_t control,
                                                uint8_t value);

        static void
        voices_controller_function(Polyvoice<CHANNEL_COUNT, VOICE_COUNT> *p, uint8_t channel, uint8_t control,
                                   uint8_t value) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                p->voices_[i]->UpdateMidiControl(control, value);
            }
        }

        static void map_controller_function(Polyvoice *p, uint8_t channel, uint8_t control, uint8_t value) {
            p->voice_map_->UpdateMidiControl(channel, control, value);
        }

        init_function init;
        process_function process;
        note_on_function note_on;
        note_off_function note_off;
        midi_controller_function update_midi_control;

        Voice *voices_[VOICE_COUNT] = {};
        bool initialized = false;
        VoiceMap<CHANNEL_COUNT> *voice_map_ = nullptr;

        t_sample frame_buffer[CHANNEL_COUNT] = {};
    public:
        explicit Polyvoice(Voice *voices[VOICE_COUNT]) :
                init(voices_init),
                process(voices_process),
                note_on(voices_note_on),
                note_off(voices_note_off),
                update_midi_control(voices_controller_function) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                voices_[i] = voices[i];
            }
        }

        explicit Polyvoice(VoiceMap<CHANNEL_COUNT> &voice_map) : voice_map_(&voice_map),
                                                                 init(map_init),
                                                                 process(map_process),
                                                                 note_on(map_note_on),
                                                                 note_off(map_note_off),
                                                                 update_midi_control(map_controller_function) {}

        void Init(t_sample sample_rate) {
            init(this, sample_rate);
        }

        void Process(t_sample *frame_out) {
            process(this, frame_out);
        }

        void NoteOn(const uint8_t note, const uint8_t velocity) {
            note_on(this, note, velocity);
        }

        void NoteOff(const uint8_t note, const uint8_t velocity) {
            note_off(this, note, velocity);
        }

        void UpdateMidiControl(uint8_t channel, uint8_t control, uint8_t value) {
            update_midi_control(this, channel, control, value);
        }


    };
}
#endif //OL_DSP_POLYVOICE_H
