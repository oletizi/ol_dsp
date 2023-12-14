#include <iostream>
#include "ol_synthlib.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"

#include "workout_buddy.h"

#define CHANNEL_COUNT 2
#define VOICE_COUNT 2

using namespace ol::synth;

int main() {
//    daisysp::Oscillator dosc1;
//    auto osc1 = new OscillatorSoundSource<CHANNEL_COUNT>(dosc1);
//    daisysp::Svf *v1_f[CHANNEL_COUNT] = {};
//    for (auto &i: v1_f) {
//        i = new daisysp::Svf();
//    }
//    auto v1_fe = daisysp::Adsr();
//    auto v1_ae = daisysp::Adsr();
//    auto v1_port = daisysp::Port();
//    auto v1 = SynthVoice<CHANNEL_COUNT>(osc1, v1_f, &v1_fe, &v1_ae, &v1_port);
//    daisysp::Svf v1_filter = daisysp::Svf();
//    daisysp::Adsr v1_filter_envelope = daisysp::Adsr();
//    daisysp::Adsr v1_amp_envelope = daisysp::Adsr();
//    daisysp::Port v1_portamento = daisysp::Port();
//
//    daisysp::Oscillator dosc2;
//    auto osc2 = new OscillatorSoundSource<CHANNEL_COUNT>(dosc2);
//    daisysp::Svf *v2_f[CHANNEL_COUNT] = {};
//    for (auto &i: v2_f) {
//        i = new daisysp::Svf();
//    }
//    daisysp::Adsr v2_filter_envelope = daisysp::Adsr();
//    daisysp::Adsr v2_amp_envelope = daisysp::Adsr();
//    daisysp::Port v2_portamento = daisysp::Port();
//    SynthVoice<CHANNEL_COUNT> v2 = SynthVoice<CHANNEL_COUNT>(osc2, v2_f, &v2_filter_envelope, &v2_amp_envelope,
//                                                             &v2_portamento);

    Voice *voices[VOICE_COUNT] = {ol::workout::VoiceFactory<CHANNEL_COUNT>(),
                                  ol::workout::VoiceFactory<CHANNEL_COUNT>()};
    auto poly = Polyvoice<CHANNEL_COUNT, VOICE_COUNT>(voices);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    //auto midi_callback = SynthMidiCallback(poly);
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

