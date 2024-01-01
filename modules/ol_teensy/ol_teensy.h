//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_OL_TEENSY_H
#define OL_DSP_OL_TEENSY_H
#include "Arduino.h"

#ifdef TEENSY_LOCAL
extern HardwareSerial Serial;
#endif // TEENSY_LOCAL
typedef float t_sample;

inline t_sample safediv(t_sample num, t_sample denom) {
    return denom == 0. ? (t_sample) 0. : (t_sample) (num / denom);
}
inline t_sample scale(float in, float inlow, float inhigh, float outlow, float outhigh, float power) {
    t_sample value;
    t_sample inscale = safediv(1., inhigh - inlow);
    t_sample outdiff = outhigh - outlow;

    value = (in - inlow) * inscale;
    if (value > 0.0)
        value = pow(value, power);
    else if (value < 0.0)
        value = -pow(-value, power);
    value = (value * outdiff) + outlow;

    return value;
}

#endif //OL_DSP_OL_TEENSY_H
