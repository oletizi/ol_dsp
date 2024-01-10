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
            int width = 128, height = 64;

            auto column1 = new Layout();
            auto column2 = new Layout();
            auto column3 = new Layout();
            auto columns = new Layout(Horizontal);

            columns->add(column1);
            columns->add(column2);
            columns->add(column3);

            auto meter1 = new Meter(0.6f);
            auto meter2 = new Meter(0.25f);
            auto meter3 = new Meter(0.5f);

            auto meter4 = new Meter(0.9f);
            auto meter5 = new Meter(0.8f);
            auto meter6 = new Meter(0.1f);

            auto meter7 = new Meter(0.3f);
            auto meter8 = new Meter(1);


            column1->add(meter1);
            column1->add(meter2);
            column1->add(meter3);

            column2->add(meter4);
            column2->add(meter5);
            column2->add(meter6);

            column3->add(meter7);
            column3->add(meter8);

            columns->setSize(width, height); // XXX: Hack just to get this working

            auto layout = new OlLayout(columns);

            mainComponent->addAndMakeVisible(layout);
            setContentOwned(mainComponent, true);
            centreWithSize(width, height);
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