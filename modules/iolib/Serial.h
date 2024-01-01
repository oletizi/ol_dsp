//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_SERIAL_H
#define OL_DSP_SERIAL_H

#include "ol_corelib.h"

namespace ol::io {
    class Serial {
        virtual size_t Write(const char *data, size_t size) = 0;

        virtual size_t Write(const uint8_t *data, size_t size) = 0;

        virtual size_t Available() = 0;

        virtual int Read() = 0;
    };
}
#endif //OL_DSP_SERIAL_H
