#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class JuceGraphics : public ol::gui::Graphics {
public:
    explicit JuceGraphics(juce::Graphics &g) : g_(g) {}

    void drawRect(int x, int y, int width, int height, int line_width) override {
        g_.drawRect(x, y, width, height, line_width);
    }

    void fillRect(int x, int y, int width, int height) override {
        g_.fillRect(x, y, width, height);
    }

private:
    juce::Graphics &g_;
};

/**
 * Bridge between juce components and ol::gui components
 */


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
            child_->paint(offsetGraphics);
        }

        void resized() override {
            child_->setSize(getWidth(), getHeight());
            child_->resized();
        }

    private:
        ol::gui::Component *child_;
    };

    /**
     * Updates the value of a Control based on the value of a slider
     */
    class SliderListener : public juce::Slider::Listener {

    public:
        explicit SliderListener(OlGuiContainer &container, ol::ctl::Control &control) : container_(container),
                                                                                        control_(control) {}

        void sliderValueChanged(juce::Slider *slider) override {
            control_.setScaledValue(t_sample(slider->getValue()));
            container_.repaint();
        }

    private:
        ol::ctl::Control &control_;
        OlGuiContainer &container_;
    };


    class LabeledComponent : public juce::Component {
    public:
        LabeledComponent(juce::Component &component, const juce::String &label_text) : component_(component) {
            label_.setText(label_text, juce::NotificationType::dontSendNotification);
        }

        void resized() override {
            auto bounds = getLocalBounds();
            auto label_height = bounds.getHeight() * 0.25;
            auto component_height = bounds.getHeight() * 0.75;
            component_.setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), int(component_height));
            label_.setBounds(bounds.getX(), bounds.getY() + int(component_height), bounds.getWidth(),
                             int(label_height));
        }

    private:
        juce::Component &component_;
        juce::Label label_;
    };

    class MainComponent : public juce::Component {
    public:
        MainComponent() {
            addAndMakeVisible(screen_container_);
            for (auto s: sliders) {
                s->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                s->setRange(0, 1);
                s->setTextBoxStyle(juce::Slider::NoTextBox, false, 20, 20);
                addAndMakeVisible(s);
            }

            s_filter_cutoff.addListener(new SliderListener(screen_container_, app_config.filter_cutoff));
            s_filter_resonance.addListener(new SliderListener(screen_container_, app_config.filter_resonance));
            s_filter_env_amt.addListener(new SliderListener(screen_container_, app_config.filter_env_amt));
            s_filter_drive.addListener(new SliderListener(screen_container_, app_config.filter_drive));

            s_filter_attack.addListener(new SliderListener(screen_container_, app_config.filter_attack));
            s_filter_decay.addListener(new SliderListener(screen_container_, app_config.filter_decay));
            s_filter_sustain.addListener(new SliderListener(screen_container_, app_config.filter_sustain));
            s_filter_release.addListener(new SliderListener(screen_container_, app_config.filter_release));
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

            auto slider_width = bounds.getWidth() / int(sliders.size());
            auto slider_height = bounds.getHeight() / 2;
            for (int i = 0; i < sliders.size(); i++) {
                auto slider = sliders[i];
                slider->setBounds(i * slider_width, slider_height, slider_width, slider_height);
//                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, slider_width - 5, 20);
            }

        }

    private:
        //==============================================================================
        // Your private member variables go here...
        juce::Slider s_filter_cutoff;
        juce::Slider s_filter_resonance;
        juce::Slider s_filter_env_amt;
        juce::Slider s_filter_drive;
        juce::Slider s_filter_attack;
        juce::Slider s_filter_decay;
        juce::Slider s_filter_sustain;
        juce::Slider s_filter_release;

        std::vector<juce::Slider *> sliders{
                &s_filter_cutoff,
                &s_filter_resonance,
                &s_filter_env_amt,
                &s_filter_drive,
                &s_filter_attack,
                &s_filter_decay,
                &s_filter_sustain,
                &s_filter_release
        };

        SynthAppConfig app_config{
                Dimension{128, 64},
                Control{CC_FILTER_CUTOFF, 2500},
                Control{CC_FILTER_RESONANCE, 3800},
                Control{CC_ENV_FILT_AMT, 550},
                Control{CC_FILTER_DRIVE, 80},

                Control{CC_ENV_FILT_A, 4000},
                Control{CC_ENV_FILT_D, 3000},
                Control{CC_ENV_FILT_S, 2000},
                Control{CC_ENV_FILT_R, 2500}
        };
        SynthApp app_ = SynthApp(app_config);
        OlGuiContainer screen_container_ = OlGuiContainer(&app_);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
    };
}