//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXCONTROLPANEL_H
#define OL_DSP_FXCONTROLPANEL_H

#include "ctllib/Control.h"
#include "ReverbControlPanel.h"
#include "cc_map.h"
#include "DelayControlPanel.h"
#include "LpfControlPanel.h"

namespace ol::fx {
    class FxControlPanel {
    public:
        explicit FxControlPanel(ReverbControlPanel *reverb_control, DelayControlPanel *delay_control,
                                LpfControlPanel *lpf_control) :
                reverb_control_(reverb_control), delay_control_(delay_control), lpf_control_(lpf_control) {}

        void UpdateMidi(int controller_number, int controller_value) const;

        // Reverb
        ReverbControlPanel *reverb_control_;

        // Delay
        DelayControlPanel *delay_control_;

        // Filter
        LpfControlPanel *lpf_control_;
    };
}

#endif //OL_DSP_FXCONTROLPANEL_H