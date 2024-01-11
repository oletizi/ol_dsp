//
// Created by Orion Letizi on 1/8/24.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "ol_corelib.h"
#include "guilib/ol_guilib.h"
#include "MainComponent.h"

using namespace ol::gui;

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
class OlGuiContainer : public juce::Component {
public:
    explicit OlGuiContainer(ol::gui::Component *child) : child_(child) {}

    void paint(juce::Graphics &g) override {
        JuceGraphics myG(g);
        child_->paint(myG);
    }

    void resized() override {
        child_->setSize(getWidth(), getHeight());
        child_->resized();
    }

private:
    ol::gui::Component *child_;
};

class MyApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override {
        return juce::String();
    }

    const juce::String getApplicationVersion() override {
        return juce::String();
    }

    void initialise(const juce::String &commandLineParameters) override {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        mainWindow = nullptr;
    }

public:
    //...

    //==============================================================================
    class MainWindow : public juce::DocumentWindow {
    public:
        MainWindow(juce::String name) : DocumentWindow(name,
                                                       juce::Colours::lightgrey,
                                                       DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            auto mainComponent = new MainComponent();

            int width = 128, height = 64;


            // XXX: Hack just to get this working
            app_layout.setSize(width, height);

            mainComponent->addAndMakeVisible(app_layout);
            setContentOwned(mainComponent, true);
            centreWithSize(width, height);
            setVisible(true);
        }

        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
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

        SynthApp app = SynthApp(app_config);
        OlGuiContainer app_layout = OlGuiContainer(&app);
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};


START_JUCE_APPLICATION (MyApplication)