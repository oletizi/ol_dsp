//
// Created by Orion Letizi on 11/29/23.
//

#include "workout_buddy.h"

namespace ol::workout {
    void Workout_RtMidiCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
        auto *buddy = static_cast<workout_buddy *>(userData);
        unsigned int nBytes = message->size();
        if (nBytes > 0) {
            unsigned char status = message->at(0);
            int type = (int) (status >> 4);
            int channel = (int) (status & 0x0F);
            std::cout << "TYPE: " << type << "; CHAN: " << channel << std::endl;
            switch (type) {
                case 9:
                    std::cout << "NOTE ON!" << std::endl;
                    if (buddy->HandleNoteOn) {
                        buddy->HandleNoteOn(channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No Note On callback configured." << std::endl;
                    }
                    break;
                case 8:
                    std::cout << "NOTE OFF!" << std::endl;
                    if (buddy->HandleNoteOff) {
                        buddy->HandleNoteOff(channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No Note Off callback configured." << std::endl;
                    }
                    break;
                case 11:
                    std::cout << "MIDI CC!" << std::endl;
                    if (buddy->HandleMidiControlChange) {
                        buddy->HandleMidiControlChange(channel, message->at(1), message->at(2));
                    } else {
                        std::cout << "No CC callback configured." << std::endl;
                    }
                    break;
                default:
                    break;
            }
        }
    }


    void Workout_Init(workout_buddy *buddy) {

        // Initialize Midi
        auto midi_in = buddy->midi_in;
        uint16_t ports = midi_in->getPortCount();
        for (int i = 0; i < ports; i++) {

            std::cout << "Input port " << i << ": " << midi_in->getPortName(i) << std::endl;
            const std::string &port_name = midi_in->getPortName(i);
            // XXX: Fix the hard-coding
            if (strstr(port_name.c_str(), "Maschine") != nullptr) {
                std::cout << "Connecting to " << port_name << std::endl;
                midi_in->openPort(i);
            }
        }
        midi_in->setCallback(Workout_RtMidiCallback, buddy);
    }

    void Workout_Config(workout_buddy *buddy, RtMidiIn *mi) {
        buddy->midi_in = mi;
    }

    bool Workout_DoIExist() {
        return true;
    }
}