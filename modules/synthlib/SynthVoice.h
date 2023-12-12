//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SYNTHVOICE_H
#define OL_DSP_SYNTHVOICE_H

#include "Voice.h"

#define PRINTF printf

namespace ol::synth {
    template<int CHANNEL_COUNT>
    class SynthVoice : public Voice {
    public:
        explicit SynthVoice(SoundSource *sound_source,
                            daisysp::Svf *filters[CHANNEL_COUNT],
                            daisysp::Adsr *filter_envelope,
                            daisysp::Adsr *amp_envelope,
                            daisysp::Port *portamento)
                : sound_source_(sound_source),
                  filter_envelope_(filter_envelope),
                  amp_envelope_(amp_envelope),
                  portamento_(portamento) {
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                filters_[i] = filters[i];
            }
        }
        void Init(t_sample sr) override {
            sample_rate = sr;
            sound_source_->Init(sr);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                filters_[i]->Init(sample_rate);
            }
            filter_envelope_->Init(sample_rate);
            amp_envelope_->Init(sample_rate);
            portamento_->Init(sample_rate, 0);
            initialized = true;
        }

        void Update() override {
            // Portamento
            portamento_->SetHtime(portamento_htime);

            // Filter
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                auto filter = filters_[i];
                // XXX: probably don't need to set frequency here, since it's calculated in process loop based envelope
                filter->SetFreq(filt_freq);
                filter->SetRes(filt_res);
                filter->SetDrive(filt_drive);
            }

            filter_envelope_->SetAttackTime(filter_attack);
            filter_envelope_->SetDecayTime(filter_decay);
            filter_envelope_->SetSustainLevel(filter_sustain);
            filter_envelope_->SetReleaseTime(filter_release);

            // Amplifier
            amp_envelope_->SetAttackTime(amp_attack);
            amp_envelope_->SetDecayTime(amp_decay);
            amp_envelope_->SetSustainLevel(amp_sustain);
            amp_envelope_->SetReleaseTime(amp_release);
        }


        void Process(t_sample *frame_out) override {
            sound_source_->SetFreq(portamento_->Process(freq_));
            sound_source_->Process(frame_out);

            t_sample filter_frequency =
                    filt_freq + ((filter_envelope_->Process(Gate()) * 20000) * filter_envelope_amount);
            t_sample amp = amp_envelope_->Process(Gate());

            for (int i = 0; i < CHANNEL_COUNT; i++) {
                auto filter = filters_[i];
                frame_out[i] *= osc_1_mix;
                filter->SetFreq(filter_frequency);
                filter->Process(frame_out[i]);
                frame_out[i] = filter->Low() * amp * master_volume;
            }
        }

        void UpdateMidiControl(uint8_t ctl, uint8_t val) override {
            bool update = true;
            t_sample scaled = ol::core::scale(val, 0, 127, 0, 1, 1);
            switch (ctl) {
                case CC_CTL_VOLUME:
                    master_volume = scaled;
                    PRINTF("  volume: %f\n", master_volume);
                    break;
                case CC_CTL_PORTAMENTO:
                    portamento_htime = ol::core::scale(val, 0, 127, 0, 1, 4);
                    PRINTF("  port: %f\n", portamento_htime);
                    break;
                case CC_FILT_CUTOFF:
                    filt_freq = ol::core::scale(val, 0, 127, 0, 20000, 2.5);
                    PRINTF("  cutoff: %f\n", filt_freq);
                    break;
                case CC_FILT_Q:
                    filt_res = scaled;
                    PRINTF("  q: %f\n", filt_res);
                    break;
                case CC_FILT_DRIVE:
                    filt_drive = scaled;
                    PRINTF("  drive: %f\n", filt_drive);
                    break;
                case CC_ENV_FILT_AMT:
                    filter_envelope_amount = scaled;
                    PRINTF("  filt env amt %f\n", filter_envelope_amount);
                    break;
                case CC_ENV_FILT_A:
                    filter_attack = scaled;
                    PRINTF("  filt a: %f\n", filter_attack);
                    break;
                case CC_ENV_FILT_D:
                    filter_decay = ol::core::scale(val, 0, 127, 0, 1, 3);
                    PRINTF("  filt d: %f\n", filter_decay);
                    break;
                case CC_ENV_FILT_S:
                    filter_sustain = scaled;
                    PRINTF("  filt s: %f\n", filter_sustain);
                    break;
                case CC_ENV_FILT_R:
                    filter_release = scaled;
                    PRINTF("  filt r: %f\n", filter_release);
                    break;
                case CC_ENV_AMP_A:
                    amp_attack = scaled;
                    PRINTF("  amp a: %f\n", amp_attack);
                    break;
                case CC_ENV_AMP_D:
                    amp_decay = scaled;
                    PRINTF("  amp d: %f\n", amp_decay);
                    break;
                case CC_ENV_AMP_S:
                    amp_sustain = scaled;
                    PRINTF("  amp s: %f\n", amp_sustain);
                    break;
                case CC_ENV_AMP_R:
                    amp_release = scaled;
                    PRINTF("  amp r: %f\n", amp_release);
                    break;
                case CC_OSC_1_VOLUME:
                    osc_1_mix = scaled;
                    PRINTF("  osc 1 mix: %f\n", osc_1_mix);
                    break;
                default:
                    update = false;
                    break;
            }
            if (update) {
                Update();
            }
        }

        void GateOn() override {
            gate = true;
            sound_source_->GateOn();
        }


        void GateOff() override {
            gate = false;
            sound_source_->GateOff();
        }

        bool Gate() override {
            return gate;
        }

        void NoteOn(uint8_t midi_note, uint8_t velocity) override {
            GateOn();
            playing = midi_note;
            freq_ = daisysp::mtof(midi_note);
            amp_envelope_->Retrigger(true);
            filter_envelope_->Retrigger(true);
        }

        void NoteOff(uint8_t midi_note, uint8_t velocity) override {
            GateOff();
            playing = 0;
        }

        uint8_t Playing() override {
            return playing;
        }

    private:

        bool initialized = false;

        t_sample portamento_htime = 0;

        t_sample master_volume = 0.8f;

        t_sample sample_rate = 0;

        // Oscillator, sample player, etc.
        SoundSource *sound_source_;

        t_sample freq_ = 0;

        // Filter
        daisysp::Svf *filters_[CHANNEL_COUNT] = {};
        daisysp::Adsr *filter_envelope_;

        // Amplifier
        daisysp::Adsr *amp_envelope_;

        // Portamento
        daisysp::Port *portamento_;

        // Filter
        t_sample filt_freq = 0;
        t_sample filt_res = 0;
        t_sample filt_drive = 0;

        t_sample filter_attack = 0;
        t_sample filter_decay = 0.2f;
        t_sample filter_sustain = 0;
        t_sample filter_release = 0;
        t_sample filter_envelope_amount = 1;

        // Amp envelope
        t_sample amp_attack = 0.01f; // a little lag on attack and release help reduce clicking
        t_sample amp_decay = 0;
        t_sample amp_sustain = 1;
        t_sample amp_release = 0.01f;

        // Oscillator mixer
        t_sample osc_1_mix = 0.8f;
        uint8_t playing = 0;
        bool gate = false;
    };
}
#endif //OL_DSP_SYNTHVOICE_H