//
// Created by Orion Letizi on 11/19/23.
//
#include "LpfControlPanel.h"

namespace ol::fx {
    void LpfControlPanel::UpdateMidi(int controller_number, int value) {
        switch (controller_number) {
            case CC_LPF_CUTOFF:
                cutoff.UpdateValueMidi(value);
                break;
            case CC_LPF_RESONANCE:
                resonance.UpdateValueMidi(value);
                break;
            case CC_LPF_TYPE:
                type.UpdateValueMidi(value);
                break;
            default:
                break;
        }
    }
} // ol
// fxlib