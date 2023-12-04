//
// Created by Orion Letizi on 11/27/23.
//

#ifndef OL_DSP_FX_H
#define OL_DSP_FX_H
#define SPFLOAT t_sample

#include "corelib/ol_corelib.h"

extern "C" {
#include "soundpipe.h"
}

#include "verb.h"
#include "daisysp.h"
#include "corelib/cc_map.h"

#define MAX_DELAY 48000
namespace ol::fx {

    template<typename T>
    class TransferFunction {
    public:
        virtual t_sample Process(T data, t_sample in) = 0;
    };

    // Saturator
    struct SaturatorFx {

        t_sample drive = 1;

        void (*Init)(SaturatorFx *, t_sample sample_rate) = nullptr;

        int (*Process)(SaturatorFx *, const t_sample &in, t_sample *out) = nullptr;

        void (*Update)(SaturatorFx *) = nullptr;

        TransferFunction<SaturatorFx *> *transferFunction = nullptr;

        sp_saturator *saturator = nullptr;
        sp_data *spdata = nullptr;
    };

    class HyperTan : public TransferFunction<SaturatorFx *> {
    public:
        t_sample Process(SaturatorFx *, t_sample in) override;
    };

    void Saturator_UpdateMidiControl(SaturatorFx *, uint8_t control, uint8_t value);

    void Saturator_Config(SaturatorFx *, sp_saturator *, sp_data *);

    void Saturator_Config(SaturatorFx *, TransferFunction<SaturatorFx *> *);


    // Filter

    enum FilterType {
        LowPass,
        BandPass,
        HighPass,
        Notch,
        Peak,
        LastType = Peak
    };

    FilterType Filter_MidiToType(uint8_t value);

    enum FilterStyle {
        Biquad,
        Svf,
        MoogLadder,
        LastStyle = MoogLadder
    };

    FilterStyle Filter_MidiToStyle(uint8_t value);

    struct FilterFx {

        t_sample cutoff = 1;
        t_sample resonance = 0;
        t_sample drive = 0;
        FilterType type = LowPass;

        void *filt = nullptr;

        void (*Init)(FilterFx *, t_sample sample_rate) = nullptr;

        int (*Process)(FilterFx *, const float &in, float *out) = nullptr;

        void (*Update)(FilterFx *) = nullptr;
    };

    void Filter_UpdateHardwareControl(FilterFx *fx, uint8_t control, t_sample value);

    void Filter_UpdateMidi(FilterFx *, uint8_t control, uint8_t value);

    void Filter_Biquad_Config(FilterFx *, daisysp::Biquad *);

    void Filter_Svf_Config(FilterFx *, daisysp::Svf *);

    // Delay

    struct DelayFx {
        t_sample time = 0.5;
        t_sample feedback = 0.5;
        t_sample balance = 0.33;
        FilterFx *filter = nullptr;

        void (*Init)(DelayFx *, t_sample sample_rate) = nullptr;

        int (*Process)(DelayFx *, const t_sample &in, t_sample *out) = nullptr;

        void (*Update)(DelayFx *reverb) = nullptr;

        daisysp::DelayLine<t_sample, MAX_DELAY> *delay_line = nullptr;
    };

    void Delay_Config(DelayFx *, daisysp::DelayLine<t_sample, MAX_DELAY> *, FilterFx *);

    void Delay_UpdateMidiControl(DelayFx *, uint8_t control, uint8_t value);

    void Delay_UpdateHardwareControl(DelayFx *, uint8_t control, uint8_t value);

    // Reverb

    struct ReverbFx {
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

        void (*PrintLine)(const char *) = nullptr;

        void (*Init)(ReverbFx *, t_sample sample_rate) = nullptr;

        int (*Process)(ReverbFx *, const float &in1, const float &in2, float *out1, float *out2) = nullptr;

        void (*Update)(ReverbFx *reverb) = nullptr;

        void *reverb = nullptr;
    };

    /* Call one of these Init functions first.*/
    void ReverbSc_Config(ReverbFx *fx, daisysp::ReverbSc *reverbsc);

    void Dattorro_Config(ReverbFx *fx, sDattorroVerb *verb);

    void Reverb_UpdateMidiControl(ReverbFx *fx, uint8_t control, uint8_t value);

    void Reverb_UpdateHardwareControl(ReverbFx *fx, uint8_t control, t_sample value);


    // Multi-effects

    struct FxRack {

        ol::fx::DelayFx *delay1 = nullptr;
        ol::fx::DelayFx *delay2 = nullptr;
        ol::fx::ReverbFx *reverb = nullptr;
        FilterFx *filter1 = nullptr;
        FilterFx *filter2 = nullptr;
        SaturatorFx *saturator1 = nullptr;
        SaturatorFx *saturator2 = nullptr;
        SaturatorFx *interstage_saturator = nullptr;
        t_sample master_volume = 0.8f;

        void (*Init)(FxRack *, t_sample sample_rate) = nullptr;

        int (*Process)(FxRack *, const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) = nullptr;

        void (*Update)(FxRack *) = nullptr;

    };

    void FxRack_UpdateMidiControl(FxRack *, uint8_t control, uint8_t value);

    void
    FxRack_Config(FxRack *, DelayFx *, DelayFx *, ReverbFx *, FilterFx *, FilterFx *, SaturatorFx *, SaturatorFx *, SaturatorFx * interstage_saturator);

}
#endif //OL_DSP_FX_H
