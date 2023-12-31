//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H

#include <cstdint>
#include "ol_teensy.h"

#define BUF_SIZE 256

int led = 13;
int counter = 0;
bool led_state = false;
uint8_t buf[BUF_SIZE]{};
uint8_t inbuf[BUF_SIZE]{};
unsigned int bytes_read = 0;

void doSetup() {
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
    for (int i = 0; i < BUF_SIZE; i++) {
        inbuf[i] = 0;
    }

    Serial1.println("Teensy!");
    for (int i = 0; i < Serial1.available() && bytes_read < BUF_SIZE; i++) {
        inbuf[i] = Serial1.read();
        bytes_read++;
    }

    if (bytes_read > 0) {
        Serial.write(inbuf, bytes_read);
    }
    bytes_read = 0;
}

#endif //OL_DSP_LOOP_H
