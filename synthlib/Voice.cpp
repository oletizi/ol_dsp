//
// Created by Orion Letizi on 11/8/23.
//

#include <iostream>
#include "Voice.h"

t_sample ol::synthlib::Voice::Process() {
    counter_++;

    // Oscillator
    updateWaveform();

    portamento_.SetHtime(control_panel_->portamento.Value());
    t_sample port = portamento_.Process(freq_);

    setOscillatorFrequency(port);

    t_sample value = processOscillators();//osc_1_.Process();
    float voice_gain = control_panel_->voice_gain.Value();
    float voice_clip = daisysp::SoftClip(voice_gain * value);
    value = voice_clip;

    // Filter
    env_filt_.SetAttackTime(control_panel_->env_filt_A.Value());
    env_filt_.SetDecayTime(control_panel_->env_filt_D.Value());
    env_filt_.SetSustainLevel(control_panel_->env_filt_S.Value());
    env_filt_.SetReleaseTime(control_panel_->env_filt_R.Value());
    float env_filt_value = env_filt_.Process(notes_on_ > 0);
    control_panel_->filter_cutoff.UpdateCv(env_filt_value);
    float filt_freq = control_panel_->filter_cutoff.Value();
    float filt_res = control_panel_->filter_resonance.Value();
    float filt_drive = control_panel_->filter_drive.Value();
    filt_1_.SetFreq(filt_freq);
    filt_1_.SetRes(filt_res);
    filt_1_.SetDrive(filt_drive);
    filt_1_.Process(value);
    value = filt_1_.Low();
    // Amplifier
    env_amp_.SetAttackTime(control_panel_->env_amp_A.Value());
    env_amp_.SetDecayTime(control_panel_->env_amp_D.Value());
    env_amp_.SetSustainLevel(control_panel_->env_amp_S.Value());
    env_amp_.SetReleaseTime(control_panel_->env_amp_R.Value());
    float amp = env_amp_.Process(notes_on_ > 0);
    value *= amp;
    if (counter_ % 22000 == 0) {
        counter_ = 0;
    }
    float volume = control_panel_->master_volume.Value();
    return value * volume;
}

void ol::synthlib::Voice::updateWaveform() {
    for (auto &oscillator: oscillators) {
        switch (control_panel_->GetOsc1Waveform()) {
            case ControlPanel::WAVE_SIN:
                oscillator.SetWaveform(daisysp::Oscillator::WAVE_SIN);
                break;
            case ControlPanel::WAVE_SAW:
                oscillator.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
                break;
            case ControlPanel::WAVE_TRI:
                oscillator.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_TRI);
                break;
            case ControlPanel::WAVE_SQUARE:
                oscillator.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
                break;
        }
    }
}

void ol::synthlib::Voice::setOscillatorFrequency(t_sample freq) {
    for (auto &oscillator: oscillators) {
        oscillator.SetFreq(freq);
    }
}

t_sample ol::synthlib::Voice::processOscillators() {
    t_sample rv = 0;

    t_sample slop_lfo_1_value = slop_lfo_1.Process();
    t_sample slop_lfo_2_value = slop_lfo_2.Process();
    //oscillators[0].PhaseAdd(slop_lfo_1_value * 0.0000005f);
    //oscillators[0].SetFreq(control_panel_->)
    oscillators[0].SetFreq(freq_ * slop_lfo_1_value * 0.001f);
    oscillators[2].SetFreq(freq_ * slop_lfo_2_value * 0.002f);
    //slop_lfo_1.PhaseAdd(slop_lfo_1_value * 0.000001f);

    rv += oscillators[0].Process() * control_panel_->osc_1_volume.Value();
    rv += oscillators[1].Process() * control_panel_->osc_2_volume.Value();
    rv += oscillators[2].Process() * control_panel_->osc_3_volume.Value();
    rv += oscillators[3].Process() * control_panel_->osc_4_volume.Value();

    return rv;
}
