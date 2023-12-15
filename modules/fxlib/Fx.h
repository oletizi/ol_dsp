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

#define MAX_DELAY 48000
namespace ol::fx {

    class SaturatorFx {
    public:
        typedef t_sample (*TransferFunction)(SaturatorFx *data, const t_sample in);

        SaturatorFx(TransferFunction fn, int frame_offset = 0) : transferFunction(fn), frame_offset(frame_offset) {}

        // Default implementation uses hyberbolic tangent
        SaturatorFx(int frame_offset = 0) : SaturatorFx(hyperbolic_tangent, frame_offset) {}

        void Init(const t_sample sample_rate);

        void Process(const t_sample *frame_in, t_sample *frame_out);

        void Update();

        void UpdateMidiControl(uint8_t control, uint8_t value);

    private:
        t_sample drive = 1;
        TransferFunction transferFunction = nullptr;

        // default saturator transfer function
        static inline t_sample hyperbolic_tangent(SaturatorFx *fx, const t_sample in) {
            return tanh(fx->drive * in);
        }

        int frame_offset = 0;
    };

    class FilterFx {
    private:
        enum FilterType {
            LowPass,
            BandPass,
            HighPass,
            Notch,
            Peak,
            LastType = Peak
        };

        int frame_offset;
        t_sample cutoff = 1;
        t_sample resonance = 0;
        t_sample drive = 0;
        FilterType type = LowPass;
        void *filter = nullptr;

        static FilterType Filter_MidiToType(uint8_t value);

        // Generic filter operations
        typedef void (*process_function)(FilterFx *, const t_sample *frame_in, t_sample *frame_out);

        typedef void (*update_function)(FilterFx *);

        typedef void (*init_function)(FilterFx *, const t_sample sample_rate);

        process_function process = nullptr;
        update_function update = nullptr;
        init_function init = nullptr;

        // Filter-specific implementations
        static void svf_init(FilterFx *, const t_sample sample_rate);

        static void svf_process(FilterFx *, const t_sample *frame_in, t_sample *frame_out);

        static void svf_update(FilterFx *);

        // Biquad implementations

        static void biquad_init(FilterFx *, const t_sample sample_rate);

        static void biquad_process(FilterFx *, const t_sample *frame_in, t_sample *frame_out);

        static void biquad_update(FilterFx *);

    public:

        FilterFx(daisysp::Svf &svf, int frame_offset = 0) :
                filter(&svf),
                frame_offset(frame_offset),
                init(svf_init),
                process(svf_process),
                update(svf_update) {}

        FilterFx(daisysp::Biquad &biquad, int frame_offset = 0) :
                filter(&biquad),
                frame_offset(frame_offset),
                init(biquad_init),
                process(biquad_process),
                update(biquad_update) {}

        void Init(const t_sample sample_rate);

        void Process(const t_sample *frame_in, t_sample *frame_out);

        void Update();

        void UpdateHardwareControl(uint8_t control, t_sample value);

        void UpdateMidiControl(uint8_t control, uint8_t value);

        daisysp::Biquad *asBiquad();

        daisysp::Svf *asSvf();
    };

    // Delay

    class DelayFx {
    private:
        int frame_offset_;
        t_sample time = 0.5;
        t_sample feedback = 0.5;
        t_sample balance = 0.33;
        daisysp::DelayLine<t_sample, MAX_DELAY> &delay_line;
        FilterFx &filter;
    public:
        DelayFx(daisysp::DelayLine<t_sample, MAX_DELAY> &delay_line, FilterFx &filter, int frame_offset = 0)
                : delay_line(delay_line), filter(filter), frame_offset_(frame_offset) {}

        void Init(t_sample sample_rate);

        void Process(const t_sample *frame_in, t_sample *frame_out);

        void Update();

        void UpdateMidiControl(uint8_t control, uint8_t value);

        void UpdateHardwareControl(uint8_t control, uint8_t value);

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
        ReverbFx(daisysp::ReverbSc &reverb,
                 int input_channels = 2,
                 int output_channels = 2) :
                reverb(static_cast<void *>(&reverb)),
                input_channels(input_channels),
                output_channels(output_channels),
                init(reverbSc_init),
                process(reverbSc_process),
                update(reverbSc_update) {}

        // Dattorro reverb ctor
        ReverbFx(sDattorroVerb *reverb, int input_channels = 2, int output_channels = 2) :
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

    class FxRack {
    private:
        ol::fx::DelayFx &delay1;
        ol::fx::DelayFx &delay2;
        ol::fx::ReverbFx &reverb;
        FilterFx &filter1;
        FilterFx &filter2;
        SaturatorFx &saturator1;
        SaturatorFx &saturator2;
        SaturatorFx &interstage_saturator;
        t_sample master_volume = 0.8f;
        int channel_count = 2;

    public:
        FxRack(DelayFx &delay1, DelayFx &delay2, ReverbFx &reverb, FilterFx &filt1, FilterFx &filt2, SaturatorFx &sat1,
               SaturatorFx &sat2,
               SaturatorFx &interstage_saturator,
               int channel_count = 2)
                : delay1(delay1), delay2(delay2),
                  reverb(reverb),
                  filter1(filt1), filter2(filt2),
                  saturator1(sat1), saturator2(sat2),
                  interstage_saturator(interstage_saturator),
                  channel_count(channel_count) {}


        void Init(t_sample sample_rate);

        void Process(const t_sample *input_frame, t_sample *output_frame);

        void Update();

        void UpdateMidiControl(uint8_t control, uint8_t value);

    };


}
#endif //OL_DSP_FX_H
