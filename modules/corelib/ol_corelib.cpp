//
// Created by Orion Letizi on 11/11/23.
//
#include <cmath>
#include "ol_corelib.h"



t_sample ol::core::Scale::Process(float in) const {
    return scale(in, in_min_, in_max_, out_min_, out_max_, pow_);
}
