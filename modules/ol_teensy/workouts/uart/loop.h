//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H

#define TEENSY_DEBUG
#ifdef TEENSY_LOCAL

#include <string>
#include "WireKinetis.h"
#include "usb_serial.h"
#include "ol_teensy.h"

#else
#include "ol_dsp.h"
#include "ol_teensy.h"
#endif

#include <cstdint>
#include <deque>
#include <MIDI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "guilib/ol_guilib.h"

#define BUF_SIZE 256
#define NOISE_FLOOR 10
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define COLUMN_WIDTH 32
#define METER_WIDTH 31
#define METER_HEIGHT 15
#define LINE_HEIGHT 16
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define DISPLAY_PERIOD 100


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, OLED_RESET);

using namespace ol::ctl;
using namespace ol::gui;

class AdafruitGraphics : public Graphics {
public:
    explicit AdafruitGraphics(Adafruit_SSD1306 &d) : display_(d) {}

    void DrawRect(int x, int y, int width, int height, int line_width) override {
        display_.drawRect(x, y, width, height, SSD1306_WHITE);
    }

    void FillRect(int x, int y, int width, int height) override {
        display_.fillRect(x, y, width, height, SSD1306_WHITE);
    }

    void Print(std::string text, Rectangle area) override {
        display_.setCursor(area.point.x, area.point.y);
        display_.print(text.c_str());
    }

    void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
        display_.drawLine(startX, startY, endX, endY, SSD1306_WHITE);
    }

    void WritePixel(int x, int y, Color c) override {
        display_.drawPixel(x, y, c == Color::White ? SSD1306_WHITE : SSD1306_BLACK);
    }

private:
    Adafruit_SSD1306 &display_;
    char cbuf[256]{};
};

int led = LED_BUILTIN;

int counter = 0;
uint8_t buf[BUF_SIZE]{};
unsigned int bytes_read = 0;
ol_teensy::io::TeensySerial teensy_serial;
ol::io::SimpleSerializer serializer(teensy_serial);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);


uint64_t note_on_count = 0;
uint64_t note_off_count = 0;


AdafruitGraphics graphics(display);

//SynthAppConfig app_config{
//        Dimension{SCREEN_WIDTH, SCREEN_HEIGHT},
//        Control{CC_FILTER_CUTOFF, 0},
//        Control{CC_FILTER_RESONANCE, 0},
//        Control{CC_ENV_FILT_AMT, 0},
//        Control{CC_FILTER_DRIVE, 0},
//
//        Control{CC_ENV_FILT_A, 0},
//        Control{CC_ENV_FILT_D, 0},
//        Control{CC_ENV_FILT_S, 0},
//        Control{CC_ENV_FILT_R, 0}
//};
SynthAppConfig app_config{};
SynthGui app(app_config);

auto display_checkpoint = millis();

uint64_t control_tx_count = 0;
auto tx_checkpoint = millis();
auto tx_rate = 0.f;

void write_control(const Control &c) {
    auto now = millis();
    auto delta = now - tx_checkpoint;
    if (delta > 100) {
        tx_checkpoint = now;
        tx_rate = float(control_tx_count) / float(delta); // XXX: divide by zero waiting to happen
        control_tx_count = 0;
    }
    std::vector<uint8_t> serialized;
    serializer.SerializeControl(c, serialized);
    uint8_t data[serialized.size()];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = serialized[i];
    }
    Serial1.write(data, sizeof(data));
    control_tx_count++;
}


int sample_control(uint8_t pin) {
    return 4096 - analogRead(pin);
//    return analogRead(pin);
}

void update_control(Control &c, int new_value) {
    auto delta = c.value - new_value;
    delta = delta < 0 ? delta * -1
                      : delta; // XXX: workaround for bug in teensy library that mangles abs() and std::abs()
    bool above_noise = delta > NOISE_FLOOR;
    if (above_noise) {
        DPRINTF("Updating control %d: %d\n", c.controller, new_value);
        c.value = new_value;
        app.ControlChange(c);
        DPRINTF("Writing control: %d: %d\n", c.controller, c.value);
        write_control(c);
    }
}

void control_handler() {
    update_control(app_config.filter_cutoff, sample_control(A0));
    update_control(app_config.filter_resonance, sample_control(A1));
    update_control(app_config.filter_env_amt, sample_control(A2));
    update_control(app_config.filter_drive, sample_control(A3));

    update_control(app_config.filter_attack, sample_control(A4));
    update_control(app_config.filter_decay, sample_control(A5));
    update_control(app_config.filter_sustain, sample_control(A6));
    update_control(app_config.filter_release, sample_control(A7));
};

void handleNoteOn(byte channel, byte note, byte velocity) {
    digitalWrite(led, HIGH);
    Control pitch{CC_VOICE_PITCH, note};
    Control gate{CC_VOICE_GATE, velocity};
    gate.value = velocity;
    write_control(pitch);
    write_control(gate);
    note_on_count++;
    DPRINTF("Note ON: pitch: %d, velocity: %d\n", pitch.value, velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
    digitalWrite(led, LOW);
    Control pitch{CC_VOICE_PITCH, note};
    Control gate{CC_VOICE_GATE, velocity};
    gate.value = velocity;
    write_control(pitch);
    write_control(gate);
    note_off_count++;
    DPRINTF("Note OFF: pitch: %d, velocity: %d\n", pitch.value, velocity);
}

void midi_handler() {
    MIDI.read();
}

void d_cursor(int line_number, int column) {
    display.setCursor(column * COLUMN_WIDTH, line_number * LINE_HEIGHT);
}


void display_handler() {
    display.clearDisplay();
    app.Paint(graphics);
    display.display();
}

void doSetup() {
    analogReadResolution(13);
    analogReadAveraging(16);
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
    pinMode(A3, INPUT);
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);
    pinMode(A6, INPUT);
    pinMode(A7, INPUT);
    pinMode(A7, INPUT_PULLDOWN);
    pinMode(A9, INPUT_PULLDOWN);

    Serial1.begin(115200);

    pinMode(led, HIGH);

    DPRINTLN("Starting midi...");
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.begin(MIDI_CHANNEL_OMNI);


    DPRINTLN("Starting display...");
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("display1 allocation failed"));
    } else {
        DPRINTLN("Display started.");
    }

    DPRINTLN("Drawing splash screen to display1...");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.cp437(true);
    display.write("Hello!");
    display.display();

    DPRINTLN("Done drawing splash screen to display1.");
    DPRINTLN("Setting up control display...");
    display.display();
    DPRINTLN("Done setting up control display.");

}

ol::ctl::Control ctl{CC_FILTER_CUTOFF, 1};

void doLoop() {
    midi_handler();
    control_handler();

    auto now = millis();
    auto display_delta = now - display_checkpoint;
    if (display_delta > DISPLAY_PERIOD) {
        display_handler();
        display_checkpoint = now;
    }
    delay(1);
    counter++;
    if (counter == 50000) {
        counter = 0;
    }
}

#endif //OL_DSP_LOOP_H
