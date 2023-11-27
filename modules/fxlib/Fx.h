//
// Created by Orion Letizi on 11/27/23.
//

#ifndef OL_DSP_FX_H
#define OL_DSP_FX_H

#include "verb.h"
#include "daisysp.h"
#include "ol_corelib.h"
#include "cc_map.h"

#define MAX_DELAY 48000
namespace ol::fx {

// Fiilter
    struct FiltFx {
        t_sample cutoff = 0.5;
        t_sample resonance = 0;
        t_sample drive = 0;
        void *filt = nullptr;

        void (*Init)(FiltFx *, t_sample sample_rate) = nullptr;

        int (*Process)(FiltFx *, const float &in, float *out) = nullptr;

        void (*Update)(FiltFx *) = nullptr;
    };

    void Filter_UpdateMidi(FiltFx *, uint8_t control, uint8_t value);

    void Filter_Biquad_Config(FiltFx *, daisysp::Biquad *);

    void Filter_Svf_Config(FiltFx *, daisysp::Svf *);

    // Delay

    struct DelayFx {
        t_sample time = 0.5;
        t_sample feedback = 0.5;
        t_sample cutoff = 0.5;
        t_sample resonance = 0.2;
        t_sample balance = 0.5;

        void (*Init)(DelayFx *, t_sample sample_rate) = nullptr;

        int (*Process)(DelayFx *, const t_sample &in, t_sample *out) = nullptr;

        void (*Update)(DelayFx *reverb) = nullptr;

        daisysp::DelayLine<t_sample, MAX_DELAY> *delay_line = nullptr;
    };

    void Delay_Config(DelayFx *, daisysp::DelayLine<t_sample, MAX_DELAY> *);

    void Delay_UpdateMidiControl(DelayFx *, uint8_t control, uint8_t value);

    // Reverb

    struct ReverbFx {
        t_sample sample_rate = 44100;
        t_sample decay_time = 0.5;
        t_sample cutoff = 0.5;
        t_sample early_predelay = 0.1;
        t_sample predelay = 0.2;
        t_sample pre_cutoff = 0.5;
        t_sample input_diffusion1 = 0.5;
        t_sample input_diffusion2 = 0.5;
        t_sample decay_diffusion = 0.5;
        t_sample balance = 0.25;

        void (*Init)(ReverbFx *, t_sample sample_rate) = nullptr;

        int (*Process)(ReverbFx *, const float &in1, const float &in2, float *out1, float *out2) = nullptr;

        void (*Update)(ReverbFx *reverb) = nullptr;

        void *reverb = nullptr;
    };

    /* Call one of these Init functions first.*/
    void ReverbSc_Config(ReverbFx *fx, daisysp::ReverbSc *reverbsc);

    void Dattorro_Config(ReverbFx *fx, sDattorroVerb *verb);

    void Reverb_UpdateMidiControl(ReverbFx *fx, uint8_t control, uint8_t value);


    // Multi-effects

    struct FxRack {

        ol::fx::DelayFx *delay1 = nullptr;
        ol::fx::DelayFx *delay2 = nullptr;
        ol::fx::ReverbFx *reverb = nullptr;

        void (*Init)(FxRack *, t_sample sample_rate) = nullptr;

        int (*Process)(FxRack *, const t_sample &in1, const t_sample &in2, t_sample *out1, t_sample *out2) = nullptr;

        void (*Update)(FxRack *) = nullptr;
    };

    void FxRack_UpdateMidiControl(FxRack *, uint8_t control, uint8_t value);

    void FxRack_Config(FxRack *, DelayFx *, DelayFx *, ReverbFx *);

}
#endif //OL_DSP_FX_H
