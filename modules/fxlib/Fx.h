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
                frame_out[CHANNEL_COUNT] = transferFunction(this, frame_in[CHANNEL_COUNT]);
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

        t_sample cutoff = 1;
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
            //filter_->Low(frame_out);
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
        t_sample frame_buffer[CHANNEL_COUNT]{};
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
                frame_buffer[i] = frame_in[i] * (feedback * delay_lines_[i]->Read());
                delay_lines_[i]->Write(frame_buffer[i]);
            }

            t_sample filter_out = 0;
            filter.Process(frame_buffer, reinterpret_cast<t_sample *>(&frame_buffer));

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = (frame_buffer[i] * balance) + (frame_in[i] * (1 - balance));
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
    class ReverbFx {
    private:
        const int input_channels = 0;
        const int output_channels = 0;
        t_sample sample_rate = 44100;
        t_sample decay_time = 0.5;
        t_sample cutoff = 1;
        t_sample early_predelay = 0.1;
        t_sample predelay = 0.2;
        t_sample pre_cutoff = 0.5;
        t_sample input_diffusion1 = 0.5;
        t_sample input_diffusion2 = 0.5;
        t_sample decay_diffusion = 0.5;
        t_sample balance = 0.5;
        void *reverb = nullptr;

        typedef void (*init_function)(ReverbFx *, t_sample sample_rate);

        typedef void (*process_function)(ReverbFx *, const t_sample *frame_in, t_sample *frame_out);

        typedef void (*update_function)(ReverbFx *reverb);

        static void dattorro_init(ReverbFx *, t_sample sample_rate);

        static void dattorro_process(ReverbFx *, const t_sample *frame_in, t_sample *frame_out);

        static void dattorro_update(ReverbFx *);

        static void reverbSc_init(ReverbFx *, t_sample sample_rate);

        static void reverbSc_process(ReverbFx *, const t_sample *frame_in, t_sample *frame_out);

        static void reverbSc_update(ReverbFx *);

        init_function init = nullptr;
        process_function process = nullptr;
        update_function update = nullptr;


        inline daisysp::ReverbSc *asReverbSc() { return static_cast<daisysp::ReverbSc *>(reverb); }

        inline sDattorroVerb *asDattorroVerb() { return static_cast<sDattorroVerb *>(reverb); }

    public:
        // Daisy ReverbSc ctor
        ReverbFx() : reverb(new daisysp::ReverbSc()), input_channels(2), output_channels(2) {}

        explicit ReverbFx(daisysp::ReverbSc &reverb,
                          int input_channels = 2,
                          int output_channels = 2) :
                reverb(static_cast<void *>(&reverb)),
                input_channels(input_channels),
                output_channels(output_channels),
                init(reverbSc_init),
                process(reverbSc_process),
                update(reverbSc_update) {}

        // Dattorro reverb ctor
        explicit ReverbFx(sDattorroVerb *reverb, int input_channels = 2, int output_channels = 2) :
                reverb(static_cast<void *>(reverb)),
                input_channels(input_channels),
                output_channels(output_channels),
                init(dattorro_init),
                process(dattorro_process),
                update(dattorro_update) {}

        void Init(t_sample sample_rate);

        void Process(const t_sample *frame_in, t_sample *frame_out);

        void Update();

        void UpdateMidiControl(uint8_t control, uint8_t value);

        void UpdateHardwareControl(uint8_t control, t_sample value);

    };

    // Multi-effects

    template<int CHANNEL_COUNT>
    class FxRack {
    private:
        ol::fx::DelayFx<CHANNEL_COUNT> delay1;
        ol::fx::ReverbFx reverb;
        FilterFx<CHANNEL_COUNT> filter1;
        SaturatorFx<CHANNEL_COUNT> saturator1;
        SaturatorFx<CHANNEL_COUNT> interstage_saturator;
        t_sample master_volume = 0.8f;
        int channel_count = 2;

    public:
        FxRack() = default;

        FxRack(DelayFx<CHANNEL_COUNT> &delay1, ReverbFx &reverb, FilterFx<CHANNEL_COUNT> &filt1,
               SaturatorFx<CHANNEL_COUNT> &sat1,
               SaturatorFx<CHANNEL_COUNT> &interstage_saturator,
               int channel_count = 2)
                : delay1(delay1),
                  reverb(reverb),
                  filter1(filt1),
                  saturator1(sat1),
                  interstage_saturator(interstage_saturator),
                  channel_count(channel_count) {}


        void Init(t_sample sample_rate) {
            delay1.Init(sample_rate);
            reverb.Init(sample_rate);
            filter1.Init(sample_rate);
            saturator1.Init(sample_rate);
            interstage_saturator.Init(sample_rate);
        };

        void Process(const t_sample *frame_in, t_sample *frame_out) {

            delay1.Process(frame_in, frame_out);
            reverb.Process(frame_in, frame_out);

            saturator1.Process(frame_in, frame_out);

            filter1.Process(frame_in, frame_out);

            for (int i = 0; i < channel_count; i++) {
                frame_out[i] *= master_volume;
            }
        };

        void Update() {
            delay1.Update();
            reverb.Update();
            filter1.Update();
            saturator1.Update();
        }

        void UpdateMidiControl(const uint8_t control, const uint8_t value) {
            delay1.UpdateMidiControl(control, value);
            reverb.UpdateMidiControl(control, value);
            filter1.UpdateMidiControl(control, value);
            saturator1.UpdateMidiControl(control, value);
            if (control == CC_CTL_VOLUME) {
                master_volume = ol::core::scale(value, 0, 127, 0, 1, 1);
            }
        }

    };


}
#endif //OL_DSP_FX_H
