//
// Created by Orion Letizi on 11/29/23.
//
#include <iostream>
#include "../workout_buddy.h"
#include "ol_fxlib.h"

#define CHANNEL_COUNT 2

using namespace ol::io;
using namespace ol::synth;
using namespace ol::fx;

SynthVoice<CHANNEL_COUNT> voice;
SaturatorFx<CHANNEL_COUNT> saturator;


int notes_on = 0;

void note_on_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {
    notes_on++;
    voice.NoteOn(note, velocity);
}

void note_off_callback(workout_buddy *, uint8_t channel, uint8_t note, uint8_t velocity) {
    notes_on = notes_on > 0 ? notes_on - 1 : 0;
    voice.NoteOff(note, velocity);
}

void cc_callback(workout_buddy *, uint8_t channel, uint8_t control, uint8_t value) {
    saturator.UpdateMidiControl(control, value);
}

uint64_t counter = 0;
t_sample frame_buffer[CHANNEL_COUNT]{};

void audio_callback(workout_buddy *, t_sample &in1, t_sample &in2, t_sample *out1, t_sample *out2) {
    voice.Process(frame_buffer);
    saturator.Process(frame_buffer, frame_buffer);
    *out1 = frame_buffer[0];
    *out2 = frame_buffer[1];
}

int main() {

    ol::io::workout_buddy buddy;
    RtMidiIn midi_in;
    std::cout << "Hello, world!" << std::endl;

    ma_device audio_device;

    Workout_Config(&buddy, &midi_in, &audio_device, note_on_callback, note_off_callback, cc_callback, audio_callback);

    ol::io::InitStatus status = Workout_Init(&buddy);
    if (status != ol::io::InitStatus::Ok) {
        return status;
    }
    t_sample sample_rate = Workout_SampleRate(&buddy);

    voice.Init(sample_rate);
    saturator.Init(sample_rate);


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