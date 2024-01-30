//
// Created by Orion Letizi on 1/25/24.
//

#ifndef OL_DSP_SYNTHCONFIG_H
#define OL_DSP_SYNTHCONFIG_H

#include "guilib/ol_guilib_core.h"

namespace ol::app::synth {
    using namespace ol::ctl;

    struct SynthConfig {

        ol::gui::Dimension viewport{128, 64};

        Control osc_1_waveform = Control(CC_OSC_1_WAVEFORM, 0.f);
        Control osc_1_level = Control(CC_OSC_1_VOLUME, .8f);

        Control filter_cutoff = Control(CC_FILTER_CUTOFF, 0.5f);
        Control filter_resonance = Control(CC_FILTER_RESONANCE, 0.3f);
        Control filter_drive = Control(CC_FILTER_DRIVE, 0.1f);

        Control filter_env_amt = Control(CC_ENV_FILT_AMT, 0.25f);
        Control filter_attack = Control(CC_ENV_FILT_A, 0.f);
        Control filter_decay = Control(CC_ENV_FILT_D, 0.8f);
        Control filter_sustain = Control(CC_ENV_FILT_S, 0.f);
        Control filter_release = Control(CC_ENV_FILT_R, 0.2f);

        Control amp_env_amt = Control(CC_CTL_VOLUME, 1.f);
        Control amp_attack = Control(CC_ENV_AMP_A, 0.f);
        Control amp_decay = Control(CC_ENV_AMP_D, 0.f);
        Control amp_sustain = Control(CC_ENV_AMP_S, 1.f);
        Control amp_release = Control(CC_ENV_AMP_R, 0.f);
    };
}
#endif //OL_DSP_SYNTHCONFIG_H
