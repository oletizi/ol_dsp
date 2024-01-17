//
// Created by Orion Letizi on 1/1/24.
//

#ifndef OL_DSP_TEENSYSERIAL_H
#define OL_DSP_TEENSYSERIAL_H

#include "Arduino.h"
#include "iolib/ol_iolib.h"

namespace ol_teensy::io {
    class TeensySerial : public ol::io::Serial {
    public:
        int Write(const char *data, int size) override { return 0; };

        int Write(const uint8_t *data, int size) override { return 0; };

        int Write(std::vector<uint8_t> data, int size) override { return 0; };

        int Available() override { return Serial1.available(); };

        int Read() override { return Serial1.read(); };

    };
}


#endif //OL_DSP_TEENSYSERIAL_H
