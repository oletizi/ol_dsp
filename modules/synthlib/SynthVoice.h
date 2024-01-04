//
// Created by Orion Letizi on 12/5/23.
//

#ifndef OL_DSP_SYNTHVOICE_H
#define OL_DSP_SYNTHVOICE_H

#include "Voice.h"
#include "Portamento.h"
#include "Adsr.h"
#include "Filter.h"

#define PRINTF printf

namespace ol::synth {
    template<int CHANNEL_COUNT>
    class SynthVoice : public Voice {
    private:
        //t_sample frame_buffer[CHANNEL_COUNT]{};
    public:
        SynthVoice(SoundSource<CHANNEL_COUNT> *sound_source,
                   Filter *filter,
                   Adsr *filter_envelope,
                   Adsr *amp_envelope,
                   Portamento *portamento)
                : sound_source_(sound_source),
                  filter_(filter),
                  filter_envelope_(filter_envelope),
                  amp_envelope_(amp_envelope),
                  portamento_(portamento) {}

        SynthVoice() : SynthVoice<CHANNEL_COUNT>(new OscillatorSoundSource<CHANNEL_COUNT>(),
                                                 new SvfFilter<CHANNEL_COUNT>(),
                                                 new DaisyAdsr(), new DaisyAdsr(),
                                                 new DaisyPortamento()) {}

        void Init(t_sample sr) override {
            sample_rate = sr;
            sound_source_->Init(sr);
            filter_->Init(sr);
            filter_envelope_->Init(sample_rate, 1);
            amp_envelope_->Init(sample_rate, 1);
            portamento_->Init(sample_rate, portamento_htime);
            initialized = true;
        }

        void Process(t_sample *frame_out) override {
            t_sample frame_buffer[CHANNEL_COUNT]{};
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_buffer[i] = 0;
            }
            sound_source_->SetFreq(portamento_->Process(freq_));
            sound_source_->Process(frame_buffer);

            t_sample filter_frequency =
                    filter_cutoff + ((filter_envelope_->Process(Gate()) * 20000) * filter_env_amount);
            t_sample amp = amp_envelope_->Process(Gate());
            filter_->SetFreq(filter_frequency);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_buffer[i] *= osc_1_mix;
            }
            filter_->Process(frame_buffer);
            filter_->Low(frame_buffer);
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_out[i] = amp * amp_env_amount * frame_buffer[i];
            }
        }

        void UpdateConfig(Config &config) override {

            filter_cutoff = config.filter_cutoff;
            filter_resonance = config.filter_resonance;
            filter_drive = config.filter_drive;

            filter_attack = config.filter_attack;
            filter_attack_shape = config.filter_attack_shape;
            filter_decay = config.filter_decay;
            filter_sustain = config.filter_sustain;
            filter_release = config.filter_release;
            filter_env_amount = config.filter_env_amount;

            amp_attack = config.amp_attack;
            amp_attack_shape = config.amp_attack_shape;
            amp_decay = config.amp_decay;
            amp_sustain = config.amp_sustain;
            amp_release = config.amp_release;
            amp_env_amount = config.amp_env_amount;

            portamento_htime = config.portamento;
            Update();
        }

        void Update() override {

            //filter_->SetFreq(filter_cutoff);
            filter_->SetRes(filter_resonance);
            filter_->SetDrive(filter_drive);

            filter_envelope_->SetAttackTime(filter_attack, filter_attack_shape);
            filter_envelope_->SetDecayTime(filter_decay);
            filter_envelope_->SetSustainLevel(filter_sustain);
            filter_envelope_->SetReleaseTime(filter_release);

            // Amplifier
            amp_envelope_->SetAttackTime(amp_attack, amp_attack_shape);
            amp_envelope_->SetDecayTime(amp_decay);
            amp_envelope_->SetSustainLevel(amp_sustain);
            amp_envelope_->SetReleaseTime(amp_release);

            // Portamento
            portamento_->SetHtime(portamento_htime);
        }


        void UpdateHardwareControl(uint8_t controller, t_sample value) override {
            bool update = true;
            switch (controller) {
                case CC_CTL_VOLUME:
                    amp_env_amount = value;
                    break;
                case CC_CTL_PORTAMENTO:
                    portamento_htime = ol::core::scale(value, 0, 1, 0, 1, 4);
                    break;
                case CC_FILTER_CUTOFF:
                    filter_cutoff = ol::core::scale(value, 0, 1, 0, 20000, 2.5);
                    break;
                case CC_FILTER_RESONANCE:
                    filter_resonance = value;
                    break;
                case CC_FILTER_DRIVE:
                    filter_drive = value;
                    break;
                case CC_ENV_FILT_AMT:
                    filter_env_amount = value;
                    break;
                case CC_ENV_FILT_A:
                    filter_attack = value;
                    break;
                case CC_ENV_FILT_D:
                    filter_decay = ol::core::scale(value, 0, 1, 0, 1, 3);
                    break;
                case CC_ENV_FILT_S:
                    filter_sustain = value;
                    break;
                case CC_ENV_FILT_R:
                    filter_release = value;
                    break;
                case CC_ENV_AMP_A:
                    amp_attack = value;
                    break;
                case CC_ENV_AMP_D:
                    amp_decay = value;
                    break;
                case CC_ENV_AMP_S:
                    amp_sustain = value;
                    break;
                case CC_ENV_AMP_R:
                    amp_release = value;
                    break;
                case CC_OSC_1_VOLUME:
                    osc_1_mix = value;
                    break;
                default:
                    update = false;
                    break;
            }
            if (update) {
                Update();
            }
        }

        void UpdateMidiControl(uint8_t ctl, uint8_t val) override {
            bool update = true;
            t_sample scaled = ol::core::scale(val, 0, 127, 0, 1, 1);
            switch (ctl) {
                case CC_CTL_VOLUME:
                    amp_env_amount = scaled;
                    PRINTF("  volume: %f\n", amp_env_amount);
                    break;
                case CC_CTL_PORTAMENTO:
                    portamento_htime = ol::core::scale(val, 0, 127, 0, 1, 4);
                    PRINTF("  port: %f\n", portamento_htime);
                    break;
                case CC_FILTER_CUTOFF:
                    filter_cutoff = ol::core::scale(val, 0, 127, 0, 20000, 2.5);
                    PRINTF("  Cutoff: %d\n", int(filter_cutoff));
                    break;
                case CC_FILTER_RESONANCE:
                    filter_resonance = scaled;
                    PRINTF("  q: %f\n", filter_resonance);
                    break;
                case CC_FILTER_DRIVE:
                    filter_drive = scaled;
                    PRINTF("  drive: %f\n", filter_drive);
                    break;
                case CC_ENV_FILT_AMT:
                    filter_env_amount = scaled;
                    PRINTF("  filt env amt %f\n", filter_env_amount);
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

        void SetFrequency(t_sample freq) override {
            freq_ = freq;
            //sound_source_->SetFreq(freq);
        }

    private:
        // Init state
        bool initialized = false;
        t_sample sample_rate = 0;

        // Oscillator, sample player, etc.
        SoundSource<CHANNEL_COUNT> *sound_source_{};

        // Oscillator/sound source params
        t_sample freq_ = 0;
        t_sample osc_1_mix = 0.8f;

        // Filter
        //Filter *filters_[CHANNEL_COUNT] = {};
        Filter *filter_ = nullptr;
        ol::synth::Adsr *filter_envelope_{};

        // Filter parameters
        t_sample filter_cutoff = 0;
        t_sample filter_resonance = 0;
        t_sample filter_drive = 0;

        t_sample filter_attack = 0;
        t_sample filter_attack_shape = 1;
        t_sample filter_decay = 0.2f;
        t_sample filter_sustain = 0;
        t_sample filter_release = 0;
        t_sample filter_env_amount = 1;

        // Amplifier
        ol::synth::Adsr *amp_envelope_{};

        // Amplifier parameters
        t_sample amp_attack = 0.01f; // a little lag on attack and release help reduce clicking
        t_sample amp_attack_shape = 1;
        t_sample amp_decay = 0;
        t_sample amp_sustain = 1;
        t_sample amp_release = 0.01f;
        t_sample amp_env_amount = 0.8f; // AKA voice volume

        // Portamento
        Portamento *portamento_{};

        // Portamento parameters
        t_sample portamento_htime = 0;

        // Gate/note on status
        uint8_t playing = 0;
        bool gate = false;
    };
}
#endif //OL_DSP_SYNTHVOICE_H