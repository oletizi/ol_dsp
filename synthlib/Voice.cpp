//
// Created by Orion Letizi on 11/8/23.
//

#include <iostream>
#include "Voice.h"

t_sample ol::synthlib::Voice::Process() {
    counter_++;

    // Oscillator
    switch (control_panel_->GetOsc1Waveform()) {
        case ControlPanel::WAVE_SIN:
            osc1_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
            break;
        case ControlPanel::WAVE_SAW:
            osc1_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
            break;
        case ControlPanel::WAVE_TRI:
            osc1_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_TRI);
            break;
        case ControlPanel::WAVE_SQUARE:
            osc1_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
            break;
    }
    portamento_.SetHtime(control_panel_->portamento.Value());
    t_sample port = portamento_.Process(freq_);

    osc1_.SetFreq(port);
    t_sample value = osc1_.Process();
    float pre_gain = value;
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

    //value = daisysp::SoftClip(value * voice_gain);

    // Amplifier
    env_amp_.SetAttackTime(control_panel_->env_amp_A.Value());
    env_amp_.SetDecayTime(control_panel_->env_amp_D.Value());
    env_amp_.SetSustainLevel(control_panel_->env_amp_S.Value());
    env_amp_.SetReleaseTime(control_panel_->env_amp_R.Value());
    float amp = env_amp_.Process(notes_on_ > 0);
    value *= amp;
    if (counter_ % 22000 == 0) {
        //std::cout << "Portamento: " << port << "; htime: " << portamento_.GetHtime() << std::endl;
        //`std::cout << "Gain: " << voice_gain << "; Clip: " << voice_clip << "; Pre gain: " << pre_gain << std::endl;
        counter_ = 0;
    }
    float volume = control_panel_->master_volume.Value();
    return value * volume;
}
