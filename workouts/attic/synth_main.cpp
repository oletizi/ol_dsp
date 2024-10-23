#include <iostream>
#include "ol_synthlib.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"

#include "../workout_buddy.h"

#define CHANNEL_COUNT 2

using namespace ol::synth;

int main() {
    //Voice *voices[VOICE_COUNT];
    SynthVoice sv1;
    SynthVoice sv2;
    SynthVoice sv3;
    SynthVoice sv4;
    std::vector<Voice *> voices = {&sv1, &sv2, &sv3, &sv4};
    Polyvoice poly(voices);

//    Voice *poly = new Polyvoice<CHANNEL_COUNT>(voices);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    SynthMidiCallback<CHANNEL_COUNT> midi_callback(&poly);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    SynthAudioCallback<CHANNEL_COUNT> callback(&poly);
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

