//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H

#include <cstdint>
#include <deque>

#ifdef TEENSY_LOCAL

#include "usb_serial.h"
#include "ol_teensy.h"

#else
#include "ol_dsp.h"
#include "ol_teensy.h"
#endif

#define BUF_SIZE 256

int led = 13;
int counter = 0;
bool led_state = false;
uint8_t buf[BUF_SIZE]{};
uint8_t inbuf[BUF_SIZE]{};
unsigned int bytes_read = 0;
ol_teensy::io::TeensySerial teensy_serial;
ol::io::SimpleSerializer serializer(teensy_serial);
std::deque<ol::ctl::Control> controls;

class TeensyListener : public ol::io::ControlListener {
public:
    void HandleControl(ol::ctl::Control control) override {
        Serial.println("HandleControl!");
        controls.push_back(control);
    }

};

void doSetup() {
    analogReadResolution(13);
    analogReadAveraging(16);
    Serial1.begin(9600);
    pinMode(led, OUTPUT);
    // put your setup code here, to run once:
    Serial.println("This may be sent before your PC is able to receive");
    while (!Serial) {
        // wait for Arduino Serial Monitor to be ready
    }
    Serial.println("This line will definitely appear in the serial monitor");
}


void doLoop() {
    counter++;
    if (counter % 100 == 0) {
        int cc_1 = analogRead(A9);
        Serial.printf("cc_1: %d\n", cc_1);
    }

    for (unsigned char &i: inbuf) {
        i = 0;
    }

    Serial1.println("Teensy!");
//    for (int i = 0; i < Serial1.available() && bytes_read < BUF_SIZE; i++) {
//        inbuf[i] = Serial1.read();
//        bytes_read++;
//    }
    if (Serial1.available()) {
        Serial.printf("Parsing controls from Serial1... Serial1.available(): %d\n", Serial1.available());
        serializer.Process();
    }

    while (!controls.empty()) {
        auto &c = controls.front();
        Serial.printf("Control: controller: %d, value: %d\n", c.controller, c.value);
        controls.pop_front();
    }
//    if (bytes_read > 0) {
//        Serial.write(inbuf, bytes_read);
//    }

    bytes_read = 0;
    if (counter == 100000) {
        counter = 0;
    }
}

#endif //OL_DSP_LOOP_H
