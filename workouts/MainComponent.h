#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class JuceGraphics : public ol::gui::Graphics {
public:
    explicit JuceGraphics(juce::Graphics &g) : g_(g) {}

    void DrawRect(int x, int y, int width, int height, int line_width) override {
        g_.drawRect(x, y, width, height, line_width);
    }

    void FillRect(int x, int y, int width, int height) override {
        g_.fillRect(x, y, width, height);
    }

    void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
        g_.drawLine(float(startX), float(startY), float(endX), float(endY), float(line_width));
    }

    void WritePixel(int x, int y, ol::gui::Color c) override {
        g_.fillRect(x, y, 2, 1);
    }

    void Print(std::string text, ol::gui::Rectangle area) override {
        g_.drawText(juce::String(text),
                    juce::Rectangle<int>(area.point.x, area.point.y, area.dimension.width, area.dimension.height),
                    juce::Justification::left);
    }

private:
    juce::Graphics &g_;
};

namespace ol::gui::ol_juce {

    /**
     * Bridge between Juce GUI component system and the ol::gui component system.
     */
    class OlGuiContainer : public juce::Component {
    public:
        explicit OlGuiContainer(ol::gui::Component *child) : child_(child) {}

        void paint(juce::Graphics &g) override {
            JuceGraphics myG(g);
            auto bounds = getLocalBounds();
            auto x = bounds.getTopLeft().x;
            auto y = bounds.getTopLeft().y;

            ol::gui::OffsetGraphics offsetGraphics(myG, ol::gui::Point{x, y});
            child_->Paint(offsetGraphics);
        }

        void resized() override {
            child_->SetSize(getWidth(), getHeight());
            child_->Resized();
        }

    private:
        ol::gui::Component *child_;
    };

    /**
     * Updates the value of a Control based on the value of a slider
     */
    class SliderListener : public juce::Slider::Listener {

    public:
        explicit SliderListener(SynthGui &app, OlGuiContainer &container, ol::ctl::Control &control)
                : app_(app), container_(container), control_(control) {}

        void sliderValueChanged(juce::Slider *slider) override {
            control_.setScaledValue(t_sample(slider->getValue()));
            app_.ControlChange(control_);
            container_.repaint();
        }

    private:
        ol::ctl::Control &control_;
        SynthGui &app_;
        OlGuiContainer &container_;
    };

    struct LabelComponent {
        juce::Component &component;
        juce::Label &label;
    };

    class MainComponent : public juce::Component {
    public:
        MainComponent() {
            addAndMakeVisible(screen_container_);
            for (auto s: filter_sliders_) {
                s->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                s->setRange(0, 1);
                s->setTextBoxStyle(juce::Slider::NoTextBox, false, 20, 20);
                addAndMakeVisible(s);
            }

            for (auto s: amp_sliders_) {
                s->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                s->setRange(0, 1);
                s->setTextBoxStyle(juce::Slider::NoTextBox, false, 20, 20);
                addAndMakeVisible(s);
            }

            s_filter_cutoff.addListener(new SliderListener(app_, screen_container_, app_config.filter_cutoff));
            s_filter_resonance.addListener(new SliderListener(app_, screen_container_, app_config.filter_resonance));
            s_filter_env_amt.addListener(new SliderListener(app_, screen_container_, app_config.filter_env_amt));
            s_filter_drive.addListener(new SliderListener(app_, screen_container_, app_config.filter_drive));

            s_filter_attack.addListener(new SliderListener(app_, screen_container_, app_config.filter_attack));
            s_filter_decay.addListener(new SliderListener(app_, screen_container_, app_config.filter_decay));
            s_filter_sustain.addListener(new SliderListener(app_, screen_container_, app_config.filter_sustain));
            s_filter_release.addListener(new SliderListener(app_, screen_container_, app_config.filter_release));

            s_amp_attack.addListener(new SliderListener(app_, screen_container_, app_config.amp_attack));
            s_amp_decay.addListener(new SliderListener(app_, screen_container_, app_config.amp_decay));
            s_amp_sustain.addListener(new SliderListener(app_, screen_container_, app_config.amp_sustain));
            s_amp_release.addListener(new SliderListener(app_, screen_container_, app_config.amp_release));
        }

        ~MainComponent() override = default;

        void paint(::juce::Graphics &g) override {
            g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
            juce::Component::paint(g);
        }

        void resized() override {
            auto bounds = getLocalBounds();
            screen_container_.setSize(SCREEN_WIDTH, SCREEN_HEIGHT);
            screen_container_.setBounds(bounds.getCentreX() - (SCREEN_WIDTH / 2), 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            juce::FlexBox filter_box;
            filter_box.justifyContent = juce::FlexBox::JustifyContent::center;
            filter_box.alignContent = juce::FlexBox::AlignContent::center;
            for (auto s: filter_sliders_) {
                filter_box.items.add(juce::FlexItem(*s).withMinWidth(60).withMinHeight(60));
            }

            juce::FlexBox amp_box;
            amp_box.justifyContent = juce::FlexBox::JustifyContent::center;
            amp_box.alignContent = juce::FlexBox::AlignContent::center;
            for (auto s: amp_sliders_) {
                amp_box.items.add(juce::FlexItem(*s).withMinWidth(60).withMinHeight(60));
            }

            juce::FlexBox column;
            column.flexDirection = juce::FlexBox::Direction::column;
            column.alignItems = juce::FlexBox::AlignItems::center;
            column.justifyContent = juce::FlexBox::JustifyContent::center;
            column.items.add(juce::FlexItem(screen_container_).withMinWidth(SCREEN_WIDTH).withMinHeight(SCREEN_HEIGHT));
            column.items.add(juce::FlexItem(filter_box).withMinHeight(80));
            column.items.add(juce::FlexItem(amp_box).withMinHeight(80));
            column.performLayout(getLocalBounds());
        }

    private:

        // Filter controls
        juce::Slider s_filter_cutoff;
        juce::Slider s_filter_resonance;
        juce::Slider s_filter_env_amt;
        juce::Slider s_filter_drive;
        juce::Slider s_filter_attack;
        juce::Slider s_filter_decay;
        juce::Slider s_filter_sustain;
        juce::Slider s_filter_release;

        std::vector<juce::Slider *> filter_sliders_{
                &s_filter_cutoff,
                &s_filter_resonance,
                &s_filter_drive,
                &s_filter_env_amt,
                &s_filter_attack,
                &s_filter_decay,
                &s_filter_sustain,
                &s_filter_release
        };

        // Amp controls
        juce::Slider s_amp_attack;
        juce::Slider s_amp_decay;
        juce::Slider s_amp_sustain;
        juce::Slider s_amp_release;

        std::vector<juce::Slider *> amp_sliders_{
                &s_amp_attack,
                &s_amp_decay,
                &s_amp_sustain,
                &s_amp_release
        };

        SynthAppConfig app_config{};
        SynthGui app_ = SynthGui(app_config);
        OlGuiContainer screen_container_ = OlGuiContainer(&app_);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
    };
}