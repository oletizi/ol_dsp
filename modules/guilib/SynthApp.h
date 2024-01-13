//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_SYNTHAPP_H
#define OL_DSP_SYNTHAPP_H

#include <string>
#include "spline.h"
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

        AdsrView(Control &attack, Control &decay, Control &sustain, Control &release, Control &amount) :
                attack_(attack), decay_(decay), sustain_(sustain), release_(release), amount_(amount) {}

        void Resized() override {}

        void Paint(Graphics &g) override {
            auto amount = amount_.scaledValue();
            auto height = GetHeight();
            auto segment_width = double(GetWidth()) / 4;
            auto attack_endX = segment_width * attack_.scaledValue();
            auto decay_endX = (segment_width * decay_.scaledValue()) + attack_endX;

            auto decayY = height - (height * amount);

            auto sustainY = height - (amount * (double(height) - (double(height) * (1 - sustain_.scaledValue()))));
            auto sustain_endX = segment_width * 3;
            auto release_endX = sustain_endX + (segment_width * release_.scaledValue());

            // attack
            g.DrawLine(0, height, int(attack_endX), decayY, 1);

            // decay
            g.DrawLine(int(attack_endX), decayY, int(decay_endX), int(sustainY), 1);

            // sustain
            g.DrawLine(int(decay_endX), int(sustainY), int(sustain_endX), int(sustainY), 1);

            // release
            g.DrawLine(int(sustain_endX), int(sustainY), int(release_endX), height, 1);
        }

    private:
        Control &attack_;
        Control &decay_;
        Control &sustain_;
        Control &release_;
        Control &amount_;
    };

    /**
     * XXX: This is an interesting idea, but currently a mess
     */
    class FilterSplineView : public Component {
    public:
        FilterSplineView(Control &cutoff, Control &resonance, Control &env_amt, Control &drive)
                : cutoff_(cutoff), resonance_(resonance), env_amt_(env_amt), drive_(drive) {}

        void Resized() override {}

        void Paint(Graphics &g) override {
            double startX = 0;
            double startY = GetHeight() - (GetHeight() * 0.66);
            double cutoffX = ol::core::scale(cutoff_.scaledValue(), 0, 1, 0, t_sample(GetWidth()), 1);
            double endX = cutoffX + (double(GetWidth()) / 4);
            double endY = GetHeight();
            auto resonance = resonance_.scaledValue();
            std::vector<double> X{0, cutoffX - (double(GetWidth()) / 2), cutoffX, endX};
            std::vector<double> Y{startY, startY, startY - (resonance * 50), endY};
            tk::spline s(X, Y);
            for (int x = int(cutoffX - double(GetWidth()) / 2); x < GetWidth(); x++) {
                auto y = s(x);
                y = y > GetHeight() ? GetHeight() : y;
                g.WritePixel(x, int(y), ol::gui::Color::White);
            }
        }

    private:
        Control &cutoff_;
        Control &resonance_;
        Control &env_amt_;
        Control &drive_;
    };

    class FilterView : public Component {
    public:
        FilterView(Control &cutoff, Control &resonance, Control &env_amt, Control &drive)
                : cutoff_(cutoff), resonance_(resonance), env_amt_(env_amt), drive_(drive) {}

        void Resized() override {}

        void Paint(Graphics &g) override {
            auto startX = 0;
            auto startY = GetHeight() - (GetHeight() * 0.5); //GetHeight();

            auto cutoffX = ol::core::scale(cutoff_.scaledValue(), 0, 1, 0, GetWidth(), 1);
            auto preCutoffX = cutoffX - GetWidth() / 10;
            auto cutoffY = startY - ol::core::scale(resonance_.scaledValue(), 0, 1, 0, GetHeight() / 4, 1);


            auto endX = cutoffX + (GetWidth() / 8);
            auto endY = GetHeight();

            g.DrawLine(startX, startY, preCutoffX, startY, 1);
            g.DrawLine(preCutoffX, startY, cutoffX, cutoffY, 1);
            g.DrawLine(cutoffX, cutoffY, endX, endY, 1);

            double drive_scale = 10;
            for (int i = 0; i < int((drive_.scaledValue() * drive_scale)); i++) {
                g.DrawLine(preCutoffX, startY, cutoffX, cutoffY - i, 1);
                g.DrawLine(cutoffX, cutoffY - i, endX, endY, 1);
            }


        }

    private:
        Control &cutoff_;
        Control &resonance_;
        Control &env_amt_;
        Control &drive_;
    };

    class FilterScreen : public Component {
    public:
        explicit FilterScreen(FilterView &filter_view, AdsrView &adsr_view) {
            layout_.Add(&filter_view);
//            layout_.Add(&adsr_view);
            layout_.Add(&title_);
        }

        void Resized() override {
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

    private:
        Font font_ = Font(16);
        Text title_ = Text(font_, (std::string &) "Filter");
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
        explicit SynthApp(SynthAppConfig &config) : config_(config) {
            filter_view_ = new FilterView(config.filter_cutoff, config.filter_resonance, config.filter_env_amt,
                                          config.filter_drive);
            adsr_view_ = new AdsrView(config.filter_attack, config.filter_decay, config.filter_sustain,
                                      config.filter_release, config.filter_env_amt);
            filter_screen_ = new FilterScreen(*filter_view_, *adsr_view_);

            layout_ = Layout(Horizontal);
            layout_.SetSize(config.viewport.width, config.viewport.height);
            layout_.Add(filter_screen_);
        }

        void Paint(Graphics &g) override {
            g.DrawRect(Rectangle{0, 0, config_.viewport});
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
                case CC_ENV_FILT_A:
                case CC_ENV_FILT_D:
                case CC_ENV_FILT_S:
                case CC_ENV_FILT_R:
                    showFilterScreen();
                    break;
                default:
                    break;
            }
        }

    private:

        void showFilterScreen() {
            setScreen(filter_screen_);
        }

        void setScreen(Component *c) {
            layout_.Clear();
            layout_.Add(c);
        }

        SynthAppConfig &config_;
        FilterView *filter_view_;
        AdsrView *adsr_view_;
        FilterScreen *filter_screen_;
        Layout layout_;

    };
}

#endif //OL_DSP_SYNTHAPP_H
