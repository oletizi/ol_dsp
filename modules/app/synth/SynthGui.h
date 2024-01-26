//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_SYNTHGUI_H
#define OL_DSP_SYNTHGUI_H

#include <string>
#include <utility>
#include "spline/spline.h"
#include "guilib/ol_guilib_core.h"
#include "ctllib/ol_ctllib.h"
//#include "app/synth/SynthEngine.h"

namespace ol::app::synth {

    using namespace ol::ctl;
    using namespace ol::gui;


    struct SynthGuiConfig {
        ol::gui::Dimension viewport{128, 64};

        Control filter_cutoff = Control(CC_FILTER_CUTOFF, 0.5f);
        Control filter_resonance = Control(CC_FILTER_RESONANCE, 0.3f);
        Control filter_drive = Control(CC_FILTER_DRIVE, 0.1f);

        Control filter_env_amt = Control(CC_ENV_FILT_AMT, 0.25f);
        Control filter_attack = Control(CC_ENV_FILT_A, 0.f);
        Control filter_decay = Control(CC_ENV_FILT_D, 0.8f);
        Control filter_sustain = Control(CC_ENV_FILT_S, 0.f);
        Control filter_release = Control(CC_ENV_FILT_R, 0.2f);

        Control amp_env_amt = Control(CC_CTL_VOLUME, 1.f);
        Control amp_attack = Control(CC_ENV_AMP_A, 0.f);
        Control amp_decay = Control(CC_ENV_AMP_D, 0.f);
        Control amp_sustain = Control(CC_ENV_AMP_S, 1.f);
        Control amp_release = Control(CC_ENV_AMP_R, 0.f);
    };

    class AdsrView : public Component {
    public:

        AdsrView(Control &attack, Control &decay, Control &sustain, Control &release, Control &amount) :
                attack_(attack), decay_(decay), sustain_(sustain), release_(release), amount_(amount) {}

        void Resized() override {}

        void Paint(Graphics &g) override {
            auto amount = amount_.GetFloatValue();
            auto height = GetHeight();
            auto segment_width = double(GetWidth()) / 4;
            auto attack_endX = segment_width * attack_.GetFloatValue();
            auto decay_endX = (segment_width * decay_.GetFloatValue()) + attack_endX;

            auto decayY = height - (height * amount);

            auto sustainY = height - (amount * (double(height) - (double(height) * (1 - sustain_.GetFloatValue()))));
            auto sustain_endX = segment_width * 3;
            auto release_endX = sustain_endX + (segment_width * release_.GetFloatValue());
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

    class FilterView : public Component {
    public:
        FilterView(Control &cutoff, Control &resonance, Control &env_amt, Control &drive)
                : cutoff_(cutoff), resonance_(resonance), env_amt_(env_amt), drive_(drive) {}

        void Resized() override {}

        void Paint(Graphics &g) override {
            auto startX = 0;
            auto startY = GetHeight() - (GetHeight() * 0.5); //GetHeight();

            auto cutoffX = ol::core::scale(cutoff_.GetFloatValue(), 0, 1, 0, GetWidth(), 1);
            auto preCutoffX = cutoffX - GetWidth() / 10;
            auto cutoffY = startY - ol::core::scale(resonance_.GetFloatValue(), 0, 1, 0, GetHeight() / 4, 1);


            auto endX = cutoffX + (GetWidth() / 8);
            auto endY = GetHeight();

            g.DrawLine(startX, startY, preCutoffX, startY, 1);
            g.DrawLine(preCutoffX, startY, cutoffX, cutoffY, 1);
            g.DrawLine(cutoffX, cutoffY, endX, endY, 1);

            double drive_scale = 10;
            for (int i = 0; i < int((drive_.GetFloatValue() * drive_scale)); i++) {
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

    class AppScreen : public Component {

    public:
        AppScreen(Component *component, std::string title_text) {
            SetTitle(std::move(title_text));
            layout_.Add(component);
            layout_.Add(&title_);
        }

        void Resized() override {
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

        void SetTitle(std::string title) {
            title_.SetText(std::move(title));
        }


    private:
        Layout layout_;
        Text title_ = Text(font_, "");
        Font font_ = Font(16);
    };

    class SynthGui : public Component {
    public:
        explicit SynthGui(ol::app::synth::SynthGuiConfig &config) : config_(config) {
            filter_view_ = new FilterView(config.filter_cutoff, config.filter_resonance, config.filter_env_amt,
                                          config.filter_drive);
            filter_adsr_view_ = new AdsrView(config.filter_attack, config.filter_decay, config.filter_sustain,
                                             config.filter_release, config.filter_env_amt);
            auto screen_layout = new Layout();
            screen_layout->Add(filter_view_);
            screen_layout->Add(filter_adsr_view_);
            filter_screen_ = new AppScreen(screen_layout, "Filter");
            filter_adsr_screen_ = filter_screen_;

            amp_adsr_view_ = new AdsrView(config.amp_attack, config.amp_decay, config.amp_sustain, config.amp_release,
                                          config.amp_env_amt);
            amp_screen_ = new AppScreen(amp_adsr_view_, "Amp");


            layout_ = Layout(Horizontal);
            layout_.SetSize(config.viewport.width, config.viewport.height);
            layout_.Add(filter_screen_);
        }

        void Paint(Graphics &g) override {
//            DPRINTLN("SynthGui: Paint()...");
            g.DrawRect(Rectangle{0, 0, config_.viewport});
            layout_.Paint(g);
//            DPRINTLN("SynthGui: Paint().");
        }

        void Resized() override {
            layout_.Resized();
        }

        void ControlChange(Control &control) {
            DPRINTF("SynthGui.ControlChanve: ctl: %d, val: %d\n", control.GetController(), control.GetAdcValue());

            switch (control.GetController()) {
                case CC_FILTER_CUTOFF:
                    filter_screen_->SetTitle("Filter: Cutoff");
                    setScreen(filter_screen_);
                    break;
                case CC_FILTER_RESONANCE:
                    filter_screen_->SetTitle("Filter: Resonance");
                    setScreen(filter_screen_);
                    break;
                case CC_FILTER_DRIVE:
                    filter_screen_->SetTitle("Filter: Drive");
                    setScreen(filter_screen_);
                    break;
                case CC_ENV_FILT_AMT:
                    filter_adsr_screen_->SetTitle("Filter: Env Amt");
                    setScreen(filter_adsr_screen_);
                    break;
                case CC_ENV_FILT_A:
                    filter_adsr_screen_->SetTitle("Filter: Attack");
                    setScreen(filter_adsr_screen_);
                    break;
                case CC_ENV_FILT_D:
                    filter_adsr_screen_->SetTitle("Filter: Decay");
                    setScreen(filter_adsr_screen_);
                    break;
                case CC_ENV_FILT_S:
                    filter_adsr_screen_->SetTitle("Filter: Sustain");
                    setScreen(filter_adsr_screen_);
                    break;
                case CC_ENV_FILT_R:
                    filter_adsr_screen_->SetTitle("Filter: Rel");
                    setScreen(filter_adsr_screen_);
                    break;
                case CC_CTL_VOLUME:
                    amp_screen_->SetTitle("Amp: Vol");
                    setScreen(amp_screen_);
                    break;
                case CC_ENV_AMP_A:
                    amp_screen_->SetTitle("Amp: Attack");
                    setScreen(amp_screen_);
                    break;
                case CC_ENV_AMP_D:
                    amp_screen_->SetTitle("Amp: Decay");
                    setScreen(amp_screen_);
                    break;
                case CC_ENV_AMP_S:
                    amp_screen_->SetTitle("Amp: Sustain");
                    setScreen(amp_screen_);
                    break;
                case CC_ENV_AMP_R:
                    amp_screen_->SetTitle("Amp: Release");
                    setScreen(amp_screen_);
                    break;
                default:
                    break;
            }
        }

    private:
        void setScreen(Component *c) {
            layout_.Clear();
            layout_.Add(c);
        }

        SynthGuiConfig &config_;
        FilterView *filter_view_;
        AdsrView *filter_adsr_view_;
        AppScreen *filter_screen_;
        AppScreen *filter_adsr_screen_;
        AdsrView *amp_adsr_view_;
        AppScreen *amp_screen_;
        Layout layout_;

    };

}

#endif //OL_DSP_SYNTHGUI_H
