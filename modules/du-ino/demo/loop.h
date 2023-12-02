//
// Created by Orion Letizi on 2/2/23.
//

#ifndef ARDUINO_CLI_TEST_FOO_H
#define ARDUINO_CLI_TEST_FOO_H
#include <Arduino.h>
#include <du-ino_function.h>
#include <du-ino_clock.h>
#include <du-ino_sh1106.h>

#define GT_INT_DISPLAY_TIME   200
#define CV_IN_UPDATE_FREQ     100

class DU_Demo_Function : public DUINO_Function {
public:
    DU_Demo_Function() : DUINO_Function(0b00001111) {}
    void function_setup() override;
    void function_loop() override;
private:
    DUINO_Clock clock_;
};

float scale(float in, float inlow, float inhigh, float outlow, float outhigh, float power);
inline float safediv(float num, float denom) {
    return denom == 0. ? (float) 0. : (float) (num / denom);
}
float m2f(int note);

#endif //ARDUINO_CLI_TEST_FOO_H

