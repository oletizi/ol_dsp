//
// Created by Orion Letizi on 2/2/23.
//
#include "loop.h"


// GUItool: begin automatically generated code
uint64_t counter = 0;

__attribute__((unused)) void doSetup() {
    Serial.begin(9600);
}

template<typename... A>
void println(const char *message, A... args) {
    char buffer[100];
    snprintf(buffer, 100, message, args...);
    Serial.print(buffer);
    Serial.print("\n\r");
}


static uint32_t checkpoint = 0;

__attribute__((unused)) void doLoop() {
    uint32_t now = millis();

    if (now - checkpoint >= 1000) {
        checkpoint = now;
        println("Hi! Now: %d", now);;
    }

}

float m2f(int note) {
    float a = 440; //frequency of A (coomon value is 440Hz)
    return (a / 32) * pow(2, ((note - 9) / 12.0));
}

float scale(float in, float inlow, float inhigh, float outlow, float outhigh, float power) {
    float value;
    float inscale = safediv(1., inhigh - inlow);
    float outdiff = outhigh - outlow;

    value = (in - inlow) * inscale;
    if (value > 0.0)
        value = pow(value, power);
    else if (value < 0.0)
        value = -pow(-value, power);
    value = (value * outdiff) + outlow;

    return value;
}


void clock_callback() {
    println("Clock! %d", millis());
}

void DU_Demo_Function::function_setup() {
    DUINO_Function::function_setup();
    Serial.begin(9600);

    clock_.set_bpm(120);
    clock_.attach_clock_callback(clock_callback);
    clock_.
    clock_.begin();

}

void DU_Demo_Function::function_loop() {
    counter++;
    DUINO_Function::function_loop();
    uint64_t now = millis();

    if (now - checkpoint >= 1000) {
        checkpoint = now;
        println("Hi! Now: %d", now);;
    }

    if (counter % CV_IN_UPDATE_FREQ == 0) {
        Display.fill_screen(DUINO_SH1106::Black);
        //Display.draw_circle(0, 0, 50, DUINO_SH1106::White);
        char *buffer[32];
        snprintf(buffer, 32, "Now: %d", now);
        Display.draw_text(0, 10, buffer, DUINO_SH1106::White);
        Display.display();
        counter = 0;
    }
}

