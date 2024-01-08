//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_LOOP_H
#define OL_DSP_LOOP_H


#ifdef TEENSY_LOCAL

#include "WireKinetis.h
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

//int led = LED_BUILTIN;
int led = -1;
int counter = 0;
uint8_t buf[BUF_SIZE]{};
uint8_t inbuf[BUF_SIZE]{};
unsigned int bytes_read = 0;
ol_teensy::io::TeensySerial teensy_serial;
ol::io::SimpleSerializer serializer(teensy_serial);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

IntervalTimer display_timer;
uint64_t note_on_count = 0;
uint64_t note_off_count = 0;

Control filter_cutoff{CC_FILTER_CUTOFF, 0};
Control filter_resonance{CC_FILTER_RESONANCE, 0};
Control filter_env_amt{CC_ENV_FILT_AMT, 0};
Control filter_drive{CC_FILTER_DRIVE, 0};

Control filter_attack{CC_ENV_FILT_A, 0};
Control filter_decay{CC_ENV_FILT_D, 0};
Control filter_sustain{CC_ENV_FILT_S, 0};
Control filter_release{CC_ENV_FILT_R, 0};


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
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = serialized[i];
    }
    Serial1.write(data, sizeof(data));
    control_tx_count++;
}


int sample_control(uint8_t pin) {
    return 4096 - analogRead(pin);
}

void update_control(Control &c, uint16_t new_value) {
    auto delta = c.value - new_value;
    delta = delta < 0 ? delta * -1
                      : delta; // XXX: workaround for bug in teensy library that mangles abs() and std::abs()
    bool above_noise = delta > NOISE_FLOOR;
    if (above_noise) {
        c.value = new_value;
        write_control(c);
    }
}

void control_handler() {
    update_control(filter_cutoff, sample_control(A0));
    update_control(filter_resonance, sample_control(A1));
    update_control(filter_env_amt, sample_control(A2));
    update_control(filter_drive, sample_control(A3));

    update_control(filter_attack, sample_control(A4));
    update_control(filter_decay, sample_control(A5));
    update_control(filter_sustain, sample_control(A6));
    update_control(filter_release, sample_control(A7));
};

void handleNoteOn(byte channel, byte note, byte velocity) {
    digitalWrite(led, HIGH);
    Control pitch{CC_VOICE_PITCH, note};
    Control gate{CC_VOICE_GATE, velocity};
    gate.value = velocity;
    write_control(pitch);
    write_control(gate);
    note_on_count++;
    Serial.printf("Note ON: pitch: %d, velocity: %d\n", pitch.value, velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
    digitalWrite(led, LOW);
    Control pitch{CC_VOICE_PITCH, note};
    Control gate{CC_VOICE_GATE, velocity};
    gate.value = velocity;
    write_control(pitch);
    write_control(gate);
    note_off_count++;
    Serial.printf("Note OFF: pitch: %d, velocity: %d\n", pitch.value, velocity);
}

void midi_handler() {
    MIDI.read();
}

void d_cursor(int line_number, int column) {
    display.setCursor(column * COLUMN_WIDTH, line_number * LINE_HEIGHT);
}

void d_meter(int line_number, int column, const String &label, int value) {
    int x = column * COLUMN_WIDTH + (column > 0 ? 2 : 0);
    int y = line_number * LINE_HEIGHT;
    int width = int(ol::core::scale(float(value), 0, 4096, 0, 32, 1));
    int height = METER_HEIGHT;
    display.fillRect(x, y, width, height, SSD1306_WHITE);
    display.drawRect(x, y, METER_WIDTH, height, SSD1306_WHITE);
    //display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK);

    d_cursor(line_number, column);
    display.print(label);

    //display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
}

void display_handler() {
    Serial.printf("Display loop: counter: %d\n", counter);
    Serial.printf("A0: %d\n", analogRead(A0));
    Serial.printf("A1: %d\n", analogRead(A1));
    Serial.printf("A2: %d\n", analogRead(A2));
    Serial.printf("A3: %d\n", analogRead(A3));
    Serial.printf("A4: %d\n", analogRead(A4));
    Serial.printf("A5: %d\n", analogRead(A5));
    Serial.printf("A6: %d\n", analogRead(A6));
    Serial.printf("A7: %d\n", analogRead(A7));
    Serial.printf("A8: %d\n", analogRead(A8));
    Serial.printf("A9: %d\n", analogRead(A9));

    int line_number = 0;
    int column_number = 0;
    display.clearDisplay();

    d_meter(line_number++, column_number, "cut", filter_cutoff.value);

    d_meter(line_number++, column_number, "res", filter_resonance.value);

    d_meter(line_number++, column_number, "env", filter_env_amt.value);

    d_meter(line_number++, column_number, "drv", filter_drive.value);

    line_number = 0;
    column_number++;
    d_meter(line_number++, column_number, "att", filter_attack.value);
    d_meter(line_number++, column_number, "dec", filter_decay.value);
    d_meter(line_number++, column_number, "sus", filter_sustain.value);
    d_meter(line_number++, column_number, "rel", filter_release.value);


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


    //Serial1.begin(9600);
//    Serial1.begin(57600);
    Serial1.begin(115200);


    pinMode(led, HIGH);
    // put your setup code here, to run once:
    Serial.println("This may be sent before your PC is able to receive");
    while (!Serial) {
        // wait for Arduino Serial Monitor to be ready
    }
    Serial.println("This line will definitely appear in the serial monitor");
    Serial.println("Starting midi...");
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.begin(MIDI_CHANNEL_OMNI);


    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    Serial.println("Starting display1...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("display1 allocation failed"));
    } else {
        Serial.println("Display started.");
    }

    Serial.println("Drawing splash screen to display1...");
    display.clearDisplay();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.write("Hello!");
    display.display();
    Serial.println("Done drawing splash screen to display1.");

//    display_timer.priority(128);
//    display_timer.begin(display_handler, 1000 * 100);
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
