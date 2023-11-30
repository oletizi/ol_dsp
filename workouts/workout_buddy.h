//
// Created by Orion Letizi on 11/29/23.
//

#ifndef OL_DSP_WORKOUT_BUDDY_H
#define OL_DSP_WORKOUT_BUDDY_H

#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include "RtMidi.h"
#include "corelib/ol_corelib.h"


namespace ol::workout {

    enum InitStatus {
        Ok,
        MidiInitError,
        AudioInitError
    };

    typedef
    void (*MidiNoteOnCallback)(uint8_t channel, uint8_t note, uint8_t velocity);

    typedef
    void (*MidiNoteOffCallback)(uint8_t channel, uint8_t note, uint8_t value);

    typedef
    void (*MidiControlChangeCallback)(uint8_t channel, uint8_t controller, uint8_t value);

    typedef
    void (*AudioCallback)(t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2);

    struct workout_buddy {

        MidiNoteOnCallback HandleNoteOn = nullptr;

        MidiNoteOffCallback HandleNoteOff = nullptr;

        MidiControlChangeCallback HandleMidiControlChange = nullptr;

        AudioCallback Process = nullptr;

        ma_device *audio_device = nullptr;

        RtMidiIn *midi_in = nullptr;
    };

    t_sample Workout_SampleRate(workout_buddy *);

    void Workout_Start(workout_buddy *);

    InitStatus Workout_Init(workout_buddy *);

    void
    Workout_Config(workout_buddy *, RtMidiIn *, ma_device *, MidiNoteOnCallback, MidiNoteOffCallback,
                   MidiControlChangeCallback, AudioCallback);
}


#endif //OL_DSP_WORKOUT_BUDDY_H
