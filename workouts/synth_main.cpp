#include <iostream>
#include "ol_synthlib.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"

#include "workout_buddy.h"

#define CHANNEL_COUNT 2
#define VOICE_COUNT 2

using namespace ol::synth;

int main() {
    Voice *voices[VOICE_COUNT];
    for (auto &v : voices) {
        v = new SynthVoice<CHANNEL_COUNT>();
    }
    auto poly = Polyvoice<CHANNEL_COUNT, VOICE_COUNT>(voices);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    auto midi_callback = SynthMidiCallback(poly);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    auto callback = SynthAudioCallback(poly);
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

