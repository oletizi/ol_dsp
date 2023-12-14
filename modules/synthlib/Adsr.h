//
// Created by Orion Letizi on 12/13/23.
//

#ifndef OL_DSP_ADSR_H
#define OL_DSP_ADSR_H

#include "corelib/ol_corelib.h"
#include "Control/adsr.h"

namespace ol::synth {
    class Adsr {
    public:
        virtual void Init(t_sample sample_rate, int blockSize) = 0;

        virtual void Retrigger(bool hard) = 0;

        virtual t_sample Process(bool gate) = 0;

        virtual void SetTime(int seg, t_sample time) = 0;

        virtual void SetAttackTime(t_sample timeInS, t_sample shape) = 0;

        virtual void SetDecayTime(t_sample timeInS) = 0;

        virtual void SetSustainLevel(t_sample level) = 0;

        virtual void SetReleaseTime(t_sample timeInS) = 0;

        virtual uint8_t GetCurrentSegment() = 0;

        virtual bool IsRunning() = 0;
    };

    class DaisyAdsr : public Adsr {
    private:
        daisysp::Adsr adsr_;

    public:
        void Init(t_sample sample_rate, int blockSize) override {
            adsr_.Init(sample_rate, blockSize);
        }

        void Retrigger(bool hard) override {
            adsr_.Retrigger(hard);
        }

        t_sample Process(bool gate) override {
            return adsr_.Process(gate);
        }

        void SetTime(int seg, t_sample time) override {
            adsr_.SetTime(seg, time);
        }

        void SetAttackTime(t_sample timeInS, t_sample shape) override {
            adsr_.SetAttackTime(timeInS, shape);
        }

        void SetDecayTime(t_sample timeInS) override {
            adsr_.SetDecayTime(timeInS);
        }

        void SetSustainLevel(t_sample level) override {
            adsr_.SetSustainLevel(level);
        }

        void SetReleaseTime(t_sample timeInS) override {
            adsr_.SetReleaseTime(timeInS);
        }

        uint8_t GetCurrentSegment() override { return adsr_.GetCurrentSegment(); }

        bool IsRunning() override { return adsr_.IsRunning(); }

    };
}
#endif //OL_DSP_ADSR_H
