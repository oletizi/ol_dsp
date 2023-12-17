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

        virtual void Process(const t_sample *frame_in) = 0;

        virtual void Low(t_sample *frame_out) = 0;

        virtual void High(t_sample *frame_out) = 0;

        virtual void Band(t_sample *frame_out) = 0;

        virtual void Notch(t_sample *frame_out) = 0;

        virtual void Peak(t_sample *frame_out) = 0;
    };

    template<int CHANNEL_COUNT>
    class SvfFilter : public Filter {
    private:
        daisysp::Svf filters_[CHANNEL_COUNT];
    public:
        void Init(t_sample sample_rate) override {
            //svf.Init(sample_rate);
            for (auto &f : filters_) {
                f.Init(sample_rate);
            }
        }

        void SetFreq(t_sample freq) override {
            //svf.SetFreq(freq);
            for (auto &f : filters_) {
                f.SetFreq(freq);
            }
        }

        void SetRes(t_sample res) override {
            //svf.SetRes(res);
            for (auto &f : filters_) {
                f.SetRes(res);
            }
        }

        void SetDrive(t_sample drive) override {
            //svf.SetDrive(drive);
            for (auto &f: filters_){
                f.SetDrive(drive);
            }
        }

        void Process(const t_sample *frame_in) override {
            //svf.Process(in);
            for (int i=0; i<CHANNEL_COUNT; i++) {
                filters_[i].Process(frame_in[i]);
            }
        }

        void Low(t_sample *frame_out) override {
            //return svf.Low();
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame_out[i] = filters_[i].Low();
            }
        }

        void High(t_sample *frame_out) override {
            //return svf.High();
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame_out[i] = filters_[i].High();
            }
        }

        void Band(t_sample *frame_out) override {
            //return svf.Band();
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame_out[i] = filters_[i].Band();
            }
        }

        void Notch(t_sample *frame_out) override {
            //return svf.Notch();
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame_out[i] = filters_[i].Notch();
            }
        }

        void Peak(t_sample *frame_out) override {
            for (int i=0; i<CHANNEL_COUNT; i++) {
                frame_out[i] = filters_[i].Peak();
            }
        }
    };
}

#endif //OL_DSP_FILTER_H
