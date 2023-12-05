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

    Polyvoice multi;

    Voice v1;
    daisysp::Svf v1_filter = daisysp::Svf();
    daisysp::Adsr v1_filter_envelope = daisysp::Adsr();
    daisysp::Adsr v1_amp_envelope = daisysp::Adsr();
    daisysp::Port v1_portamento = daisysp::Port();

    Voice v2;
    daisysp::Svf v2_filter = daisysp::Svf();
    daisysp::Adsr v2_filter_envelope = daisysp::Adsr();
    daisysp::Adsr v2_amp_envelope = daisysp::Adsr();
    daisysp::Port v2_portamento = daisysp::Port();

    uint8_t voice_count = 2;
    //std::vector<Voice *> voices{&v1, &v2, &v3, &v4};
    //Voice voices[] = {v1, v2, v3, v4};
    Voice *voices[] = {&v1, &v2};

    Voice_Config(&v1, &v1_filter, &v1_filter_envelope, &v1_amp_envelope, &v1_portamento);
    Voice_Config(&v2, &v2_filter, &v2_filter_envelope, &v2_amp_envelope, &v2_portamento);

    Polyvoice_Config(&multi, voices, voice_count);

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

