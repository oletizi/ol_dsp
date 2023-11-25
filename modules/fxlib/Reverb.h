//
// Created by Orion Letizi on 11/15/23.
//

#ifndef OL_DSP_REVERB_H
#define OL_DSP_REVERB_H
#include "daisysp.h"
#include "corelib/ol_corelib.h"
#include "ReverbControlPanel.h"
#include "Fx.h"

namespace ol::fx {

    class Reverb {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual int Process(const float &in1, const float &in2, float *out1, float *out2) = 0;

        virtual void SetReverbTime(t_sample t60) = 0;

        virtual void SetCutoff(t_sample cutoff) = 0;

        virtual void SetEarlyPredelay(t_sample t60) = 0;

        virtual void SetPredelay(t_sample t60) = 0;

        virtual void SetPreCutoff(t_sample freq) = 0;

        virtual void SetInputDiffusion1(t_sample val) = 0;

        virtual void SetInputDiffusion2(t_sample val) = 0;

        virtual void SetDecayDiffusion(t_sample vale) = 0;
    };

    class ReverbScWrapper : public Reverb {
    public:

        explicit ReverbScWrapper(daisysp::ReverbSc *reverb) : reverb_(reverb) {}

        void Init(t_sample sample_rate) override;

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) override;

        void SetReverbTime(t_sample t60) override;

        void SetCutoff(t_sample cutoff) override;

        void SetEarlyPredelay(t_sample t60) override;

        void SetPredelay(t_sample t60) override;

        void SetPreCutoff(t_sample freq) override;

        void SetInputDiffusion1(t_sample val) override;

        void SetInputDiffusion2(t_sample val) override;

        void SetDecayDiffusion(t_sample vale) override;

    private:
        daisysp::ReverbSc *reverb_;

    };

    class ReverbFx : public Fx {
    public:
        explicit ReverbFx(ol::fx::ReverbControlPanel *control_panel, Reverb *reverb) :
                control_panel_(control_panel),
                reverb_(reverb) {};

        void Init(t_sample sample_rate) override;

        int Process(const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) override;

    private:
        Reverb *reverb_;
        ReverbControlPanel *control_panel_;
    };
}

#endif //OL_DSP_REVERB_H
