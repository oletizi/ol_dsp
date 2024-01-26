//
// Created by Orion Letizi on 1/26/24.
//

#ifndef OL_DSP_CONTROLLISTENER_H
#define OL_DSP_CONTROLLISTENER_H
namespace ol::app::synth {
    using namespace ol::ctl;
    class ControlListener {
    public:
        virtual void UpdateFilterCutoff(Control control) = 0;

        virtual void UpdateFilterResonance(Control control) = 0;

        virtual void UpdateFilterDrive(Control control) = 0;

        virtual void UpdateFilterEnvAmount(Control control) = 0;

        virtual void UpdateFilterAttack(Control control) = 0;

        virtual void UpdateFilterDecay(Control control) = 0;

        virtual void UpdateFilterSustain(Control control) = 0;

        virtual void UpdateFilterRelease(Control control) = 0;

        virtual void UpdateAmpVolume(Control control) = 0;

        virtual void UpdateAmpAttack(Control control) = 0;

        virtual void UpdateAmpDecay(Control control) = 0;

        virtual void UpdateAmpSustain(Control control) = 0;

        virtual void UpdateAmpRelease(Control control) = 0;
    };
}


#endif //OL_DSP_CONTROLLISTENER_H
