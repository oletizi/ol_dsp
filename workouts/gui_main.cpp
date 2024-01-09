//
// Created by Orion Letizi on 1/8/24.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "MainComponent.h"

namespace ol::gui {

    class Graphics {
    public:
        virtual void drawRect(int x, int y, int width, int height, int line_width) = 0;
    };

    class JuceGraphics : public Graphics {
    private:
        juce::Graphics &g_;
    public:
        explicit JuceGraphics(juce::Graphics &g) : g_(g) {}

        void drawRect(int x, int y, int width, int height, int line_width) override {
            g_.drawRect(x, y, width, height, line_width);
        }
    };

    class Component {
    public:
        void setSize(int width, int height) {
            width_ = width;
            height_ = height;
        };

        int getWidth() { return width_; }

        int getHeight() { return height_; }

        virtual void paint(Graphics &g) = 0;

    private:
        int width_ = 0;
        int height_ = 0;
    };

    class Layout : public Component {
        void add(Component *c) {
            children_.push_back(c);
        }

        void paint(Graphics &g) override {
            for (auto c : children_) {
                c->paint(g);
            }
        }

    private:
        std::vector<Component *> children_;
    };

    class Meter : public Component {
    public:
        void paint(Graphics &g) override {
            g.drawRect(0, 0, getWidth(), getHeight(), 1);
        }
    };
}

class Meter : public juce::Component {
public:
    explicit Meter(ol::gui::Meter *meter) : meter_(meter) {}

    void paint(juce::Graphics &g) override {
        ol::gui::JuceGraphics myG(g);
        meter_->paint(myG);
    }

private:
    ol::gui::Meter *meter_;
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
            auto meter = new Meter(new ol::gui::Meter());
            mainComponent->addAndMakeVisible(meter);
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