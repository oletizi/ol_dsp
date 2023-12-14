//
// Created by Orion Letizi on 12/13/23.
//

#ifndef OL_DSP_FILTER_H
#define OL_DSP_FILTER_H

namespace ol::synth {
    class Filter {
    public:
        virtual void Init(t_sample sample_rate) = 0;

        virtual void SetFreq(t_sample freq) = 0;

        virtual void SetRes(t_sample res) = 0;

        virtual void SetDrive(t_sample drive) = 0;

        virtual void Process(t_sample in) = 0;

        virtual t_sample Low() = 0;

        virtual t_sample High() = 0;

        virtual t_sample Band() = 0;

        virtual t_sample Notch() = 0;

        virtual t_sample Peak() = 0;
    };

    class SvfFilter : public Filter {
    private:
        daisysp::Svf svf;
    public:
        void Init(t_sample sample_rate) override {
            svf.Init(sample_rate);
        }

        void SetFreq(t_sample freq) override {
            svf.SetFreq(freq);
        }

        void SetRes(t_sample res) override {
            svf.SetRes(res);
        }

        void SetDrive(t_sample drive) override {
            svf.SetDrive(drive);
        }

        void Process(t_sample in) override {
            svf.Process(in);
        }

        t_sample Low() override {
            return svf.Low();
        }

        t_sample High() override {
            return svf.High();
        }

        t_sample Band() override {
            return svf.Band();
        }

        t_sample Notch() override {
            return svf.Notch();
        }

        t_sample Peak() override {
            return svf.Peak();
        }
    };
}

#endif //OL_DSP_FILTER_H
