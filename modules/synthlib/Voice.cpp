//
// Created by Orion Letizi on 11/8/23.
//

#include <iostream>
#include "Voice.h"

namespace ol::synth {

    t_sample Voice_process(Voice *v) {

        t_sample freq = v->freq_;
        //t_sample freq = v->portamento_.Process(freq);
        t_sample rv = 0;

//        t_sample slop_lfo_1_value = v->slop_lfo_1.Process();
//        t_sample slop_lfo_2_value = v->slop_lfo_2.Process();
//        t_sample slop_lfo_3_value = v->slop_lfo_3.Process();
//        t_sample slop_lfo_4_value = v->slop_lfo_4.Process();
//
//        t_sample osc_1_slop_freq = port + port * slop_lfo_1_value * v->slop_factor * v->osc_1_slop;
//        t_sample osc_2_slop_freq = port - port * slop_lfo_2_value * v->slop_factor * v->osc_2_slop;
//        t_sample osc_3_slop_freq = port + port * slop_lfo_3_value * v->slop_factor * v->osc_3_slop;
//        t_sample osc_4_slop_freq = port - port * slop_lfo_4_value * v->slop_factor * v->osc_4_slop;
//        v->slop_lfo_1.PhaseAdd(slop_lfo_1_value * 0.000001f);
//        v->slop_lfo_2.PhaseAdd(slop_lfo_2_value * 0.0000009f);
//
        v->oscillators[0].SetFreq(freq);
//        v->oscillators[1].SetFreq(osc_2_slop_freq);
//        v->oscillators[2].SetFreq(osc_3_slop_freq);
//        v->oscillators[3].SetFreq(osc_4_slop_freq);

        rv += v->oscillators[0].Process() * v->osc_1_mix;
//        rv += v->oscillators[1].Process() * v->osc_2_mix;;
//        rv += v->oscillators[2].Process() * v->osc_3_mix;
//        rv += v->oscillators[3].Process() * v->osc_4_mix;

        // Filter
//        v->filt_freq = v->filter_envelope.Process(v->notes_on > 0);
//
//        v->filt_1.SetFreq(v->filt_freq);
//        v->filt_1.SetRes(v->filt_res);
//        v->filt_1.SetDrive(v->filt_drive);
//        v->filt_1.Process(rv);
//        rv = v->filt_1.Low();

        //float amp = v->amp_envelope.Process(v->notes_on > 0);
        //rv *= amp;
        //rv *= v->master_volume;
        return rv;
    }

    void Voice_updateMidiControl(Voice *v, int ctl, int val) {
        std::cout << "Synth ctl: " << ctl << "; val: " << val << std::endl;
        bool update = true;
        t_sample scaled = ol::core::scale(val, 0, 127, 0, 1, 1);
        switch (ctl) {
            case CC_CTL_VOLUME:
                v->master_volume = scaled;
                break;
            case CC_CTL_PORTAMENTO:
                v->portamento_htime = scaled;
                break;
            case CC_FILT_CUTOFF:
                v->filt_freq = daisysp::mtof(val);
                break;
            case CC_FILT_Q:
                v->filt_res = scaled;
                break;
            case CC_FILT_DRIVE:
                v->filt_drive = scaled;
                break;
            case CC_ENV_FILT_AMT:
                v->filter_envelope_amount = scaled;
                break;
            case CC_ENV_FILT_A:
                v->filter_attack = scaled;
                break;
            case CC_ENV_FILT_D:
                v->filter_decay = scaled;
                break;
            case CC_ENV_FILT_S:
                v->filter_sustain = scaled;
                break;
            case CC_ENV_FILT_R:
                v->filter_release = scaled;
                break;
            case CC_ENV_AMP_A:
                v->amp_attack = scaled;
                break;
            case CC_ENV_AMP_D:
                v->amp_decay = scaled;
                break;
            case CC_ENV_AMP_S:
                v->amp_sustain = scaled;
                break;
            case CC_ENV_AMP_R:
                v->amp_release = scaled;
                break;
            case CC_OSC_1_VOLUME:
                v->osc_1_mix = scaled;
                break;
            case CC_OSC_2_VOLUME:
                v->osc_2_mix = scaled;
                break;
            case CC_OSC_3_VOLUME:
                v->osc_3_mix = scaled;
                break;
            case CC_OSC_4_VOLUME:
                v->osc_4_mix = scaled;
                break;
            case CC_OSC_1_SLOP:
                v->osc_1_slop = scaled;
                break;
            case CC_OSC_2_SLOP:
                v->osc_2_slop = scaled;
                break;
            case CC_OSC_3_SLOP:
                v->osc_3_slop = scaled;
                break;
            case CC_OSC_4_SLOP:
                v->osc_4_slop = scaled;
                break;
            default:
                std::cout << "CC not mapped: " << ctl << std::endl;
                update = false;
                break;
        }
        if (update) {
            v->Update(v);
        }
    }

    void Voice_noteOn(Voice *v, uint8_t midi_note, uint8_t velocity) {
        std::cout << "Note on: " << midi_note << "; velocity: " << velocity << std::endl;
        v->notes_on++;
        v->freq_ = daisysp::mtof(midi_note);
        v->amp_envelope.Retrigger(false);
        v->filter_envelope.Retrigger(false);
        std::cout << "  Frequency: " << v->freq_ << std::endl;
    }

    void Voice_noteOff(Voice *v, uint8_t midi_note, uint8_t velocity) {
        if (v->notes_on > 0) {
            v->notes_on--;
        }
    }

    void Voice_update(Voice *v) {
        v->portamento_.SetHtime(v->portamento_htime);

        // Filter
        v->filter_envelope.SetAttackTime(v->filter_attack);
        v->filter_envelope.SetDecayTime(v->filter_decay);
        v->filter_envelope.SetSustainLevel(v->filter_sustain);
        v->filter_envelope.SetReleaseTime(v->filter_release);

        // Amplifier
        v->amp_envelope.SetAttackTime(v->amp_attack);
        v->amp_envelope.SetDecayTime(v->amp_decay);
        v->amp_envelope.SetSustainLevel(v->amp_sustain);
        v->amp_envelope.SetReleaseTime(v->amp_release);
    }

    void Voice_init(Voice *v, t_sample sample_rate) {
        v->slop_lfo_1.Init(sample_rate);
        v->slop_lfo_2.Init(sample_rate);
        v->slop_lfo_3.Init(sample_rate);
        v->slop_lfo_4.Init(sample_rate);
        v->slop_lfo_1.SetFreq(0.0001f);
        v->slop_lfo_2.SetFreq(0.00009f);
        v->slop_lfo_3.SetFreq(0.00008f);
        v->slop_lfo_4.SetFreq(0.00011f);
        for (auto &oscillator: v->oscillators) {
            oscillator.Init(sample_rate);
        }

        v->filt_1.Init(sample_rate);
        v->filter_envelope.Init(sample_rate);
        v->amp_envelope.Init(sample_rate);
        v->portamento_.Init(sample_rate, 0);
    }

    void Voice_Config(Voice *v) {
        v->Init = Voice_init;
        v->Process = Voice_process;
        v->Update = Voice_update;
        v->UpdateMidiControl = Voice_updateMidiControl;
        v->NoteOn = Voice_noteOn;
        v->NoteOff = Voice_noteOff;
    }
}