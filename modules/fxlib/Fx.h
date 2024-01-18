//
// Created by Orion Letizi on 11/27/23.
//

#ifndef OL_DSP_FX_H
#define OL_DSP_FX_H
#define SPFLOAT t_sample

#include <utility>

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

        void Init([[maybe_unused]] const t_sample sample_rate) { /* nop */ }

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
        ol::synth::SvfFilter filter_;

    public:

        void Init(t_sample sample_rate) {
            filter_.Init(sample_rate);
            Update();
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            filter_.Process(frame_in);
            switch (type) {
                case HighPass:
                    filter_.High(frame_out);
                    break;
                case BandPass:
                    filter_.Band(frame_out);
                    break;
                case Peak:
                    filter_.Peak(frame_out);
                    break;
                case Notch:
                    filter_.Notch(frame_out);
                    break;
                case LowPass:
                default:
                    filter_.Low(frame_out);
                    break;
            }
        };

        void Update() {
            filter_.SetFreq(cutoff);
            filter_.SetRes(resonance);
            filter_.SetDrive(drive);
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

        void UpdateHardwareControl(uint8_t control, t_sample value) {
            bool update = true;
            switch (control) {
                case CC_FILTER_CUTOFF:
                    cutoff = ol::core::scale(value, 0, 1, 0, 20000, 1.02);
                    break;
                case CC_FILTER_RESONANCE:
                    resonance = value;
                    break;
                case CC_FILTER_DRIVE:
                    drive = value;
                    break;
                case CC_FILTER_TYPE:
                    type = FilterType(ol::core::scale(value, 0, 1, 0, 5, 1));
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
        //daisysp::DelayLine<t_sample, MAX_DELAY> *d1_;
        std::vector<daisysp::DelayLine<t_sample, MAX_DELAY> *> delay_lines_;
        //daisysp::Svf filter_;
        FilterFx<CHANNEL_COUNT> filter_;
        //daisysp::MoogLadder *filter_;
    public:
        DelayFx(std::vector<daisysp::DelayLine<t_sample, MAX_DELAY> *> &delay_lines) : delay_lines_(delay_lines) {}

        void Init(const t_sample sample_rate) {
            //d1_->Init();
            for (auto &d: delay_lines_) {
                d->Init();
            }
            filter_.Init(sample_rate);
            filter_.UpdateMidiControl(CC_FILTER_CUTOFF, 64);
            filter_.UpdateMidiControl(CC_FILTER_RESONANCE, 24);
            Update();
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            t_sample buf[CHANNEL_COUNT];
            //d1_->Write(*frame_in + (feedback * buf));
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                auto d = delay_lines_.at(i);
                buf[i] = d->Read();
                d->Write(frame_in[i] + (feedback * buf[i]));
            }
            //buf = filter_->Process(buf);
            filter_.Process(buf, buf);
            //buf = filter_.Low();
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = (buf[i] * balance) + (frame_in[i] * (1 - balance));
            }
        }

        void Update() {
            // TODO: try using a read index for multi-tap support.
            //delay_.SetDelay(ol::core::scale(time, 0, 1, 0, MAX_DELAY, 1));
            for (auto &d: delay_lines_) {
                d->SetDelay(ol::core::scale(time, 0, 1, 0, MAX_DELAY, 1));
            }
            filter_.Update();
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
            if (update) {
                Update();
            }
        }

        void UpdateMidiControl(uint8_t control, uint8_t value) {
            bool update = true;
            t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
            switch (control) {
                case CC_DELAY_TIME:
                    time = scaled;
                    break;
                case CC_DELAY_FEEDBACK:
                    feedback = scaled;
                    break;
                case CC_DELAY_BALANCE:
                    balance = scaled;
                    break;
                case CC_DELAY_CUTOFF:
                    filter_.UpdateMidiControl(CC_FILTER_CUTOFF, value);
//                    filter_.SetFreq(ol::core::scale(value, 0, 127, 0, 20000, 1));
                    break;
                case CC_DELAY_RESONANCE:
                    filter_.UpdateMidiControl(CC_FILTER_RESONANCE, value);
//                    filter_.SetRes(scaled);
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
        t_sample balance = 0.1;
        DaisyVerb<CHANNEL_COUNT> verb_;

    public:

        ReverbFx(DaisyVerb<CHANNEL_COUNT> &verb) : verb_(verb) {};

        void Init(const t_sample sample_rate) {
            verb_.Init(sample_rate);
            Update();
        }

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            t_sample buf[CHANNEL_COUNT]{};
            verb_.Process(frame_in, buf);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = (buf[i] * balance) + (frame_in[i] * (1 - balance));
            }
        }

        void Update() {
            verb_.SetTime(decay_time);
            verb_.SetCutoff(cutoff);
            verb_.SetEarlyPredelay(early_predelay);
            verb_.SetPredelay(predelay);
            verb_.SetPrefilter(pre_cutoff);
            verb_.SetInputDiffusion1(input_diffusion1);
            verb_.SetInputDiffusion2(input_diffusion2);
            verb_.SetDecayDiffusion(decay_diffusion);
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
                case CC_REVERB_DECAY_DIFFUSION:
                    decay_diffusion = value;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_1:
                    input_diffusion1 = value;
                    break;
                case CC_REVERB_INPUT_DIFFUSION_2:
                    decay_diffusion = value;
                    break;
                case CC_REVERB_CUTOFF:
                    cutoff = ol::core::scale(value, 0, 1, 0, 20000, 1);
                    break;
                case CC_REVERB_BALANCE:
                    balance = value;
                    break;
                case CC_REVERB_PREDELAY:
                    predelay = value;
                    break;
                case CC_EARLY_PREDELAY:
                    early_predelay = value;
                    break;
                case CC_REVERB_PREFILTER:
                    pre_cutoff = value;
                    break;
                case CC_REVERB_TIME:
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
        DelayFx<CHANNEL_COUNT> &delay_;
        ol::fx::ReverbFx<CHANNEL_COUNT> &reverb_;
        //FilterFx<CHANNEL_COUNT> filter1;
        FilterFx<CHANNEL_COUNT> &filter1;
        SaturatorFx<CHANNEL_COUNT> saturator1;
        SaturatorFx<CHANNEL_COUNT> interstage_saturator;
        t_sample master_volume = 0.8f;
        t_sample buf_a[CHANNEL_COUNT]{};
        t_sample buf_b[CHANNEL_COUNT]{};
        t_sample buf_c[CHANNEL_COUNT]{};

    public:

        FxRack(DelayFx<CHANNEL_COUNT> &delay, ReverbFx<CHANNEL_COUNT> &reverb, FilterFx<CHANNEL_COUNT> &filter)
                : delay_(delay), reverb_(reverb), filter1(filter) {}

        void Init(t_sample sample_rate) {
            delay_.Init(sample_rate);

            reverb_.Init(sample_rate);
            filter1.Init(sample_rate);
            saturator1.Init(sample_rate);
            interstage_saturator.Init(sample_rate);
            Update();
        };

        void Process(const t_sample *frame_in, t_sample *frame_out) {
            delay_.Process(frame_in, buf_a);
            reverb_.Process(buf_a, buf_b);
            filter1.Process(buf_b, buf_c);

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = buf_c[i] * master_volume;
            }
        };

        void Update() {
            delay_.Update();
            reverb_.Update();
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

            delay_.UpdateMidiControl(control, value);
            reverb_.UpdateMidiControl(control, value);
            saturator1.UpdateMidiControl(control, value);
            if (control == CC_CTL_VOLUME) {
                master_volume = ol::core::scale(value, 0, 127, 0, 1, 1);
            }
        }

        void UpdateHardwareControl(uint8_t control, t_sample value) {
            switch (control) {
                case CC_FX_FILTER_CUTOFF:
                    filter1.UpdateHardwareControl(CC_FILTER_CUTOFF, value);
                    break;
                case CC_FX_FILTER_RESONANCE:
                    filter1.UpdateHardwareControl(CC_FILTER_RESONANCE, value);
                    break;
                case CC_FX_FILTER_DRIVE:
                    filter1.UpdateHardwareControl(CC_FILTER_DRIVE, value);
                    break;
                case CC_FX_FILTER_TYPE:
                    filter1.UpdateHardwareControl(CC_FILTER_TYPE, value);
                    break;
                case CC_CTL_VOLUME:
                    master_volume = value;
                    break;
                default:
                    break;
            }
            delay_.UpdateHardwareControl(control, value);
            reverb_.UpdateHardwareControl(control, value);
        }
    };


}
#endif //OL_DSP_FX_H
