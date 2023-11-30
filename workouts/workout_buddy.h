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

    struct workout_buddy {
        void (*HandleNoteOn)(uint8_t channel, uint8_t note, uint8_t velocity) = nullptr;

        void (*HandleNoteOff)(uint8_t channel, uint8_t note, uint8_t value) = nullptr;

        void (*HandleMidiControlChange)(uint8_t channel, uint8_t controller, uint8_t value) = nullptr;

//        void (*Process)(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) = nullptr;

        RtMidiIn *midi_in = nullptr;
    };

    void Workout_RtMidiCallback(double deltatime, std::vector<unsigned char> *message, void *userData);

//    static void Workout_Miniaudio_Callback(ma_device *, void *output, const void *input, ma_uint32 frame_count);

    bool Workout_DoIExist();

    void Workout_Init(workout_buddy *);

    void Workout_Config(workout_buddy *, RtMidiIn *);
}


#endif //OL_DSP_WORKOUT_BUDDY_H
