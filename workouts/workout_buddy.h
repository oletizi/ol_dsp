//
// Created by Orion Letizi on 11/29/23.
//

#ifndef OL_DSP_WORKOUT_BUDDY_H
#define OL_DSP_WORKOUT_BUDDY_H

#define MINIAUDIO_IMPLEMENTATION
//#include "miniaudio.h"
#include "RtMidi.h"
#include "corelib/ol_corelib.h"


namespace ol::workout {

    typedef
    void (*MidiNoteOnCallback)(uint8_t channel, uint8_t note, uint8_t velocity);

    typedef
    void (*MidiNoteOffCallback)(uint8_t channel, uint8_t note, uint8_t value);

    typedef
    void (*MidiControlChangeCallback)(uint8_t channel, uint8_t controller, uint8_t value);

    struct workout_buddy {

        MidiNoteOnCallback HandleNoteOn = nullptr;

        MidiNoteOffCallback HandleNoteOff = nullptr;

        MidiControlChangeCallback HandleMidiControlChange = nullptr;

        //        void (*Process)(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) = nullptr;

        RtMidiIn *midi_in = nullptr;
    };

    void Workout_RtMidiCallback([[maybe_unused]] double deltatime, std::vector<unsigned char> *message, void *userData);

//    static void Workout_Miniaudio_Callback(ma_device *, void *output, const void *input, ma_uint32 frame_count);


    void Workout_Init(workout_buddy *);

    void
    Workout_Config(workout_buddy *, RtMidiIn *, MidiNoteOnCallback, MidiNoteOffCallback, MidiControlChangeCallback);
}


#endif //OL_DSP_WORKOUT_BUDDY_H
