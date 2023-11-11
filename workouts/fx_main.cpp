#include <iostream>
#include <ol_fxlib.h>
#include "FxAudioCallback.h"

using namespace ol::fxlib;

int main() {
    std::cout << "Hello, world!" << std::endl;
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);
    FxControlPanel cp;
    FxAudioCallback audio_callback(&deviceManager);
    std::cout << "Send me some audio!" << std::endl;
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
    return 0;
}