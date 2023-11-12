//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_OL_CORELIB_H
#define OL_DSP_OL_CORELIB_H

#include <cmath>

#define t_sample float
namespace ol::core {
    // put something here someday.
    inline t_sample safediv(t_sample num, t_sample denom) {
        return denom == 0. ? (t_sample) 0. : (t_sample) (num / denom);
    }

    inline t_sample
    scale(t_sample in, t_sample inlow, t_sample inhigh, t_sample outlow, t_sample outhigh, t_sample power) {
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

/**
 * Codifies a scale operation for a given input to output mapping
 */
    class Scale {
    public:
        Scale(t_sample in_min, t_sample in_max, t_sample out_min, t_sample out_max, t_sample power) :
                in_min_(in_min), in_max_(in_max), out_min_(out_min), out_max_(out_max), pow_(power) {
        }

        [[nodiscard]] t_sample Process(t_sample in) const {
            return scale(in, in_min_, in_max_, out_min_, out_max_, pow_);
        }

    private:
        t_sample in_min_, in_max_, out_min_, out_max_, pow_;
    };
}
#endif //OL_DSP_OL_CORELIB_H
