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
DMAMEM uint16_t fb_internal[240 * 320];  // the 'internal' frame buffer
DMAMEM uint16_t fb[LX * LY];
// main framebuffer we draw onto.
ILI9341_T4::DiffBuffStatic<4096> diff1; // a first diff buffer with 4K memory (statically allocated)
ILI9341_T4::DiffBuffStatic<4096> diff2; // and a second one.

ILI9341_T4::ILI9341Driver tft(TFT_CS, SPI_DC, SPI_CLK, SPI_MOSI, SPI_MISO, TFT_RESET);
tgx::Image<tgx::RGB565> im(fb, TFT_HOR_RES, TFT_VER_RES);
const tgx::RGB565 ol_orange(31, 21, 0);
const tgx::RGB565 ol_dark_gray(11, 22, 11);

tgx::RGB565 fg_color = tgx::RGB565_Black;//ol_orange;
tgx::RGB565 bg_color = tgx::RGB565_White;//ol_dark_gray;

ILI9341_t3_font_t font;
//
// TODO: Try these libs:
//  * https://github.com/vindar/ILI9341_T4
//  * https://github.com/KurtE/ILI9341_t3n
//
ol::app::synth::SynthGuiConfig config{};
ol::app::synth::SynthGui gui(config);

namespace ol::app::synth {
    class TgxGraphics : public Graphics {
    public:
        explicit TgxGraphics(tgx::Image<tgx::RGB565> &canvas, const ILI9341_t3_font_t font, const tgx::RGB565 fg_color,
                             const tgx::RGB565 bg_color)
                : canvas_(canvas), font_(font), fg_color_(fg_color), bg_color_(bg_color) {}

        void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
            canvas_.drawLine(startX, startY, endX, endY, fg_color_, 1);
        }

        void DrawRect(int x, int y, int width, int height, int line_width) override {
            canvas_.drawRect(x, y, width, height, fg_color_, 1);
        }

        void FillRect(int x, int y, int width, int height) override {
            canvas_.fillRect(x, y, width, height, fg_color_, 1);
        }

        void WritePixel(int x, int y, Color c) override {
            canvas_.drawPixel(x, y, fg_color_);
        }

        void Print(std::string text, Rectangle area) override {
//            DPRINTF("font_.bits_height: %d, cap_heigth: %d, line_space: %d\n", font_.bits_height, font_.cap_height,
//                    font_.line_space);
            tgx::iVec2 point(area.point.x, area.point.y + font_.line_space);
            canvas_.drawText(text.c_str(), point, fg_color_, font_, true);
        }


    private:
        tgx::Image<tgx::RGB565> &canvas_;
        tgx::RGB565 bg_color_;
        tgx::RGB565 fg_color_;
        ILI9341_t3_font_t font_;
    };
}

void handleMidiCC(uint8_t channel, uint8_t cc, byte value) {
    Serial.printf("CC: chan: %d, ctl: %d, val: %d\n", channel, cc, value);
    auto control = ol::ctl::Control(cc, value);
    DPRINTF("handleMidiCC: Controller: controller: %d; value: %d\n", control.GetController(), control.GetMidiValue());
    gui.ControlChange(control);
}

void doSetup() {

    usbMIDI.setHandleControlChange(handleMidiCC);

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

    font = font_tgx_OpenSans_12;
    tft.setFramebuffer(fb_internal);     // register the internal framebuffer: this activates double buffering
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setRotation(1);
    im.fillScreen(bg_color);
    gui.SetSize(128, 64);
    gui.Resized();
}

int counter = 0;
int direction = 1;


void doLoop() {
    usbMIDI.read();
    im.fillScreen(bg_color);

    auto g = ol::app::synth::TgxGraphics(im, font, fg_color, bg_color);
    gui.Paint(g);
    tft.overlayFPS(fb);                  // optional: draw the current FPS on the top right corner of the framebuffer
    tft.update(fb);
    im.drawLine(96, 46, 115, 48, fg_color);
    counter += direction;
    if (counter % TFT_HOR_RES == 0) {
        direction *= -1;
    }
}

#endif //OL_DSP_TFT_LOOP_H
