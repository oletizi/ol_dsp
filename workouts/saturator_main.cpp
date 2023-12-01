//
// Created by Orion Letizi on 11/29/23.
//
#include <iostream>
#include "workout_buddy.h"
#include "ol_fxlib.h"
#include "ol_synthlib.h"

using namespace ol::workout;

ol::synthlib::ControlPanel cp;
ol::synthlib::Voice voice(&cp);

ol::fx::SaturatorFx saturator1;
ol::fx::SaturatorFx saturator2;

int notes_on = 0;

void note_on_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    notes_on++;
    voice.NoteOn(note, velocity);
}

void note_off_callback(uint8_t channel, uint8_t note, uint8_t velocity) {
    notes_on = notes_on > 0 ? notes_on - 1 : 0;
    voice.NoteOff(note);
}

void cc_callback(uint8_t channel, uint8_t control, uint8_t value) {
    Saturator_UpdateMidiControl(&saturator1, control, value);
    Saturator_UpdateMidiControl(&saturator2, control, value);
}

uint64_t counter = 0;

void audio_callback(t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    counter++;
    t_sample voice_out = voice.Process();
    t_sample next_in1 = voice_out + in1;
    t_sample next_in2 = voice_out + in2;
    saturator1.Process(&saturator1, next_in1, out1);
    saturator2.Process(&saturator2, next_in2, out2);
    if (counter % 20000 == 0) {
        counter = 0;
    }
}

int main() {
    sp_saturator sat1;
    sp_data sp_data1;
    sp_saturator sat2;
    sp_data sp_data2;

    Saturator_Config(&saturator1, &sat1, &sp_data1);
    Saturator_Config(&saturator2, &sat2, &sp_data2);

//    ol::fx::HyperTan hyperbolic_tangent;
//    Saturator_Config(&saturator1, &hyperbolic_tangent);
//    Saturator_Config(&saturator2, &hyperbolic_tangent);

    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    std::cout << "Hello, world!" << std::endl;

    ma_device audio_device;

    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback);

    ol::workout::InitStatus status = Workout_Init(&buddy);
    if (status != InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);
    voice.Init(sample_rate);
    saturator1.Init(&saturator1, sample_rate);
    saturator2.Init(&saturator2, sample_rate);


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