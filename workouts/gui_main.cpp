//
// Created by Orion Letizi on 1/8/24.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "ol_corelib.h"
#include "guilib/ol_guilib.h"
#include "MainComponent.h"


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
class OlLayout : public juce::Component {
public:
    explicit OlLayout(ol::gui::Layout *layout) : layout_(layout) {}

    void paint(juce::Graphics &g) override {
        JuceGraphics myG(g);
        layout_->paint(myG);
    }

    void resized() override {
        layout_->setSize(getWidth(), getHeight());
    }

private:
    ol::gui::Layout *layout_;
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
            auto meter1 = new ol::gui::Meter(0.6f);
            auto meter2 = new ol::gui::Meter(0.25f);
            auto olLayout = new ol::gui::Layout();
            olLayout->setSize(128, 64); // XXX: Hack just to get this working
            olLayout->add(meter1);
            olLayout->add(meter2);

            auto layout = new OlLayout(olLayout);

            mainComponent->addAndMakeVisible(layout);
            setContentOwned(mainComponent, true);
            centreWithSize(128, 64);
            setVisible(true);
        }

        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};


START_JUCE_APPLICATION (MyApplication)