//
// Created by Orion Letizi on 3/25/25.
//

#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
class JuceHello : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override {
        return "JuceHello";
    };

    const juce::String getApplicationVersion() override {
        return "0.1";
    };

    void initialise(const juce::String &commandLineParameters) override {
        std::cout << "Initialising JuceHello..." << std::endl;
    };

    void shutdown() override {
        std::cout << "Shutdown JuceHello..." << std::endl;
    };
};
// #include <juce_audio_processors/juce_audio_processors.h>
// int main() {
//     std::cout << "Hello, World!" << std::endl;
//     return 0;
// }
START_JUCE_APPLICATION(JuceHello)
