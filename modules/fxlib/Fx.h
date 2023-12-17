//
// Created by Orion Letizi on 11/27/23.
//

#ifndef OL_DSP_FX_H
#define OL_DSP_FX_H
#define SPFLOAT t_sample

#include "corelib/ol_corelib.h"

//extern "C" {
//#include "soundpipe.h"
//}

#include "verb.h"
#include "daisysp.h"
#include "corelib/cc_map.h"
#include "synthlib/Filter.h"
#include "Reverb.h"

#define MAX_DELAY 48000
namespace ol::fx {

    template<int CHANNEL_COUNT>
    class SaturatorFx {
    public:
        typedef t_sample (*TransferFunction)(SaturatorFx *data, const t_sample in);

        explicit SaturatorFx(TransferFunction fn) : transferFunction(fn) {}

        // Default implementation uses hyberbolic tangent
        explicit SaturatorFx() : SaturatorFx(hyperbolic_tangent) {}

        void Init(const t_sample sample_rate) { /* nop */ }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                t_sample out = transferFunction(this, frame_in[CHANNEL_COUNT]);
                frame_out[CHANNEL_COUNT] = out;
            }
        }

        void Update() { /* nop */ }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            if (control == CC_SATURATOR_DRIVE) {
                drive = ol::core::scale(value, 0, 127, 1, 128, 1.5);
                Update();
            }
        }

    private:
        t_sample drive = 1;
        TransferFunction transferFunction = nullptr;

        // default saturator transfer function
        static inline t_sample hyperbolic_tangent(SaturatorFx *fx, const t_sample in) {
            return tanh(fx->drive * in);
        }
    };

    template<int CHANNEL_COUNT>
    class FilterFx {
    private:
        enum FilterType {
            LowPass,
            BandPass,
            HighPass,
            Notch,
            Peak
        };

        t_sample cutoff = 20000;
        t_sample resonance = 0;
        t_sample drive = 0;
        FilterType type = LowPass;
        ol::synth::Filter *filter_;

    public:

        FilterFx() : filter_(new ol::synth::SvfFilter<CHANNEL_COUNT>()) {}

        explicit FilterFx(ol::synth::Filter *filter) : filter_(filter) {}

        void Init(t_sample sample_rate) {
            filter_->Init(sample_rate);
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            filter_->Process(frame_in);
            switch (type) {
                case HighPass:
                    filter_->High(frame_out);
                    break;
                case BandPass:
                    filter_->Band(frame_out);
                    break;
                case Peak:
                    filter_->Peak(frame_out);
                    break;
                case Notch:
                    filter_->Notch(frame_out);
                    break;
                case LowPass:
                default:
                    filter_->Low(frame_out);
                    break;
            }
        };

        void Update() {
            filter_->SetFreq(cutoff);
            filter_->SetRes(resonance);
            filter_->SetDrive(drive);
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            bool update = true;
            t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
            switch (control) {
                case CC_FILTER_CUTOFF:
                    cutoff = ol::core::scale(value, 0, 127, 0, 20000, 1);
                    break;
                case CC_FILTER_RESONANCE:
                    resonance = scaled;
                    break;
                case CC_FILTER_DRIVE:
                    drive = scaled;
                    break;
                case CC_FILTER_TYPE:
                    type = FilterType(ol::core::scale(value, 0, 127, 0, 5, 1));
                    break;
                default:
                    update = false;
                    break;
            }
            if (update) {
                Update();
            }
        }
    };

    // Delay

    template<int CHANNEL_COUNT>
    class DelayFx {
    private:
        t_sample time = 0.5;
        t_sample feedback = 0.5;
        t_sample balance = 0.33;
        t_sample dbuf = 0;
        t_sample ibuf[CHANNEL_COUNT]{};
        t_sample obuf[CHANNEL_COUNT]{};
        daisysp::DelayLine<t_sample, MAX_DELAY> *delay_lines_[CHANNEL_COUNT]{};
        FilterFx<CHANNEL_COUNT> filter;
    public:
        DelayFx() {
            for (auto &d: delay_lines_) {
                d = new daisysp::DelayLine<t_sample, MAX_DELAY>();
            }
        }

        explicit DelayFx(daisysp::DelayLine<t_sample, MAX_DELAY> *delay_lines[CHANNEL_COUNT]) : delay_lines_(
                delay_lines) {}

        DelayFx(daisysp::DelayLine<t_sample, MAX_DELAY> *delay_lines[CHANNEL_COUNT], FilterFx<CHANNEL_COUNT> &filter)
                : delay_lines_(delay_lines), filter(filter) {}

        void Init(const t_sample sample_rate) {
            for (auto &d: delay_lines_) {
                d->Init();
            }
            filter.Init(sample_rate);
            Update();
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                ibuf[i] = obuf[i] = dbuf = delay_lines_[i]->Read();
                delay_lines_[i]->Write(frame_in[i] + (feedback * dbuf));
            }

            filter.Process(ibuf, obuf);

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = (obuf[i] * balance) + (frame_in[i] * (1 - balance));
            }
        }

        void Update() {
            // TODO: try using a read index for multi-tap support.

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                delay_lines_[i]->SetDelay(ol::core::scale(time, 0, 1, 0, MAX_DELAY, 1));
            }

            filter.Update();
        }

        void UpdateHardwareControl(uint8_t control, t_sample value) {
            bool update = true;
            switch (control) {
                case CC_DELAY_TIME:
                    time = value;
                    break;
                case CC_DELAY_FEEDBACK:
                    feedback = value;
                    break;
                case CC_DELAY_BALANCE:
                    balance = value;
                    break;
                default:
                    update = false;
            }
            //filter.UpdateHardwareControl(control, value);
            if (update) {
                Update();
            }
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            bool update = true;
            t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
            printf("Delay midi control: %d, %d\n", control, value);
            switch (control) {
                case CC_DELAY_TIME:
                    time = scaled;
                    printf("Delay time: %d\n", int(time) * 1000);
                    break;
                case CC_DELAY_FEEDBACK:
                    feedback = scaled;
                    break;
                case CC_DELAY_BALANCE:
                    balance = scaled;
                    break;
                case CC_DELAY_CUTOFF:
                    filter.UpdateMidiControl(CC_FILTER_CUTOFF, value);
                    break;
                case CC_DELAY_RESONANCE:
                    filter.UpdateMidiControl(CC_FILTER_RESONANCE, value);
                    break;
                default:
                    update = false;
            }
            if (update) {
                Update();
            }
        }

    };

    // Reverb
    template<int CHANNEL_COUNT>
    class ReverbFx {
    private:
        t_sample decay_time = 0.5;
        t_sample cutoff = 12000;
        t_sample early_predelay = 0.1;
        t_sample predelay = 0.2;
        t_sample pre_cutoff = 0.5;
        t_sample input_diffusion1 = 0.5;
        t_sample input_diffusion2 = 0.5;
        t_sample decay_diffusion = 0.5;
        t_sample balance = 0.5;
        Reverb *reverb;
        t_sample buf[CHANNEL_COUNT]{};

    public:
        ReverbFx() : reverb(new DaisyVerb<CHANNEL_COUNT>) {}

        explicit ReverbFx(Reverb *verb) : reverb(verb) {}

        void Init(const t_sample sample_rate) {
            reverb->Init(sample_rate);
            //Update();
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            reverb->Process(frame_in, buf);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = (buf[i] * balance) + (frame_in[i] * (1 - balance));
            }
        }

        void Update() {
            reverb->SetTime(decay_time);
            reverb->SetCutoff(cutoff);
            reverb->SetEarlyPredelay(early_predelay);
            reverb->SetPredelay(predelay);
            reverb->SetPrefilter(pre_cutoff);
            reverb->SetInputDiffusion1(input_diffusion1);
            reverb->SetInputDiffusion2(input_diffusion2);
            reverb->SetDecayDiffusion(decay_diffusion);
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            bool do_update = true;
            t_sample scaled = core::scale(value, 0, 127, 0, 1, 1);
            switch (control) {
                case CC_REVERB_DECAY_DIFFUSION:
                    decay_diffusion = scaled;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_1:
                    input_diffusion1 = scaled;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_2:
                    decay_diffusion = scaled;
                    break;
                case CC_REVERB_CUTOFF:
                    cutoff = core::scale(value, 0, 127, 0, 20000, 1);
                    //cutoff = scaled;
                    break;
                case CC_REVERB_BALANCE:
                    balance = scaled;
                    break;
                case CC_REVERB_PREDELAY:
                    predelay = scaled;
                    break;
                case CC_EARLY_PREDELAY:
                    early_predelay = scaled;
                    break;
                case CC_REVERB_PREFILTER:
                    pre_cutoff = scaled;
                    break;
                case CC_REVERB_TIME:
                    decay_time = scaled;
                    break;
                default:
                    do_update = false;
                    break;
            }
            if (do_update) {
                Update();
            }
        }

        void UpdateHardwareControl(uint8_t control, t_sample value) {
            bool do_update = true;
            switch (control) {
                case CC_REVERB_DECAY_DIFFUSION:;
                    decay_diffusion = value;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_1:;
                    input_diffusion1 = value;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_2:;
                    decay_diffusion = value;
                    break;
                case CC_REVERB_CUTOFF:;
                    cutoff = ol::core::scale(value, 0, 1, 0, 20000, 1);
                    break;
                case CC_REVERB_BALANCE:;
                    balance = value;
                    break;
                case CC_REVERB_PREDELAY:;
                    predelay = value;
                    break;
                case CC_EARLY_PREDELAY:;
                    early_predelay = value;
                    break;
                case CC_REVERB_PREFILTER:;
                    pre_cutoff = value;
                    break;
                case CC_REVERB_TIME:;
                    decay_time = value;
                    break;
                default:
                    do_update = false;
                    break;
            }
            if (do_update) {
                Update();
            }
        }

    };

    // Multi-effects

    template<int CHANNEL_COUNT>
    class FxRack {
    private:
        ol::fx::DelayFx<CHANNEL_COUNT> delay1;
        ol::fx::ReverbFx<CHANNEL_COUNT> reverb;
        FilterFx<CHANNEL_COUNT> filter1;
        SaturatorFx<CHANNEL_COUNT> saturator1;
        SaturatorFx<CHANNEL_COUNT> interstage_saturator;
        t_sample master_volume = 0.8f;
        t_sample abuf[CHANNEL_COUNT]{};
        t_sample bbuf[CHANNEL_COUNT]{};
        t_sample obuf[CHANNEL_COUNT]{};

    public:
        FxRack() = default;

        FxRack(DelayFx<CHANNEL_COUNT> &delay1, ReverbFx<CHANNEL_COUNT> &reverb, FilterFx<CHANNEL_COUNT> &filt1,
               SaturatorFx<CHANNEL_COUNT> &sat1,
               SaturatorFx<CHANNEL_COUNT> &interstage_saturator)
                : delay1(delay1),
                  reverb(reverb),
                  filter1(filt1),
                  saturator1(sat1),
                  interstage_saturator(interstage_saturator) {}

        void Init(t_sample sample_rate) {
            delay1.Init(sample_rate);
            reverb.Init(sample_rate);
            filter1.Init(sample_rate);
            saturator1.Init(sample_rate);
            interstage_saturator.Init(sample_rate);
            Update();
        };

        void Process(const t_sample *frame_in, t_sample *frame_out) {

            delay1.Process(frame_in, abuf);
            reverb.Process(abuf, abuf);
//            saturator1.Process(abuf, obuf);
//
            filter1.Process(abuf, obuf);

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = obuf[i] * master_volume;
            }
        };

        void Update() {
            delay1.Update();
            reverb.Update();
            filter1.Update();
            saturator1.Update();
        }

        void UpdateMidiControl(const uint8_t control, const uint8_t value) {
            switch (control) {
                case CC_FX_FILTER_CUTOFF:
                    filter1.UpdateMidiControl(CC_FILTER_CUTOFF, value);
                    break;
                case CC_FX_FILTER_RESONANCE:
                    filter1.UpdateMidiControl(CC_FILTER_RESONANCE, value);
                    break;
                case CC_FX_FILTER_DRIVE:
                    filter1.UpdateMidiControl(CC_FILTER_DRIVE, value);
                    break;
                case CC_FX_FILTER_TYPE:
                    filter1.UpdateMidiControl(CC_FILTER_TYPE, value);
                    break;
                default:
                    break;
            }

            delay1.UpdateMidiControl(control, value);
            reverb.UpdateMidiControl(control, value);
            saturator1.UpdateMidiControl(control, value);
            if (control == CC_CTL_VOLUME) {
                master_volume = ol::core::scale(value, 0, 127, 0, 1, 1);
            }
        }

    };


}
#endif //OL_DSP_FX_H
