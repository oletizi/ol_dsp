//
// Created by Orion Letizi on 1/8/24.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "ol_corelib.h"
#include "MainComponent.h"


namespace ol::gui {

    struct Point {
        int x = 0;
        int y = 0;
    };

    class Graphics {
    public:
        virtual void drawRect(int x, int y, int width, int height, int line_width) = 0;
        virtual void fillRect(int x, int y, int width, int height) = 0;
    };

    class OffsetGraphics : public Graphics {
    public:
        explicit OffsetGraphics(Graphics &g, Point offset) : g_(g), offset_(offset) {}

        void drawRect(int x, int y, int width, int height, int line_width) override {
            g_.drawRect(x + offset_.x, y + offset_.y, width, height, line_width);
        }

        void fillRect(int x, int y, int width, int height) override {
            g_.fillRect(x + offset_.x, y + offset_.y, width, height);
        }

    private:
        Point offset_;
        Graphics &g_;
    };

    class JuceGraphics : public Graphics {
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
    public:
        void add(Component *child) {
            children_.push_back(child);
            const auto width = getWidth();
            const auto height = getHeight() / int(children_.size());
            for (auto c: children_) {
                c->setSize(width, height);
            }
        }

    public:
        void paint(Graphics &g) override {
            Point offset{};
            const auto height = getHeight() / int(children_.size());
            for (auto c: children_) {
                OffsetGraphics og(g, offset);
                c->paint(og);
                offset.y += height;
            }
        }

    private:
        std::vector<Component *> children_;
    };

    class Meter : public Component {
    public:
        explicit Meter(float level = 0) : level_(level) {}

        void setLevel(float level) { level_ = level; }

        void paint(Graphics &g) override {
            g.drawRect(0, 0, getWidth(), getHeight(), 1);
            g.fillRect(0, 0, int(core::scale(level_, 0, 1, 0, float(getWidth()), 1)), getHeight());
        }

    private:
        float level_;
    };
}

class OlLayout : public juce::Component {
public:
    explicit OlLayout(ol::gui::Layout *layout) : layout_(layout) {}

    void paint(juce::Graphics &g) override {
        ol::gui::JuceGraphics myG(g);
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