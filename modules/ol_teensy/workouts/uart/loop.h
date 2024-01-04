//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H

#include <cstdint>
#include <deque>
#include <MIDI.h>

#ifdef TEENSY_LOCAL

#include "usb_serial.h"
#include "ol_teensy.h"

#else
#include "ol_dsp.h"
#include "ol_teensy.h"
#endif

#define BUF_SIZE 256

using namespace ol::ctl;

int led = 13;
int counter = 0;
uint8_t buf[BUF_SIZE]{};
uint8_t inbuf[BUF_SIZE]{};
unsigned int bytes_read = 0;
ol_teensy::io::TeensySerial teensy_serial;
ol::io::SimpleSerializer serializer(teensy_serial);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

void doSetup() {
    analogReadResolution(13);
    analogReadAveraging(16);
    //Serial1.begin(9600);
    Serial1.begin(57600);
//    Serial1.begin(115200);


    pinMode(led, OUTPUT);
    // put your setup code here, to run once:
    Serial.println("This may be sent before your PC is able to receive");
    while (!Serial) {
        // wait for Arduino Serial Monitor to be ready
    }
    Serial.println("This line will definitely appear in the serial monitor");
    Serial.println("Starting midi...");
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

ol::ctl::Control ctl{CC_FILTER_CUTOFF, 1};

void doLoop() {
    if (MIDI.read()) {
        Control pitch{CC_VOICE_PITCH, 0};
        Control gate{CC_VOICE_GATE, 0};

        bool write_controls = true;
        switch (MIDI.getType()) {
            case midi::NoteOn:
                pitch.value = MIDI.getData1();
                gate.value = 1;
                break;
            case midi::NoteOff:
                pitch.value = MIDI.getData1();
                gate.value = 0;
                break;
            default:
                write_controls = false;
                break;
        }
        if (write_controls) {
            std::vector<uint8_t> serialized;
            serializer.SerializeControl(pitch, serialized);
            serializer.SerializeControl(gate, serialized);
            uint8_t data[serialized.size()];
            for (int i=0; i<sizeof(data); i++) {
                data[i] = serialized[i];
            }
            Serial.println("Writing midi data as control data...");
            Serial1.write(data, sizeof(data));
        }
    }

    for (int i = 0; i < Serial1.available() && bytes_read < BUF_SIZE; i++) {
        inbuf[i] = Serial1.read();
        bytes_read++;
        if (bytes_read == sizeof(inbuf)) {
            Serial.printf("Teensy: printing inbuf. Bytes read: %d\n", bytes_read);
            Serial.write(inbuf, sizeof(inbuf));
            for (int j = 0; j < bytes_read; j++) {
                Serial.print(inbuf[j]);
            }
            bytes_read = 0;
        }
    }


    counter++;
//    if (counter == 5000000) {
    if (counter == 50000) {
        counter = 0;
    }
}

#endif //OL_DSP_LOOP_H
