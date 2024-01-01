//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_SERIAL_H
#define OL_DSP_SERIAL_H

#include "ol_corelib.h"

namespace ol::io {
    class Serial {
        virtual int Write(const char *data, size_t size) = 0;

        virtual int Write(const uint8_t *data, size_t size) = 0;

        virtual int Available() = 0;

        virtual int Read() = 0;
    };
}
#endif //OL_DSP_SERIAL_H
