//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_POLYVOICE_H
#define OL_DSP_POLYVOICE_H

#include "Voice.h"
#include "VoiceMap.h"

namespace ol::synth {
    class Polyvoice {
    private:
        // init functions
        typedef void (*init_function)(Polyvoice *, t_sample sample_rate);

        static void voices_init(Polyvoice *p, t_sample sample_rate);

        static void map_init(Polyvoice *p, t_sample sample_rate);

        init_function init;

        // process functions
        typedef void (*process_function)(Polyvoice *, t_sample *frame_out);
        static void voices_process(Polyvoice *, t_sample *frame_out);
        static void map_process(Polyvoice *, t_sample *frame_out);
        process_function process;

        // note on functions
        typedef void (*note_on_function)(Polyvoice *, uint8_t note, uint8_t velocity);
        static void voices_note_on(Polyvoice *, uint8_t note, uint8_t velocity);
        static void map_note_on(Polyvoice *, uint8_t note, uint8_t velocity);
        note_on_function note_on;

        // note off functions
        typedef void (*note_off_function) (Polyvoice *, uint8_t note, uint8_t velocity);
        static void voices_note_off(Polyvoice *, uint8_t note, uint8_t velocity);
        static void map_note_off(Polyvoice *, uint8_t note, uint8_t velocity);
        note_off_function note_off;

        // controller functions
        typedef void(*midi_controller_function)(Polyvoice *, uint8_t control, uint8_t value);
        static void voices_controller_function(Polyvoice *, uint8_t control, uint8_t value);
        static void map_controller_function(Polyvoice *, uint8_t control, uint8_t value);
        midi_controller_function update_midi_control;

        Voice **voices_ = nullptr;
        bool initialized = false;
        uint8_t voice_count = 0;
        VoiceMap *voice_map_ = nullptr;

    public:
        Polyvoice(Voice **voices, uint8_t voice_count) : voices_(voices), voice_count(voice_count),
                                                         voice_map_(nullptr),
                                                         init(voices_init),
                                                         process(voices_process),
                                                         note_on(voices_note_on),
                                                         note_off(voices_note_off),
                                                         update_midi_control(voices_controller_function){}

        explicit Polyvoice(VoiceMap &voice_map) : voice_map_(&voice_map),
                                                  init(map_init),
                                                  process(map_process),
                                                  note_on(map_note_on),
                                                  note_off(map_note_off),
                                                  update_midi_control(map_controller_function){}

        void Init(t_sample sample_rate);

        void Process(t_sample *frame_out);

        void NoteOn(uint8_t note, uint8_t velocity);

        void NoteOff(uint8_t note, uint8_t velocity);

        void UpdateMidiControl(uint8_t control, uint8_t value);


    };
}
#endif //OL_DSP_POLYVOICE_H
