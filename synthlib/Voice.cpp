//
// Created by Orion Letizi on 11/8/23.
//

#include <iostream>
#include "Voice.h"

t_sample ol::synthlib::Voice::Process() {
    counter_++;

    // Oscillator
    switch (control_panel_->wave_form) {

        case ControlPanel::WAVE_SIN:
            osc1_.SetWaveform(osc1_.WAVE_SIN);
            break;
        case ControlPanel::WAVE_SAW:
            osc1_.SetWaveform(osc1_.WAVE_POLYBLEP_SAW);
            break;
    }
    osc1_.SetFreq(freq_);
    t_sample value = osc1_.Process();

    // Filter
    env_filt_.SetAttackTime(control_panel_->env_filt_A.Value());
    env_filt_.SetDecayTime(control_panel_->env_filt_D.Value());
    env_filt_.SetSustainLevel(control_panel_->env_filt_S.Value());
    env_filt_.SetReleaseTime(control_panel_->env_filt_R.Value());
    float env_filt_value = env_filt_.Process(notes_on_ > 0);
    control_panel_->filter_cutoff.UpdateCv(env_filt_value);
    float filt_freq = control_panel_->filter_cutoff.Value();
    filt_1_.SetFreq(filt_freq);
    filt_1_.Process(value);
    value = filt_1_.Low();

    // Amplifier
    env_amp_.SetAttackTime(control_panel_->env_amp_A.Value());
    env_amp_.SetDecayTime(control_panel_->env_amp_D.Value());
    env_amp_.SetSustainLevel(control_panel_->env_amp_S.Value());
    env_amp_.SetReleaseTime(control_panel_->env_amp_R.Value());
    float amp = env_amp_.Process(notes_on_ > 0);
    value *= amp;
    if (counter_ % 2200 == 0) {
        //std::cout << "filt freq: " << filt_freq << std::endl;
        counter_ = 0;
    }
    return value;
}
