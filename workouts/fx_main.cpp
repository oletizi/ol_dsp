#include <iostream>
#include <ol_fxlib.h>
#include "FxAudioCallback.h"
#include "FxMidiCallback.h"

using namespace ol::fx;

int main() {
    std::cout << "Hello, world!" << std::endl;
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    try {
        deviceManager.initialiseWithDefaultDevices(2, 2);
    } catch (std::exception &e) {
        std::cerr << "Exception initializing device manager: " << e.what() << std::endl;
    }
    FxControlPanel cp;
    FxChain fx(&cp);
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    FxMidiCallback midi_callback(&cp);
    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }

    FxAudioCallback audio_callback(&deviceManager, &fx);

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