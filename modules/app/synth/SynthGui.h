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

    class FaderFace : public Component {
    public:
        explicit FaderFace(Control &control) : control_(control) {}

        void Resized() override {

        }

        void Paint(Graphics &g) override {
            int fader_mid = GetHeight() - (control_.GetFloatValue() * GetHeight());
            int fader_height = (GetHeight() / 17) * 4;
            int fader_width = (GetWidth() / 9) * 4;
            int fader_margin = GetWidth() - fader_width;
            g.DrawLine(GetWidth() / 2, 0, GetWidth() / 2, GetHeight(), 1);
            g.FillRect(fader_margin / 2, fader_mid - (fader_height / 2), fader_width, fader_height);
        }

    private:
        Control &control_;
    };

    class Fader : public Component {
    public:

        Fader(Text *label, Control &control) : control_(control), label_(label) {
            layout_.SetVertical();
            layout_.Add(&face_);
            layout_.Add(label_);
        }

        void Resized() override {
            box_.SetSize(GetWidth(), GetHeight());
            box_.Resized();
        }

        void Paint(Graphics &g) override {
            box_.Paint(g);
        }

    private:
        Control &control_;
        Text *label_;
        Layout layout_ = Layout();
        Box box_ = Box(&layout_);
        FaderFace face_ = FaderFace(control_);
    };

    class DialFace : public Component {
    public:
        explicit DialFace(Control &c) : control_(c) {}

        void Resized() override {

        }

        void Paint(Graphics &g) override {
            g.DrawCircle(0, 0, GetHeight() / 2);
        }

    private:
        Control &control_;
    };

    class Dial : public Component {
    public:
        Dial(Text *label, Control &c) : control_(c), label_(label) {
            layout_.SetVertical();
            layout_.Add(&face_);
            layout_.Add(label_);
        }

        void Resized() override {
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

    private:
        Control &control_;
        Text *label_;
        Layout layout_ = Layout();
        Box box = Box(&layout_);
        DialFace face_ = DialFace(control_);
    };

    class MainScreenA : public Component {
    public:
        explicit MainScreenA(TextFactory &text_factory, Control &waveform, Control &level)
                : text_factory_(text_factory), waveform_control_(waveform), level_control_(level) {
            Dimension widget_size{50, 50};
            waveform_dial_->SetFixedSize(widget_size);
            level_fader_->SetFixedSize(widget_size);
            layout_.SetHorizontal();
            layout_.Add(waveform_dial_);
            layout_.Add(level_fader_);
        }

        void Resized() override {
            DPRINTF("Main screen resize: w: %d, h: %d\n", GetWidth(), GetHeight());
            layout_.SetSize(GetWidth(), GetHeight());
            layout_.Resized();
        }

        void Paint(Graphics &g) override {
            layout_.Paint(g);
        }

    private:
        TextFactory &text_factory_;
        Control &waveform_control_;
        Control &level_control_;
        Layout layout_ = Layout();
        Dial *waveform_dial_ = new Dial(text_factory_.NewText("wave"), waveform_control_);
        Fader *level_fader_ = new Fader(text_factory_.NewText("level"), level_control_);
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

    class MenuItem : public Box {
    public:
        explicit MenuItem(Text *text, bool active) : text_(text), active_(active), Box(text) {

        }

        void SetActive(bool active) {
            active_ = active;
            SetBorder(Border{0, 0, 0, active_ ? 1 : 0});
        }

        [[nodiscard]] bool IsActive() const {
            return active_;
        }

    private:
        Text *text_;
        bool active_;
    };

    class MainMenu : public Box {
    public:
        explicit MainMenu(const std::vector<MenuItem *> &menu_items) : Box(&layout_) {
            layout_.SetHorizontal();
            for (auto m: menu_items) {
                m->SetMarginLeft(10)->SetMarginRight(16);
                menu_items_.push_back(m);
                layout_.Add(m);
            }
        }

        void Resized() override {
            SetFixedHeight(24 + GetOffsetVertical());

            Box::Resized();
        }


        void SetActive(int element_id) {
            for (int i = 0; i < menu_items_.size(); i++) {
                menu_items_[i]->SetActive(i == element_id);
            }
        }

    private:
        std::vector<MenuItem *> menu_items_{};
        Layout layout_ = Layout();
    };

    class Carousel {
    public:
        Component *Next() {
            Component *rv = nullptr;
            if (screens_.size() > 0) {
                index++;
                if (index >= screens_.size()) {
                    index = 0;
                }
                rv = screens_[index];
            }
            return rv;
        }

        Carousel *Add(Component *c) {
            screens_.push_back(c);
            return this;
        }

    private:
        std::vector<Component *> screens_{};
        int index = 0;
    };

    class SynthMediumGui : public Component, public ControlListener {
    public:
        explicit SynthMediumGui(SynthConfig &config, TextFactory &text_factory) : config_(config),
                                                                                  text_factory_(text_factory) {

            // Main screens
            auto main_layoutA = new Layout();
            main_layoutA->Add(new MainScreenA(text_factory, config_.osc_1_waveform, config_.osc_1_level));
            main_screens_.Add(main_layoutA);

            auto main_layoutB = new Layout();
            main_layoutB->Add(new Text(font_, "Main screen 2"));
            main_screens_.Add(main_layoutB);

            auto main_layoutC = new Layout();
            main_layoutC->Add(new Text(font_, "Main screen 3"));
            main_screens_.Add(main_layoutC);

            // Filter screens
            auto filter_layoutA = new Layout();
            filter_layoutA->SetVertical();

            auto filter_view_box = new Box(filter_view_);
            filter_view_box->SetMargin(10)->SetPadding(5)->SetBorder(Border{1, 1, 1, 1});
            filter_layoutA->Add(filter_view_box);

            auto filter_adsr_view_box_ = new Box(filter_adsr_view_);
            filter_adsr_view_box_->SetMargin(10)->SetPadding(5)->SetBorder(Border{1, 1, 1, 1});
            filter_layoutA->Add(filter_adsr_view_box_);
            filter_screens_.Add(filter_layoutA);

            auto filter_layoutB = new Layout();
            filter_layoutB->Add(new Text(font_, "Filter screen 2"));
            filter_screens_.Add(filter_layoutB);

            // Amp screens
            auto amp_layoutA = new Layout();
            amp_layoutA->Add(new Text(font_, "Amp screen 1"));
            amp_screens_.Add(amp_layoutA);

            auto amp_layoutB = new Layout();
            amp_layoutB->Add(new Text(font_, "Amp screen 2"));
            amp_screens_.Add(amp_layoutB);

            // Fx screens
            auto fx_layoutA = new Layout();
            fx_layoutA->Add(new Text(font_, "Fx screen 1"));
            fx_screens_.Add(fx_layoutA);

            auto fx_layoutB = new Layout();
            fx_layoutB->Add(new Text(font_, "Fx screen 2"));
            fx_screens_.Add(fx_layoutB);

            // Mod screens
            auto mod_layoutA = new Layout();
            mod_layoutA->Add(new Text(font_, "Mod screen 1"));
            mod_screens_.Add(mod_layoutA);

            auto mod_layoutB = new Layout();
            mod_layoutB->Add(new Text(font_, "Mod screen 2"));
            mod_screens_.Add(mod_layoutB);

            // screens go in the screen layout. Set the initial screen layout to the first main screen.
            screen_layout_.Add(main_layoutA);

            // app layout is the screen layout + the menu
            app_layout_.SetVertical();
            app_layout_.Add(&screen_layout_);
            app_layout_.Add(main_menu_);

            main_menu_->SetActive(0);
        }

        void Resized() override {
            DPRINTF("gui resized: menu fixed height: %d", main_menu_->GetFixedHeight());
            app_layout_.SetSize(GetWidth(), GetHeight());
            app_layout_.Resized();
        }

        void Paint(Graphics &g) override {
            app_layout_.Paint(g);
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

        void SelectMainScreen() {
            fprintf(stderr, "Main screen!\n");
            screen_layout_.Clear();
            screen_layout_.Add(main_screens_.Next());
            main_menu_->SetActive(0);
        }

        void SelectFilterScreen() {
            fprintf(stderr, "Filter screen!\n");
            screen_layout_.Clear();
            screen_layout_.Add(filter_screens_.Next());
            main_menu_->SetActive(1);
        }

        void SelectAmpScreen() {
            fprintf(stderr, "Amp screen!\n");
            screen_layout_.Clear();
            screen_layout_.Add(amp_screens_.Next());
            main_menu_->SetActive(2);
        }

        void SelectFxScreen() {
            fprintf(stderr, "Fx screen!\n");
            screen_layout_.Clear();
            screen_layout_.Add(fx_screens_.Next());
            main_menu_->SetActive(3);
        }

        void SelectModScreen() {
            fprintf(stderr, "Mod screen!\n");
            screen_layout_.Clear();
            screen_layout_.Add(mod_screens_.Next());
            main_menu_->SetActive(4);
        }

    private:
        SynthConfig &config_;
        Font font_ = Font(16);
        FilterView *filter_view_ = new FilterView(config_.filter_cutoff, config_.filter_resonance,
                                                  config_.filter_env_amt,
                                                  config_.filter_drive);
        AdsrView *filter_adsr_view_ = new AdsrView(config_.filter_attack, config_.filter_decay, config_.filter_sustain,
                                                   config_.filter_release, config_.filter_env_amt);

        Carousel main_screens_{};
        Carousel filter_screens_{};
        Carousel amp_screens_{};
        Carousel fx_screens_{};
        Carousel mod_screens_{};

        std::vector<MenuItem *> menu_items_ = {
                new MenuItem(new Text(font_, "MAIN"), true),
                new MenuItem(new Text(font_, "FILTER"), false),
                new MenuItem(new Text(font_, "AMP"), false),
                new MenuItem(new Text(font_, "FX"), false),
                new MenuItem(new Text(font_, "MOD"), false)
        };
        MainMenu *main_menu_ = new MainMenu(menu_items_);

        Layout screen_layout_ = Layout();
        Layout app_layout_ = Layout();

        TextFactory &text_factory_;
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


            layout_ = Layout();
            layout_.SetHorizontal();
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
