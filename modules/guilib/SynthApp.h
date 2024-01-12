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
                attack_(attack), decay_(decay), sustain(sustain), release(release) {}

        void resized() override {

        }

        void paint(Graphics &g) override {
            auto segment_width = getWidth() / 4;
            auto attack_endX = segment_width * attack_.scaledValue();
            auto decay_endX = (segment_width * decay_.scaledValue()) + attack_endX;
            auto sustainY = getHeight() - (getHeight() * sustain.scaledValue());
            auto sustain_endX = segment_width * 3;
            auto release_endX = sustain_endX + (segment_width * release.scaledValue());

            // attack
            g.drawLine(0, getHeight(), attack_endX, 0, 1);

            // decay
            g.drawLine(attack_endX, 0, decay_endX, sustainY, 1);

            // sustain
            g.drawLine(decay_endX, sustainY, sustain_endX, sustainY, 1);

            // release
            g.drawLine(sustain_endX, sustainY, release_endX, getHeight(), 1);
        }

    private:
        Control &attack_;
        Control &decay_;
        Control &sustain;
        Control &release;
    };

    class AdsrScreen : public Component {
    public:
        explicit AdsrScreen(AdsrView &adsr_view) {
            layout_.add(&adsr_view);
        }

        void resized() override {
            layout_.setSize(getWidth(), getHeight());
            layout_.resized();
        }

        void paint(Graphics &g) override {
            layout_.paint(g);
        }

    private:
        Layout layout_;
    };

    class MeterScreen : public Component {
    public:
        explicit MeterScreen(SynthAppConfig &config)
                : m_filter_cutoff(ControlMeter(config.filter_cutoff)),
                  m_filter_resonance(ControlMeter(config.filter_resonance)),
                  m_filter_env_amt(ControlMeter(config.filter_env_amt)),
                  m_filter_drive(ControlMeter(config.filter_drive)),
                  m_filter_attack(ControlMeter(config.filter_attack)),
                  m_filter_decay(ControlMeter(config.filter_decay)),
                  m_filter_sustain(ControlMeter(config.filter_sustain)),
                  m_filter_release(ControlMeter(config.filter_release)) {
            column1.add(&m_filter_cutoff);
            column1.add(&m_filter_resonance);
            column1.add(&m_filter_env_amt);
            column1.add(&m_filter_drive);

            column2.add(&m_filter_attack);
            column2.add(&m_filter_decay);
            column2.add(&m_filter_sustain);
            column2.add(&m_filter_release);

            layout_.setDirection(Horizontal);
            layout_.add(&column1);
            layout_.add(&column2);
            layout_.add(&column3);
        }

        void paint(Graphics &g) override {
            layout_.paint(g);
        }

        void resized() override {
            layout_.setSize(getWidth(), getHeight());
            layout_.resized();
        }

    private:
        ControlMeter m_filter_cutoff;
        ControlMeter m_filter_resonance;
        ControlMeter m_filter_env_amt;
        ControlMeter m_filter_drive;
        ControlMeter m_filter_attack;
        ControlMeter m_filter_decay;
        ControlMeter m_filter_sustain;
        ControlMeter m_filter_release;
        std::vector<ControlMeter *> meters_{
                &m_filter_cutoff,
                &m_filter_resonance,
                &m_filter_drive,
                &m_filter_env_amt,
                &m_filter_attack,
                &m_filter_decay,
                &m_filter_sustain,
                &m_filter_release
        };
        Layout column1;
        Layout column2;
        Layout column3;
        Layout layout_;
    };

    class SynthApp : public Component {
    public:
        explicit SynthApp(SynthAppConfig &config) : meter_screen_(MeterScreen(config)) {
            adsr_view_ = new AdsrView(config.filter_attack, config.filter_decay, config.filter_sustain,
                                      config.filter_release);
            adsr_screen_ = new AdsrScreen(*adsr_view_);

            layout_ = Layout(Horizontal);
            layout_.setSize(config.viewport.width, config.viewport.height);
//            layout_.add(&meter_screen_);
            layout_.add(adsr_screen_);
        }

        void paint(Graphics &g) override {
            layout_.paint(g);
        }

        void resized() override {
            layout_.resized();
        }

    private:
        AdsrView *adsr_view_;
        AdsrScreen *adsr_screen_;
        MeterScreen meter_screen_;
        Layout layout_;

    };
}

#endif //OL_DSP_SYNTHAPP_H
