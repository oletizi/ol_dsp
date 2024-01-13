//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_SYNTHAPP_H
#define OL_DSP_SYNTHAPP_H

#include "guilib/ol_guilib_core.h"
#include "ctllib/ol_ctllib.h"

namespace ol::gui {
    using namespace ol::ctl;

    struct SynthAppConfig {
        Dimension viewport;
        Control filter_cutoff;
        Control filter_resonance;
        Control filter_env_amt;
        Control filter_drive;

        Control filter_attack;
        Control filter_decay;
        Control filter_sustain;
        Control filter_release;
    };

    class AdsrView : public Component {
    public:

        AdsrView(Control &attack, Control &decay, Control &sustain, Control &release) :
                attack_(attack), decay_(decay), sustain_(sustain), release_(release) {}

        void Resized() override {

        }

        void Paint(Graphics &g) override {
            auto segment_width = GetWidth() / 4;
            auto attack_endX = segment_width * attack_.scaledValue();
            auto decay_endX = (segment_width * decay_.scaledValue()) + attack_endX;
            auto sustainY = GetHeight() - (GetHeight() * sustain_.scaledValue());
            auto sustain_endX = segment_width * 3;
            auto release_endX = sustain_endX + (segment_width * release_.scaledValue());

            // attack
            g.DrawLine(0, GetHeight(), attack_endX, 0, 1);

            // decay
            g.DrawLine(attack_endX, 0, decay_endX, sustainY, 1);

            // sustain
            g.DrawLine(decay_endX, sustainY, sustain_endX, sustainY, 1);

            // release
            g.DrawLine(sustain_endX, sustainY, release_endX, GetHeight(), 1);
        }

    private:
        Control &attack_;
        Control &decay_;
        Control &sustain_;
        Control &release_;
    };

    class AdsrScreen : public Component {
    public:
        explicit AdsrScreen(AdsrView &adsr_view) {
            layout_.Add(&adsr_view);
        }

        void Resized() override {
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

    private:
        Layout layout_;
    };

    class MeterScreen : public Component {
    public:
        explicit MeterScreen(SynthAppConfig &config)
                : m_filter_cutoff_(ControlMeter(config.filter_cutoff)),
                  m_filter_resonance_(ControlMeter(config.filter_resonance)),
                  m_filter_env_amt_(ControlMeter(config.filter_env_amt)),
                  m_filter_drive_(ControlMeter(config.filter_drive)),
                  m_filter_attack_(ControlMeter(config.filter_attack)),
                  m_filter_decay_(ControlMeter(config.filter_decay)),
                  m_filter_sustain_(ControlMeter(config.filter_sustain)),
                  m_filter_release_(ControlMeter(config.filter_release)) {
            column1_.Add(&m_filter_cutoff_);
            column1_.Add(&m_filter_resonance_);
            column1_.Add(&m_filter_env_amt_);
            column1_.Add(&m_filter_drive_);

            column2_.Add(&m_filter_attack_);
            column2_.Add(&m_filter_decay_);
            column2_.Add(&m_filter_sustain_);
            column2_.Add(&m_filter_release_);

            layout_.SetDirection(Horizontal);
            layout_.Add(&column1_);
            layout_.Add(&column2_);
            layout_.Add(&column3_);
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

        void Resized() override {
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

    private:
        ControlMeter m_filter_cutoff_;
        ControlMeter m_filter_resonance_;
        ControlMeter m_filter_env_amt_;
        ControlMeter m_filter_drive_;
        ControlMeter m_filter_attack_;
        ControlMeter m_filter_decay_;
        ControlMeter m_filter_sustain_;
        ControlMeter m_filter_release_;
        std::vector<ControlMeter *> meters_{
                &m_filter_cutoff_,
                &m_filter_resonance_,
                &m_filter_drive_,
                &m_filter_env_amt_,
                &m_filter_attack_,
                &m_filter_decay_,
                &m_filter_sustain_,
                &m_filter_release_
        };
        Layout column1_;
        Layout column2_;
        Layout column3_;
        Layout layout_;
    };

    class SynthApp : public Component {
    public:
        explicit SynthApp(SynthAppConfig &config) : meter_screen_(MeterScreen(config)) {
            adsr_view_ = new AdsrView(config.filter_attack, config.filter_decay, config.filter_sustain,
                                      config.filter_release);
            adsr_screen_ = new AdsrScreen(*adsr_view_);

            layout_ = Layout(Horizontal);
            layout_.SetSize(config.viewport.width, config.viewport.height);
//            layout_.add(&meter_screen_);
            layout_.Add(adsr_screen_);
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

        void Resized() override {
            layout_.Resized();
        }


        void ControlChange(Control &control) {
            //if (control.)
            switch (control.controller) {
                case CC_FILTER_CUTOFF:
                case CC_FILTER_RESONANCE:
                case CC_ENV_FILT_AMT:
                case CC_FILTER_DRIVE:
                    showFilterScreen();
                    break;
                case CC_ENV_FILT_A:
                case CC_ENV_FILT_D:
                case CC_ENV_FILT_S:
                case CC_ENV_FILT_R:
                    showFilterEnvelopeScreen();
                    break;
                default:
                    break;
            }
        }

    private:

        void showFilterScreen() {
            setScreen(&meter_screen_);
        }

        void showFilterEnvelopeScreen() {
            setScreen(adsr_screen_);
        }

        void setScreen(Component *c) {
            layout_.Clear();
            layout_.Add(c);
        }


        AdsrView *adsr_view_;
        AdsrScreen *adsr_screen_;
        MeterScreen meter_screen_;
        Layout layout_;

    };
}

#endif //OL_DSP_SYNTHAPP_H
