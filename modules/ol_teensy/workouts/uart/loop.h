//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H

#include <cstdint>
#include <deque>
#include <MIDI.h>
#include <TimeLib.h>

#ifdef TEENSY_LOCAL

#include "usb_serial.h"
#include "ol_teensy.h"

#else
#include "ol_dsp.h"
#include "ol_teensy.h"
#endif

#define BUF_SIZE 256

using namespace ol::ctl;

int led = LED_BUILTIN;
int counter = 0;
uint8_t buf[BUF_SIZE]{};
uint8_t inbuf[BUF_SIZE]{};
unsigned int bytes_read = 0;
ol_teensy::io::TeensySerial teensy_serial;
ol::io::SimpleSerializer serializer(teensy_serial);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

IntervalTimer control_timer;
Control control{};


void write_control(const Control &c) {

    std::vector<uint8_t> serialized;
    serializer.SerializeControl(c, serialized);
    uint8_t data[serialized.size()];
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = serialized[i];
    }
    Serial1.write(data, sizeof(data));
}

void send_control(int channel, int value) {
    uint8_t controller = 0;
    switch (channel) {
        case A0:
            controller = CC_FILTER_CUTOFF;
            break;
        case A1:
            controller = CC_FILTER_RESONANCE;
            break;
        case A2:
            controller = CC_ENV_FILT_AMT;
            break;
        case A3:
            controller = CC_ENV_FILT_D;
            break;
        default:
            break;
    }
    if (controller > 0) {
        //const Control c = {controller, value};
        control.controller = controller;
        control.value = value;
        write_control(control);
    }
}

void control_handler() {

    //Serial.println("Control Handler!");
    auto channel = A0;
    auto value = analogRead(channel);
    send_control(channel, value);
    send_control(A1, analogRead(A1));
    send_control(A2, analogRead(A2));
    send_control(A3, analogRead(A3));
//    Serial.printf("A0: %d\n", analogRead(A0));
//    Serial.printf("A1: %d\n", analogRead(A1));
//    Serial.printf("A2: %d\n", analogRead(A2));
//    Serial.printf("A3: %d\n", analogRead(A3));
//    Serial.printf("A4: %d\n", analogRead(A4));
//    Serial.printf("A5: %d\n", analogRead(A5));
//    Serial.printf("A6: %d\n", analogRead(A6));
//    Serial.printf("A7: %d\n", analogRead(A7));
//    Serial.printf("A8: %d\n", analogRead(A8));
//    Serial.printf("A9: %d\n", analogRead(A9));
};


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

    control_timer.priority(128);
    control_timer.begin(control_handler, 1000 * 1000);
}

ol::ctl::Control ctl{CC_FILTER_CUTOFF, 1};

void doLoop() {
    if (MIDI.read()) {
        Control pitch{CC_VOICE_PITCH, 0};
        Control gate{CC_VOICE_GATE, 0};

        bool write_controls = true;
        switch (MIDI.getType()) {
            case midi::NoteOn:
                digitalWrite(led, HIGH);
                pitch.value = MIDI.getData1();
                gate.value = 1;
                break;
            case midi::NoteOff:
                digitalWrite(led, LOW);
                pitch.value = MIDI.getData1();
                gate.value = 0;
                break;
            default:
                write_controls = false;
                break;
        }
        if (write_controls) {
            write_control(pitch);
            write_control(gate);
//            std::vector<uint8_t> serialized;
//            serializer.SerializeControl(pitch, serialized);
//            serializer.SerializeControl(gate, serialized);
//            uint8_t data[serialized.size()];
//            for (int i = 0; i < sizeof(data); i++) {
//                data[i] = serialized[i];
//            }
//            Serial1.write(data, sizeof(data));

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
