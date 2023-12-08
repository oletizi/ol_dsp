//
// Created by Orion Letizi on 11/15/23.
//
#include "Fx.h"

namespace ol::fx {

    // Dattorro

    void ReverbFx::dattorro_process(ReverbFx *fx, const t_sample *frame_in, t_sample *frame_out) {
        sDattorroVerb *verb = fx->asDattorroVerb();
        t_sample in1 = fx->input_channels > 0 ? frame_in[0] : 0;
        t_sample in2 = fx->input_channels > 1 ? frame_in[1] : in1;

        DattorroVerb_process(verb, (in1 + in2) / 2);

        t_sample out1 = DattorroVerb_getLeft(verb);
        t_sample out2 = DattorroVerb_getRight(verb);

        if (fx->output_channels > 0) {
            frame_out[0] = out1;
        }
        if (fx->output_channels > 1) {
            frame_out[1] = out2;
        }
    }

    void ReverbFx::dattorro_update(ReverbFx *fx) {
        sDattorroVerb *verb = fx->asDattorroVerb();
        DattorroVerb_setPreFilter(verb, fx->pre_cutoff);
        DattorroVerb_setInputDiffusion1(verb, fx->input_diffusion1);
        DattorroVerb_setInputDiffusion2(verb, fx->input_diffusion2);
        DattorroVerb_setDecay(verb, fx->decay_time);
        DattorroVerb_setDamping(verb, fx->cutoff);
        DattorroVerb_setDecayDiffusion(verb, fx->decay_diffusion);
    }

    void ReverbFx::dattorro_init(ReverbFx *fx, t_sample sample_rate) {}

    // ReverbSc
    void ReverbFx::reverbSc_process(ReverbFx *fx, const t_sample *frame_in, t_sample *frame_out) {
        const t_sample verb_in1 = fx->input_channels > 0 ? frame_in[0] : 0;
        const t_sample verb_in2 = fx->input_channels > 1 ? frame_in[1] : verb_in1;
        t_sample verb_out1 = 0;
        t_sample verb_out2 = 0;

        fx->asReverbSc()->Process(verb_in1, verb_in2, &verb_out1, &verb_out2);
        t_sample out1 = (verb_out1 * fx->balance) + (verb_in1 * (1 - fx->balance));
        t_sample out2 = (verb_out2 * fx->balance) + (verb_in2 * (1 - fx->balance));
        frame_out[0] = out1;
        if (fx->output_channels > 1) {
            frame_out[2] = out2;
        }
    }

    void ReverbFx::reverbSc_update(ReverbFx *fx) {
        auto *verb = fx->asReverbSc();
        verb->SetFeedback(fx->decay_time);
        verb->SetLpFreq(ol::core::scale(fx->cutoff, 0, 1, 0, 20000, 1));
    }

    void ReverbFx::reverbSc_init(ReverbFx *fx, const t_sample sample_rate) {
        fx->asReverbSc()->Init(sample_rate);
    }

    void ReverbFx::UpdateHardwareControl(uint8_t control, t_sample value) {
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
                cutoff = value;
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

    void ReverbFx::UpdateMidiControl(uint8_t control, uint8_t value) {
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
                cutoff = scaled;
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

    void ReverbFx::Init(const t_sample sample_rate) {
        this->init(this, sample_rate);
    }

    void ReverbFx::Process(const t_sample *frame_in, t_sample *frame_out) {
        this->process(this, frame_in, frame_out);
    }

    void ReverbFx::Update() {
        this->update(this);
    }
}

