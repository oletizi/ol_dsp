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
    //Serial1.begin(9600);
    //Serial1.begin(57600);
    Serial1.begin(115200);
    pinMode(led, OUTPUT);
    // put your setup code here, to run once:
    Serial.println("This may be sent before your PC is able to receive");
    while (!Serial) {
        // wait for Arduino Serial Monitor to be ready
    }
    Serial.println("This line will definitely appear in the serial monitor");
}


ol::ctl::Control ctl{CC_FILTER_CUTOFF, 1};

void doLoop() {

    for (int i = 0; i < Serial1.available() && bytes_read < BUF_SIZE; i++) {
        inbuf[i] = Serial1.read();
        bytes_read++;
        if (bytes_read == sizeof(inbuf)) {
            Serial.printf("Teensy: printing inbuf. Bytes read: %d\n", bytes_read);
            Serial.write(inbuf, sizeof(inbuf));
            for (int j=0; j< bytes_read; j++) {
                Serial.print(inbuf[j]);
            }
            bytes_read = 0;
        }
    }



    counter++;
//    if (counter == 5000000) {
      if (counter == 50000) {
        counter = 0;
        int cc_1 = analogRead(A9);
        Serial.printf("cc_1: %d\n", cc_1);

        ol::ctl::Control c {CC_FILTER_CUTOFF, cc_1};
        std::vector<uint8_t> serialized;
        serializer.SerializeControl(c, serialized);
        uint8_t data[serialized.size()];
        for (int i=0; i<sizeof(data); i++) {
            data[i] = serialized[i];
        }
        Serial1.write(data, sizeof(data));
        Serial.printf("Teensy: counter reset. sent ctl: {controller: %d, value: %d}\n", c.controller, c.value);
        Serial.printf("  data size: %d; first byte: %d; last byte: %d\n", sizeof(data), data[0], data[sizeof(data) - 1]);
    }
}

#endif //OL_DSP_LOOP_H
