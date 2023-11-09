#include <iostream>
#include "MyCallback.h"
#include <ol_synthlib.h>

using namespace ol::synthlib;

int main() {

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);
    t_sample frequency = 440;
    Voice voice = Voice();
    voice.SetFrequency(frequency);
    MyCallback callback = MyCallback(&deviceManager, &voice);

    std::cout << "Hello, World!" << std::endl;
    std::cout << "u: increase frequency" << std::endl;
    std::cout << "d: decrease frequency" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 't') {
            deviceManager.playTestSound();
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
        if (c == 'u') {
            frequency *= 2;
            voice.SetFrequency(frequency);
            std::cout << "New frequency: " << frequency << std::endl;
        }
        if (c == 'd') {
            frequency /= 2;
            voice.SetFrequency(frequency);
            std::cout << "New frequency: " << frequency << std::endl;
        }
    }
    std::cout << "Goodbye!" << std::endl;
    juce::shutdownJuce_GUI();
    return 0;
}
