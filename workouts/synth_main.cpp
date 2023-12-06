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


    auto osc1 = ol::synth::OscillatorSoundSource(daisysp::Oscillator());
    auto v1_f = daisysp::Svf();
    auto v1_fe = daisysp::Adsr();
    auto v1_ae = daisysp::Adsr();
    auto v1_port = daisysp::Port();
    auto v1 = ol::synth::SynthVoice(osc1, v1_f, v1_fe, v1_ae, v1_port);
    daisysp::Svf v1_filter = daisysp::Svf();
    daisysp::Adsr v1_filter_envelope = daisysp::Adsr();
    daisysp::Adsr v1_amp_envelope = daisysp::Adsr();
    daisysp::Port v1_portamento = daisysp::Port();

    auto osc2 = ol::synth::OscillatorSoundSource(daisysp::Oscillator());
    daisysp::Svf v2_filter = daisysp::Svf();
    daisysp::Adsr v2_filter_envelope = daisysp::Adsr();
    daisysp::Adsr v2_amp_envelope = daisysp::Adsr();
    daisysp::Port v2_portamento = daisysp::Port();
    auto v2 = ol::synth::SynthVoice(osc2, v2_filter, v2_filter_envelope, v2_amp_envelope, v2_portamento);

    uint8_t voice_count = 2;
    Voice *voices[] = {&v1, &v2};
    auto poly = Polyvoice(voices, voice_count);

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

