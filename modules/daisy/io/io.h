//
// Created by Orion Letizi on 11/28/23.
//

#ifndef OL_DSP_IO_H
#define OL_DSP_IO_H

#include "GpioPool.h"
#include "PolyvoiceControls.h"

namespace ol_daisy::io {
    struct Page {
        const char *name;
        Page *next = nullptr;
        Page *prev = nullptr;

        void (*UpdateKnob1)(t_sample value);

        void (*UpdateKnob2)(t_sample value);
    };
}
#endif //OL_DSP_IO_H
