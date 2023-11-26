//
// Created by Orion Letizi on 11/11/23.
//

#include "Delay.h"

namespace ol::fx::delay {
    void Delay_init(DelayFx *fx, t_sample sample_rate) {
        fx->delay_line->Init();
    }

    int Delay_process(DelayFx *fx, const t_sample &in, t_sample *out) {
        t_sample delay_out = fx->delay_line->Read();
        t_sample delay_in = in + (fx->feedback * delay_out);
        fx->delay_line->Write(delay_in);
        *out = (delay_out * fx->balance) + (in * (1 - fx->balance));
        return 0;
    }

    void Delay_update(DelayFx *fx) {
        // TODO: try using an read index for multi-tap support.
        fx->delay_line->SetDelay(ol::core::scale(fx->time, 0, 1, 0, MAX_DELAY, 1));
    }

    void Delay_Config(DelayFx *fx, daisysp::DelayLine<t_sample, MAX_DELAY> *delay_line) {
        fx->delay_line = delay_line;
        fx->Init = Delay_init;
        fx->Process = Delay_process;
        fx->Update = Delay_update;
    }

    void UpdateMidi(DelayFx *fx, uint8_t control, uint8_t value) {
        bool update = true;
        t_sample scaled = ol::core::scale(value, 0, 127, 0, 1, 1);

        switch (control) {
            case CC_DELAY_TIME:
                fx->time = scaled;
                break;
            case CC_DELAY_FEEDBACK:
                fx->feedback = scaled;
                break;
            case CC_DELAY_BALANCE:
                fx->balance = scaled;
                break;
            default:
                update = false;
        }
        if (update) {
            fx->Update(fx);
        }
    }
}

t_sample ol::fx::Delay::Process(t_sample in) {
    t_sample out;

    // Update parameters
    if (time_ != cp_->time.Value()) {
        time_ = cp_->time.Value();
        delay_line_->SetDelay(time_);
    }
    if (feedback_ != cp_->feedback.Value()) {
        feedback_ = cp_->feedback.Value();
    }
    if (cutoff_ != cp_->cutoff.Value()) {
        cutoff_ = cp_->cutoff.Value();
        filt_svf_.SetFreq(cutoff_);
    }
    if (resonance_ != cp_->resonance.Value()) {
        resonance_ = cp_->resonance.Value();
        filt_svf_.SetRes(resonance_);
    }
    // assign current delay value to output
    out = delay_line_->Read();

    // prepare feedback
    filt_svf_.Process((feedback_ * out) + in);

    // write feedback into delay
    float delay_input = filt_svf_.Low();
    delay_line_->Write(delay_input);
    //float delay_input = (feedback_ * out) + (in * 0.5f);
    //delay_line_->Write(delay_input);

    if (counter_ % 512 == 0) {
        counter_ = 0;
    }
    counter_++;
    return out;
}
