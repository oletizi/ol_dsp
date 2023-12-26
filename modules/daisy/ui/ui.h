//
// Created by Orion Letizi on 11/28/23.
//

#ifndef OL_DSP_UI_H
#define OL_DSP_UI_H
#include "GpioPool.h"
#include "PolyvoiceInputs.h"

struct Page {
    const char *name;
    Page *next = nullptr;
    Page *prev = nullptr;

    void (*UpdateKnob1)(t_sample value);

    void (*UpdateKnob2)(t_sample value);
};

#endif //OL_DSP_UI_H
