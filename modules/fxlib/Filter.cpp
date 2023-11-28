//
// Created by Orion Letizi on 11/13/23.
//

#include "Fx.h"

namespace ol::fx {

    FilterType Filter_MidiToType(uint8_t value) {
        return static_cast<FilterType>(ol::core::scale(value, 0, 127, 0, FilterType::LastType, 1));
    }

    FilterStyle Filter_MidiToStyle(uint8_t value) {
        return static_cast<FilterStyle>(ol::core::scale(value, 0, 127, 0, FilterStyle::LastStyle, 1));
    }


    // Biquad
    daisysp::Biquad *Biquad_get(FilterFx *fx) {
        return static_cast<daisysp::Biquad *>(fx->filt);
    }

    int Biquad_process(FilterFx *fx, const float &in, float *out) {
        auto filter = Biquad_get(fx);
        *out = filter->Process(in);
        return 0;
    }

    void Biquad_update(FilterFx *fx) {
        daisysp::Biquad *filt = Biquad_get(fx);
        filt->SetCutoff(fx->cutoff);
        filt->SetRes(fx->resonance);
    }

    void Biquad_init(FilterFx *fx, t_sample sample_rate) {
        Biquad_get(fx)->Init(sample_rate);
        Biquad_update(fx);
    }

    void Filter_Biquad_Config(FilterFx *fx, daisysp::Biquad *filt) {
        fx->filt = filt;
        fx->Init = Biquad_init;
        fx->Process = Biquad_process;
        fx->Update = Biquad_update;
    }

    // Svf
    daisysp::Svf *Svf_get(FilterFx *fx) {
        return static_cast<daisysp::Svf *>(fx->filt);
    }

    int Svf_process(FilterFx *fx, const float &in, float *out) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->Process(in);
        switch (fx->type) {
            case FilterType::Peak:
                *out = filt->Peak();
                break;
            case FilterType::BandPass:
                *out = filt->Band();
                break;
            case FilterType::Notch:
                *out = filt->Notch();
                break;
            case FilterType::HighPass:
                *out = filt->High();
                break;
            case FilterType::LowPass:
                *out = filt->Low();
                break;
        }
        return 0;
    }

    void Svf_update(FilterFx *fx) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->SetRes(fx->resonance);
        filt->SetFreq(ol::core::scale(fx->cutoff, 0, 1, 0, 20000, 1));
        filt->SetDrive(fx->drive);
    }

    void Svf_init(FilterFx *fx, t_sample sample_rate) {
        daisysp::Svf *filt = Svf_get(fx);
        filt->Init(sample_rate);
        Svf_update(fx);
    }

    void Filter_Svf_Config(FilterFx *fx, daisysp::Svf *filt) {
        fx->filt = filt;
        fx->Init = Svf_init;
        fx->Process = Svf_process;
        fx->Update = Svf_update;
    }

    void Filter_UpdateMidi(FilterFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
        switch (control) {
            case CC_FILTER_RESONANCE:
                fx->resonance = scaled;
                break;
            case CC_FILTER_CUTOFF:
                fx->cutoff = scaled;
                break;
            case CC_FILTER_DRIVE:
                fx->drive = scaled;
                break;
            case CC_FILTER_TYPE:
                fx->type = Filter_MidiToType(value);
                break;
            default:
                update = false;
                break;
        }
        if (update) {
            fx->Update(fx);
        }
    }
}
