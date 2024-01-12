//
// Created by Orion Letizi on 1/8/24.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "ol_corelib.h"
#include "guilib/ol_guilib.h"
#include "MainComponent.h"

using namespace ol::gui;



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
        explicit MainWindow(juce::String name) : DocumentWindow(name,
                                                       juce::Colours::lightgrey,
                                                       DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
//            auto mainComponent = new ol::gui::ol_juce::MainComponent();
            main_component_.setSize(500, 250);

            // XXX: Hack just to get this working

            setContentOwned(&main_component_, true);
            centreWithSize(500, 250);
            setResizable(true, false);
            setVisible(true);
}

        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:

        ol::gui::ol_juce::MainComponent main_component_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};


START_JUCE_APPLICATION (MyApplication)