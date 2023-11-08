#include <iostream>
#include "MyCallback.h"

int main() {

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);
    MyCallback callback = MyCallback(&deviceManager);
    std::cout << "Hello, World!" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 't') {
            deviceManager.playTestSound();
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
    std::cout << "Goodbye!" << std::endl;
    juce::shutdownJuce_GUI();
    return 0;
}
