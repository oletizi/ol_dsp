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
#include "SynthConfig.h"
#include "SynthApp.h"

namespace ol::app::synth {

    using namespace ol::ctl;
    using namespace ol::gui;

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

    class MenuItem : public Component {
    public:
        explicit MenuItem(std::string text, bool active) : text_(std::move(text)), active_(active) {}

        void Resized() override {
            area_.dimension.width = GetWidth();
            area_.dimension.height = GetHeight();
        }

        void Paint(Graphics &g) override {
            g.Print(text_, area_);
        }

        void SetActive(bool active) {
            active_ = active;
        }

    private:
        std::string text_;
        bool active_;
        Rectangle area_ = Rectangle{
                Point{0, 0},
                Dimension{100, 16}
        };
    };

    class MainMenu : public Box {
    public:
        explicit MainMenu(const std::vector<MenuItem *> &menu_items) : Box(&layout_) {
            layout_.SetHorizontal();
            for (auto m: menu_items) {
                menu_items_.push_back(m);
                layout_.Add(m);
            }
        }

//        void Resized() override {
//            layout_.SetSize(GetWidth(), GetHeight());
//            layout_.Resized();
//        }
//
//        void Paint(Graphics &g) override {
//            layout_.Paint(g);
//        }


    private:
        std::vector<MenuItem *> menu_items_{};
        Layout layout_ = Layout();
    };

    class SynthMediumGui : public Component, public ControlListener {
    public:
        explicit SynthMediumGui(SynthConfig &config) : config_(config) {
            // XXX: Make this a function of the font.
            main_menu_->SetMarginTop(10)->SetMarginBottom(10);
            main_menu_->SetFixedHeight(16 + main_menu_->GetOffsetVertical());
            main_menu_->SetMarginLeft(10)->SetMarginRight(10);
            layout_.SetVertical();
            auto filter_view_box = new Box(filter_view_);
            filter_view_box->SetMargin(10);
            layout_.Add(filter_view_box);

            auto filter_adsr_view_box_ = new Box(filter_adsr_view_);
            filter_adsr_view_box_->SetMargin(10);
            layout_.Add(filter_adsr_view_box_);
            layout_.Add(main_menu_);
        }

        void Resized() override {
            DPRINTF("gui resized: menu fixed height: %d", main_menu_->GetFixedHeight());
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

        void UpdateFilterCutoff(Control control) override {

        }

        void UpdateFilterResonance(Control control) override {

        }

        void UpdateFilterDrive(Control control) override {

        }

        void UpdateFilterEnvAmount(Control control) override {

        }

        void UpdateFilterAttack(Control control) override {

        }

        void UpdateFilterDecay(Control control) override {

        }

        void UpdateFilterSustain(Control control) override {

        }

        void UpdateFilterRelease(Control control) override {

        }

        void UpdateAmpVolume(Control control) override {

        }

        void UpdateAmpAttack(Control control) override {

        }

        void UpdateAmpDecay(Control control) override {

        }

        void UpdateAmpSustain(Control control) override {

        }

        void UpdateAmpRelease(Control control) override {

        }

    private:
        SynthConfig &config_;
        FilterView *filter_view_ = new FilterView(config_.filter_cutoff, config_.filter_resonance,
                                                  config_.filter_env_amt,
                                                  config_.filter_drive);
        AdsrView *filter_adsr_view_ = new AdsrView(config_.filter_attack, config_.filter_decay, config_.filter_sustain,
                                                   config_.filter_release, config_.filter_env_amt);
        std::vector<MenuItem *> menu_items_ = {
                new MenuItem("Main", true),
                new MenuItem("Filter", false),
                new MenuItem("Amp", false),
                new MenuItem("Fx", false),
                new MenuItem("Mod", false)
        };
        MainMenu *main_menu_ = new MainMenu(menu_items_);

        Layout layout_ = Layout();
    };

    class SynthTinyGui : public Component, public ControlListener {
    public:
        explicit SynthTinyGui(SynthConfig &config) : config_(config) {
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
            g.DrawRect(Rectangle{0, 0, config_.viewport});
            layout_.Paint(g);
        }

        void Resized() override {
            layout_.Resized();
        }

        //
        // ControlListener interface
        //

        void UpdateFilterCutoff(Control control) override {
            filter_screen_->SetTitle("Filter: Cutoff");
            setScreen(filter_screen_);
        }

        void UpdateFilterResonance(Control control) override {
            filter_screen_->SetTitle("Filter: Resonance");
            setScreen(filter_screen_);
        }

        void UpdateFilterDrive(Control control) override {
            filter_screen_->SetTitle("Filter: Drive");
            setScreen(filter_screen_);
        }

        void UpdateFilterEnvAmount(Control control) override {
            filter_adsr_screen_->SetTitle("Filter: Env Amt");
            setScreen(filter_adsr_screen_);
        }

        void UpdateFilterAttack(Control control) override {
            filter_adsr_screen_->SetTitle("Filter: Attack");
            setScreen(filter_adsr_screen_);
        }

        void UpdateFilterDecay(Control control) override {
            filter_adsr_screen_->SetTitle("Filter: Decay");
            setScreen(filter_adsr_screen_);
        }

        void UpdateFilterSustain(Control control) override {
            filter_adsr_screen_->SetTitle("Filter: Sustain");
            setScreen(filter_adsr_screen_);
        }

        void UpdateFilterRelease(Control control) override {
            filter_adsr_screen_->SetTitle("Filter: Rel");
            setScreen(filter_adsr_screen_);
        }

        void UpdateAmpVolume(Control control) override {
            amp_screen_->SetTitle("Amp: Vol");
            setScreen(amp_screen_);
        }

        void UpdateAmpAttack(Control control) override {
            amp_screen_->SetTitle("Amp: Attack");
            setScreen(amp_screen_);
        }

        void UpdateAmpDecay(Control control) override {
            amp_screen_->SetTitle("Amp: Decay");
            setScreen(amp_screen_);
        }

        void UpdateAmpSustain(Control control) override {
            amp_screen_->SetTitle("Amp: Sustain");
            setScreen(amp_screen_);
        }

        void UpdateAmpRelease(Control control) override {
            amp_screen_->SetTitle("Amp: Release");
            setScreen(amp_screen_);
        }

    private:
        void setScreen(Component *c) {
            layout_.Clear();
            layout_.Add(c);
        }

        SynthConfig &config_;
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
