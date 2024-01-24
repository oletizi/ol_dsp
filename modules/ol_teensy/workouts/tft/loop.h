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
#define DMAMEM
#else
//#include "ol_dsp.h"
//#include "ol_teensy.h"
#endif
#include "SPI.h"
#include <ILI9341_T4.h>
#include "tgx.h"
#include <font_tgx_OpenSans.h>

#include "ol_dsp.h"
//#include "ILI9341Driver.h"

#define SPI_WRITE_SPEED 30000000
#define SPI_READ_SPEED   6500000
#define SPI_MOSI        11
#define SPI_MISO        12
#define SPI_MISO       255
#define SPI_CLK         13
#define SPI_DC           9
#define TFT_CS          10
#define TFT_RESET      255

#define TFT_HOR_RES 320
#define TFT_VER_RES 240

#define OL_BLACK tgx::RGB565_Black

const int LX = TFT_HOR_RES;
const int LY = TFT_VER_RES;


//DMAMEM
DMAMEM uint16_t fb_internal[240*320];  // the 'internal' frame buffer
DMAMEM uint16_t fb[LX*LY];
// main framebuffer we draw onto.
ILI9341_T4::DiffBuffStatic<4096> diff1; // a first diff buffer with 4K memory (statically allocated)
ILI9341_T4::DiffBuffStatic<4096> diff2; // and a second one.

ILI9341_T4::ILI9341Driver tft(TFT_CS, SPI_DC, SPI_CLK, SPI_MOSI, SPI_MISO, TFT_RESET);
tgx::Image<tgx::RGB565> im(fb, TFT_HOR_RES, TFT_VER_RES);
//
// TODO: Try these libs:
//  * https://github.com/vindar/ILI9341_T4
//  * https://github.com/KurtE/ILI9341_t3n
//

void doSetup() {
    Serial.begin(9600);
    Serial.println("Setting up tft...");

    Serial.println("Setting tft output...");
    tft.output(&Serial);
    Serial.println("Done.");

    Serial.println("Starting tft...");
    while (!tft.begin(SPI_WRITE_SPEED, SPI_READ_SPEED)) {
        Serial.println("Can't start tft...");
        delay(1000);
    }
    Serial.println("Done.");

    Serial.println("Setting tft refresh rate...");
    tft.setRefreshRate(120); // set the display refresh rate around 120Hz
    Serial.println("Done");

    Serial.println("Setting tft vspacing...");
    tft.setVSyncSpacing(2); // enable vsync and set framerate = refreshrate/2 (typical choice)
    Serial.println("Done.");


    Serial.println("Tft started.");

    tft.setFramebuffer(fb_internal);     // register the internal framebuffer: this activates double buffering
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setRotation(1);
    im.fillScreen(OL_BLACK);
}

int counter = 0;
int direction = 1;
tgx::RGB565 ol_orange(31, 21, 0);
tgx::RGB565 ol_dark_gray(11,22,11);

void doLoop() {
    im.fillScreen(ol_dark_gray);
    im.drawRect(0, 0, tft.width(), tft.height() / 3, ol_orange);
    im.fillRect(0, 0, counter, tft.height() / 3, ol_orange);
    tft.overlayFPS(fb);                  // optional: draw the current FPS on the top right corner of the framebuffer
    tft.update(fb);
    counter += direction;
    if (counter % TFT_HOR_RES == 0) {
        direction *= -1;
    }
}

#endif //OL_DSP_TFT_LOOP_H
