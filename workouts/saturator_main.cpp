//
// Created by Orion Letizi on 11/29/23.
//
#include <iostream>
#include "workout_buddy.h"
int main() {
    ol::workout::workout_buddy buddy;
    ol::workout::Workout_DoIExist();
    RtMidiIn midi_in;
    std::cout << "Hello, world!" << std::endl;

    Workout_Config(&buddy, &midi_in);
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