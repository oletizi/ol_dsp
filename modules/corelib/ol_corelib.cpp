//
// Created by Orion Letizi on 11/11/23.
//
#include <cmath>
#include "ol_corelib.h"

t_sample ol::core::scale(float in, float inlow, float inhigh, float outlow, float outhigh, float power) {
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

t_sample ol::core::Scale::Process(float in) const {
    return scale(in, in_min_, in_max_, out_min_, out_max_, pow_);
}
