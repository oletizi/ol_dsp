//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_CONTROLTHINGYDEPRECATE_ME_H
#define OL_DSP_CONTROL_H

#include <cstdint>
#include "corelib/ol_corelib.h"
namespace ol::ctl {
    struct Control {
        int64_t controller;
        int64_t value;
        [[nodiscard]] t_sample scaledValue() const {
            return ol::core::scale(t_sample(value), 0, 4096, 0, 1, 1);
        }

        void setScaledValue(t_sample v) {
            value = int64_t(ol::core::scale(v, 0, 1, 0, 4096, 1));
        }
    };
}

#endif //OL_DSP_CONTROLTHINGYDEPRECATE_ME_H
