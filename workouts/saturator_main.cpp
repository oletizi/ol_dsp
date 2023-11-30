//
// Created by Orion Letizi on 11/29/23.
//
#include <iostream>
#include "workout_buddy.h"

void note_on_callback(uint8_t i, uint8_t i1, uint8_t i2) {}
void note_off_callback(uint8_t i, uint8_t i1, uint8_t i2) {}
void cc_callback(uint8_t channel, uint8_t control, uint8_t value) {}

int main() {
    ol::workout::workout_buddy buddy;
    RtMidiIn midi_in;
    std::cout << "Hello, world!" << std::endl;

    Workout_Config(&buddy, &midi_in, note_on_callback, note_off_callback, cc_callback);
    Workout_Init(&buddy);
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