//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_OL_CORELIB_H
#define OL_DSP_OL_CORELIB_H

#ifndef DSY_SDRAM_BSS
#define DSY_SDRAM_BSS
#endif

#include <cmath>
#include "corelib/cc_map.h"

typedef float t_sample;

namespace ol::core {

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

/**
 * Codifies a scale operation for a given input to output mapping
 */
    class Scale {
    public:
        Scale(t_sample in_min, t_sample in_max, t_sample out_min, t_sample out_max, t_sample power) :
                in_min_(in_min), in_max_(in_max), out_min_(out_min), out_max_(out_max), pow_(power) {}

        [[nodiscard]] t_sample Process(t_sample in) const;

    private:
        t_sample in_min_, in_max_, out_min_, out_max_, pow_;
    };


    class Rms {
    private:
        t_sample rms = 0;
        t_sample sum_of_squares = 0;
        t_sample sample_count = 0;
        t_sample window_ = 1;
    public:
        explicit Rms(){}

        //explicit Rms(int window = 1024) : window_(window) {}
        void Init(t_sample sample_rate, t_sample window = 0) {
            window_ = window != 0 ? window : sample_rate / 375;
        }

        t_sample Process(const t_sample &in) {
            if (sample_count == window_) {
                sum_of_squares = 0;
                sample_count = 0;
            }
            sum_of_squares += in * in;
            sample_count++;
            rms = sqrt(sum_of_squares / sample_count);
            return rms;
        }
    };

}

#endif //OL_DSP_OL_CORELIB_H
