//
// Created by Orion Letizi on 11/8/23.
//

#include "synthlib/ol_synthlib.h"

namespace ol::synth {

    void SynthVoice::Process(t_sample *frame_out) {
        t_sample freq = freq_;
        freq = portamento.Process(freq);
        sound_source.SetFreq(freq);

        sound_source.Process(frame_out);
        *frame_out *= osc_1_mix;

        // Filter
        t_sample env_value = filter_envelope.Process(playing);
        t_sample filter_frequency = filt_freq + ((env_value * 20000) * filter_envelope_amount);

        filter.SetFreq(filter_frequency);
        filter.Process(*frame_out);
        *frame_out = filter.Low();

        float amp = amp_envelope.Process(playing);
        *frame_out *= amp;
        *frame_out *= master_volume;
    }

    void SynthVoice::UpdateMidiControl(uint8_t ctl, uint8_t val) {
        bool update = true;
        t_sample scaled = ol::core::scale(val, 0, 127, 0, 1, 1);
        switch (ctl) {
            case CC_CTL_VOLUME:
                master_volume = scaled;
                break;
            case CC_CTL_PORTAMENTO:
                portamento_htime = ol::core::scale(val, 0, 127, 0, 1, 4);
                break;
            case CC_FILT_CUTOFF:
                filt_freq = ol::core::scale(val, 0, 127, 0, 20000, 2.5);
                break;
            case CC_FILT_Q:
                filt_res = scaled;
                break;
            case CC_FILT_DRIVE:
                filt_drive = scaled;
                break;
            case CC_ENV_FILT_AMT:
                filter_envelope_amount = scaled;
                break;
            case CC_ENV_FILT_A:
                filter_attack = scaled;
                break;
            case CC_ENV_FILT_D:
                filter_decay = ol::core::scale(val, 0, 127, 0, 1, 3);
                break;
            case CC_ENV_FILT_S:
                filter_sustain = scaled;
                break;
            case CC_ENV_FILT_R:
                filter_release = scaled;
                break;
            case CC_ENV_AMP_A:
                amp_attack = scaled;
                break;
            case CC_ENV_AMP_D:
                amp_decay = scaled;
                break;
            case CC_ENV_AMP_S:
                amp_sustain = scaled;
                break;
            case CC_ENV_AMP_R:
                amp_release = scaled;
                break;
            case CC_OSC_1_VOLUME:
                osc_1_mix = scaled;
                break;
            default:
                update = false;
                break;
        }
        if (update) {
            Update();
        }
    }

    void SynthVoice::NoteOn(uint8_t midi_note, uint8_t velocity) {
        playing = midi_note;
        freq_ = daisysp::mtof(midi_note);
        amp_envelope.Retrigger(true);
        filter_envelope.Retrigger(true);
    }

    void SynthVoice::NoteOff(uint8_t midi_note, uint8_t velocity) {
        playing = 0;
    }

    void SynthVoice::Update() {
        // Portamento
        portamento.SetHtime(portamento_htime);

        // Filter
        filter.SetFreq(filt_freq);
        filter.SetRes(filt_res);
        filter.SetDrive(filt_drive);
        filter_envelope.SetAttackTime(filter_attack);
        filter_envelope.SetDecayTime(filter_decay);
        filter_envelope.SetSustainLevel(filter_sustain);
        filter_envelope.SetReleaseTime(filter_release);

        // Amplifier
        amp_envelope.SetAttackTime(amp_attack);
        amp_envelope.SetDecayTime(amp_decay);
        amp_envelope.SetSustainLevel(amp_sustain);
        amp_envelope.SetReleaseTime(amp_release);
    }

    void SynthVoice::Init(t_sample sr) {
        sample_rate = sr;
        sound_source.Init(sr);

        filter.Init(sample_rate);
        filter_envelope.Init(sample_rate);
        amp_envelope.Init(sample_rate);
        portamento.Init(sample_rate, 0);
        initialized = true;
    }

    uint8_t SynthVoice::Playing() {
        return playing;
    }
}