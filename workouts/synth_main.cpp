#include <iostream>
#include "ol_synthlib.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"

using namespace ol::synth;

int main() {
//    Voice voice1;
//    Voice_Config(&voice1);
//
//    Voice voice2;
//    Voice_Config(&voice2);

    Multivoice multi;

    Voice v1;
    Voice v2;
    Voice v3;
    Voice v4;
    std::vector<Voice *> voices{&v1, &v2, &v3, &v4};
    for (auto v : voices) {
        Voice_Config(v);
    }

    Multivoice_Config(&multi, &voices);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    SynthMidiCallback midi_callback(&multi);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    SynthAudioCallback callback = SynthAudioCallback(&multi);
    deviceManager.addAudioCallback(&callback);

    std::cout << "Send me some MIDI" << std::endl;
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

