//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_TFT_LOOP_H
#define OL_DSP_TFTLOOP_H

#define TEENSY_DEBUG

#ifdef TEENSY_DEBUG
#define DPRINTLN(X) Serial.println(X);
#define DPRINTF(...) Serial.printf(__VA_ARGS__);
#else
#define DPRINTLN(X)
#define DPRINTF(...)
#endif

#ifdef TEENSY_LOCAL

#include <string>
#include "WireKinetis.h"
#include "usb_serial.h"
#include "ol_teensy.h"

#else
//#include "ol_dsp.h"
//#include "ol_teensy.h"
#endif

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"

#define SPI_MOSI        11
#define SPI_MISO        12
#define SPI_CLK         13
#define SPI_DC           9
#define TFT_CS          10
#define TFT_RESET      255

ILI9341_t3 tft = ILI9341_t3(TFT_CS, SPI_DC, TFT_RESET, SPI_MOSI, SPI_CLK, SPI_MISO);


void doSetup() {
    tft.begin();
    Serial.begin(9600);
}

void doLoop() {
    DPRINTF("%d: Width: %d, Height: %d\n", millis(), tft.width(), tft.height())
    tft.fillScreen(ILI9341_GREEN);
    delay(500);
}

#endif //OL_DSP_TFT_LOOP_H
