//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_MIDI_LOOP_H
#define OL_DSP_MIDI_LOOP_H

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
#include "Arduino.h"
//#include "WireKinetis.h"
//#include "usb_serial.h"
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
#include "app/synth/SynthConfig.h"
//#include "ILI9341Driver.h"
#include <MIDI.h>

#define SPI_WRITE_SPEED 30000000
#define SPI_READ_SPEED   6500000
#define SPI_MOSI        11
#define SPI_MISO        12
//#define SPI_MISO       255
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
DMAMEM uint16_t fb_internal[240 * 320];  // the 'internal' frame buffer
DMAMEM uint16_t fb[LX * LY];
// main framebuffer we draw onto.
ILI9341_T4::DiffBuffStatic<4096> diff1; // a first diff buffer with 4K memory (statically allocated)
ILI9341_T4::DiffBuffStatic<4096> diff2; // and a second one.

ILI9341_T4::ILI9341Driver tft(TFT_CS, SPI_DC, SPI_CLK, SPI_MOSI, SPI_MISO, TFT_RESET);
tgx::Image<tgx::RGB565> im(fb, TFT_HOR_RES, TFT_VER_RES);
const tgx::RGB565 ol_orange(31, 21, 0);
const tgx::RGB565 ol_dark_gray(11, 22, 11);

tgx::RGB565 fg_color = tgx::RGB565_Black;
tgx::RGB565 bg_color = tgx::RGB565_White;

ILI9341_t3_font_t font;

void handleMidiCC(uint8_t channel, uint8_t cc, byte value) {
    DPRINTF("CC: chan: %d, ctl: %d, val: %d\n", channel, cc, value);
    auto c = ol::ctl::Control(cc, value);
    DPRINTF("handleMidiCC: Controller: controller: %d; value: %d\n", c.GetController(), c.GetMidiValue());
}

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void doSetup() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
//    usbMIDI.setHandleControlChange(handleMidiCC);
    Serial.begin(57600);
    Serial.println("MIDI input test.");
}

int counter = 0;
int direction = 1;


void doLoop() {
    if (MIDI.read()) {
        Serial.println("MIDI!");
    }
    counter += direction;
    if (counter % TFT_HOR_RES == 0) {
        direction *= -1;
    }
}

#endif //OL_DSP_TFT_LOOP_H
