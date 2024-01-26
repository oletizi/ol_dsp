//
// Created by Orion Letizi on 1/26/24.
//

#ifndef OL_DSP_SYNTHAPP_H
#define OL_DSP_SYNTHAPP_H

#include "SynthConfig.h"
#include "ControlListener.h"

namespace ol::app::synth {


    class SynthApp {

    public:
        explicit SynthApp(SynthConfig &config, ControlListener &control_listener) : config_(config), control_listener_(
                control_listener) {}

        void UpdateControl(Control c) {
            switch (c.GetController()) {
                case CC_FILTER_CUTOFF:
                    config_.filter_cutoff.Update(c);
                    control_listener_.UpdateFilterCutoff(c);
                    break;
                case CC_FILTER_RESONANCE:
                    config_.filter_resonance.Update(c);
                    control_listener_.UpdateFilterResonance(c);
                    break;
                case CC_FILTER_DRIVE:
                    config_.filter_drive.Update(c);
                    control_listener_.UpdateFilterDrive(c);
                    break;
                case CC_ENV_FILT_AMT:
                    config_.filter_env_amt.Update(c);
                    control_listener_.UpdateFilterEnvAmount(c);
                    break;
                case CC_ENV_FILT_A:
                    config_.filter_attack.Update(c);
                    control_listener_.UpdateFilterAttack(c);
                    break;
                case CC_ENV_FILT_D:
                    config_.filter_decay.Update(c);
                    control_listener_.UpdateFilterDecay(c);
                    break;
                case CC_ENV_FILT_S:
                    config_.filter_sustain.Update(c);
                    control_listener_.UpdateFilterSustain(c);
                    break;
                case CC_ENV_FILT_R:
                    config_.filter_release.Update(c);
                    control_listener_.UpdateFilterRelease(c);
                    break;
                case CC_CTL_VOLUME:
                    config_.amp_env_amt.Update(c);
                    control_listener_.UpdateAmpVolume(c);
                    break;
                case CC_ENV_AMP_A:
                    config_.amp_attack.Update(c);
                    control_listener_.UpdateAmpAttack(c);
                    break;
                case CC_ENV_AMP_D:
                    config_.amp_decay.Update(c);
                    control_listener_.UpdateAmpDecay(c);
                    break;
                case CC_ENV_AMP_S:
                    config_.amp_sustain.Update(c);
                    control_listener_.UpdateAmpSustain(c);
                    break;
                case CC_ENV_AMP_R:
                    config_.amp_release.Update(c);
                    control_listener_.UpdateAmpRelease(c);
                    break;
                default:
                    break;
            }
        }

    private:
//        void UpdateFilterCutoff(Control control) override {
//            config_.filter_cutoff.Update(control);
//            control_listener_.UpdateFilterCutoff(control);
//        }
//
//        void UpdateFilterResonance(Control control) override {
//            config_.filter_resonance.Update(control);
//            control_listener_.UpdateFilterResonance(control);
//        }
//
//        void UpdateFilterDrive(Control control) override {
//            config_.filter_drive.Update(control);
//            control_listener_.UpdateFilterDrive(control);
//        }
//
//        void UpdateFilterEnvAmount(Control control) override {
//            config_.filter_env_amt.Update(control);
//            control_listener_.UpdateFilterEnvAmount(control);
//        }
//
//        void UpdateFilterAttack(Control control) override {
//            config_.filter_attack.Update(control);
//            control_listener_.UpdateFilterAttack(control);
//        }
//
//        void UpdateFilterDecay(Control control) override {
//            config_.filter_decay.Update(control);
//            control_listener_.UpdateFilterDecay(control);
//        }
//
//        void UpdateFilterSustain(Control control) override {
//            config_.filter_sustain.Update(control);
//            control_listener_.UpdateFilterSustain(control);
//        }
//
//        void UpdateFilterRelease(Control control) override {
//            config_.filter_release.Update(control);
//            control_listener_.UpdateFilterRelease(control);
//        }
//
//        void UpdateAmpVolume(Control control) override {
//            config_.amp_env_amt.Update(control);
//            control_listener_.UpdateAmpVolume(control);
//        }
//
//        void UpdateAmpAttack(Control control) override {
//            config_.amp_attack.Update(control);
//            control_listener_.UpdateAmpAttack(control);
//        }
//
//        void UpdateAmpDecay(Control control) override {
//            config_.amp_decay.Update(control);
//            control_listener_.UpdateAmpDecay(control);
//        }
//
//        void UpdateAmpSustain(Control control) override {
//            config_.amp_sustain.Update(control);
//            control_listener_.UpdateAmpSustain(control);
//        }
//
//        void UpdateAmpRelease(Control control) override {
//            config_.amp_release.Update(control);
//            control_listener_.UpdateAmpRelease(control);
//        }

    private:
        SynthConfig &config_;
        ControlListener &control_listener_;
    };
}

#endif //OL_DSP_SYNTHAPP_H
