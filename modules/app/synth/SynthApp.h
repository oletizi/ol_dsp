//
// Created by Orion Letizi on 1/15/24.
//

#ifndef OL_DSP_SYNTHAPP_H
#define OL_DSP_SYNTHAPP_H

#include "ctllib/ol_ctllib.h"
#include "guilib/ol_guilib.h"

using namespace ol::ctl;
using namespace ol::gui;
namespace ol::app::synth {
    struct SynthAppConfig {
        Dimension viewport{128, 64};

        Control filter_cutoff{CC_FILTER_CUTOFF, 2500};
        Control filter_resonance{CC_FILTER_RESONANCE, 3800};
        Control filter_drive{CC_FILTER_DRIVE, 80};

        Control filter_env_amt{CC_ENV_FILT_AMT, 550};
        Control filter_attack{CC_ENV_FILT_A, 4000};
        Control filter_decay{CC_ENV_FILT_D, 3000};
        Control filter_sustain{CC_ENV_FILT_S, 2000};
        Control filter_release{CC_ENV_FILT_R, 2500};

        Control amp_env_amt{CC_CTL_VOLUME, 4096};
        Control amp_attack{CC_ENV_AMP_A, 0};
        Control amp_decay{CC_ENV_AMP_D, 0};
        Control amp_sustain{CC_ENV_AMP_S, 4096};
        Control amp_release{CC_ENV_AMP_R, 0};
    };

    class SynthApp {

    };
}

#endif //OL_DSP_SYNTHAPP_H
