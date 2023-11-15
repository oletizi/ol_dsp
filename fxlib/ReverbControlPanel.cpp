//
// Created by Orion Letizi on 11/15/23.
//

#include "ReverbControlPanel.h"

void ol::fx::ReverbControlPanel::UpdateMidi(uint16_t control_number, uint16_t control_value) {
    switch (control_number) {
        case CC_REVERB_TIME:
            reverb_time.UpdateValueMidi(control_value);
            break;
        case CC_REVERB_CUTOFF:
            reverb_cutoff.UpdateValueMidi(control_value);
            break;
        case CC_REVERB_BALANCE:
            reverb_balance.UpdateValueMidi(control_value);
            break;
        default: break;
    }
}
