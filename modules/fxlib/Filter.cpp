//
// Created by Orion Letizi on 11/13/23.
//

#include "Fx.h"

namespace ol::fx {

    FilterFx::FilterType FilterFx::Filter_MidiToType(uint8_t value) {
        return static_cast<FilterType>(ol::core::scale(value, 0, 127, 0, FilterType::LastType, 1));
    }

    void FilterFx::biquad_process(FilterFx *fx, const t_sample * frame_in, t_sample *frame_out) {
        frame_out[fx->frame_offset] = fx->asBiquad()->Process(frame_in[fx->frame_offset]);
    }

    void FilterFx::biquad_update(FilterFx *fx) {
        daisysp::Biquad *filt = fx->asBiquad();
        filt->SetCutoff(fx->cutoff);
        filt->SetRes(fx->resonance);
    }

    void FilterFx::biquad_init(FilterFx *fx, t_sample sample_rate) {
        fx->asBiquad()->Init(sample_rate);
        fx->Update();
    }

    void FilterFx::svf_process(FilterFx *fx, const t_sample *frame_in, t_sample *frame_out) {
        t_sample out = 0;
        daisysp::Svf *svf = fx->asSvf();
        svf->Process(frame_in[fx->frame_offset]);
        switch (fx->type) {
            case FilterType::Peak:
                out = svf->Peak();
                break;
            case FilterType::BandPass:
                out = svf->Band();
                break;
            case FilterType::Notch:
                out = svf->Notch();
                break;
            case FilterType::HighPass:
                out = svf->High();
                break;
            case FilterType::LowPass:
                out = svf->Low();
                break;
        }
        frame_out[fx->frame_offset] = out;
    }

    void FilterFx::svf_update(FilterFx *fx) {
        daisysp::Svf *filt = fx->asSvf();
        filt->SetRes(fx->resonance);
        filt->SetFreq(ol::core::scale(fx->cutoff, 0, 1, 0, 20000, 1));
        filt->SetDrive(fx->drive);
    }

    void FilterFx::svf_init(FilterFx *fx, const t_sample sample_rate) {
        daisysp::Svf * filt = fx->asSvf();
        filt->Init(sample_rate);
        fx->Update();
    }

    void FilterFx::Init(const t_sample sample_rate) {
        init(this, sample_rate);
    }

    void FilterFx::Process(const t_sample *frame_in, t_sample *frame_out) {
        process(this, frame_in, frame_out);
    }

    void FilterFx::Update() {
        update(this);
    }

    void FilterFx::UpdateHardwareControl(uint8_t control, t_sample value) {
        bool do_update = true;
        switch (control) {
            case CC_FILTER_RESONANCE:
                resonance = value;
                break;
            case CC_FILTER_CUTOFF:
                cutoff = ol::core::scale(value, 0, 1, 0, 1, 1.2);
                break;
            case CC_FILTER_DRIVE:
                drive = value;
                break;
            case CC_FILTER_TYPE:
                type = Filter_MidiToType(value);
                break;
            default:
                do_update = false;
                break;
        }
        if (do_update) {
            Update();
        }
    }

    void FilterFx::UpdateMidiControl(uint8_t control, uint8_t value) {
        bool do_update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);
        switch (control) {
            case CC_FILTER_RESONANCE:
                resonance = scaled;
                break;
            case CC_FILTER_CUTOFF:
                cutoff = ol::core::scale(value, 0, 127, 0, 1, 1.2);
                break;
            case CC_FILTER_DRIVE:
                drive = scaled;
                break;
            case CC_FILTER_TYPE:
                type = Filter_MidiToType(value);
                break;
            default:
                do_update = false;
                break;
        }
        if (do_update) {
            Update();
        }
    }

    daisysp::Biquad *FilterFx::asBiquad() {
        return static_cast<daisysp::Biquad *>(filter);
    }

    daisysp::Svf *FilterFx::asSvf() {
        return static_cast<daisysp::Svf *>(filter);
    }


}
