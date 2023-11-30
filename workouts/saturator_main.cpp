//
// Created by Orion Letizi on 11/29/23.
//
#include <iostream>
#include "workout_buddy.h"
#include "ol_fxlib.h"
using namespace ol::workout;

ol::fx::SaturatorFx saturator1;
ol::fx::SaturatorFx saturator2;

void note_on_callback(uint8_t channel, uint8_t note, uint8_t velocity) {}

void note_off_callback(uint8_t channel, uint8_t note, uint8_t velocity) {}

void cc_callback(uint8_t channel, uint8_t control, uint8_t value) {}

uint64_t counter = 0;

void audio_callback(t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    counter++;
    saturator1.Process(&saturator1, in1, out1);
    saturator2.Process(&saturator2, in2, out2);
    if (counter % 20000 == 0) {
        std::cout << "Audio callback!" << std::endl;
        counter = 0;
    }
}

int main() {
    sp_saturator sat1;
    sp_data sp_data1;
    sp_saturator sat2;
    sp_data sp_data2;

    ol::fx::Saturator_Config(&saturator1, &sat1, &sp_data1);
    ol::fx::Saturator_Config(&saturator2, &sat2, &sp_data2);

    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    std::cout << "Hello, world!" << std::endl;

    ma_device audio_device;

    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback);
    ol::workout::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    saturator1.Init(&saturator1, Workout_SampleRate(&buddy));

    Workout_Start(&buddy);

    std::cout << "Send me some MIDI!" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
    return 0;

}