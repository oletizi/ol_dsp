//
// Created by Orion Letizi on 11/19/23.
//

#ifndef OL_DSP_LPFCONTROLPANEL_H
#define OL_DSP_LPFCONTROLPANEL_H

#include "ctllib/Control.h"
#include "cc_map.h"
namespace ol::fx {

    class LpfControlPanel {
    public:
        LpfControlPanel() {
            cutoff.UpdateValueHardware(0.5);
            resonance.UpdateValueHardware(0.2);
        }
        void UpdateMidi(int controller_number, int value);
        ctl::Control cutoff = ctl::Control(
                core::Scale(0, 1, 0, 20000, 1.1),
                core::Scale(0, 127, 0, 20000, 1.1));
        ctl::Control resonance  = ctl::Control();
        ctl::Control type = ctl::Control();
    };

} // ol
// fxlib

#endif //OL_DSP_LPFCONTROLPANEL_H
