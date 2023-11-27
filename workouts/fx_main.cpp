#include <iostream>
#include "fxlib/Fx.h"
#include "FxAudioCallback.h"
#include "FxMidiCallback.h"

using namespace ol::fx;

int main() {
    std::cout << "Hello, world!" << std::endl;
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);


    daisysp::DelayLine<t_sample, MAX_DELAY> delay_line1;
    DelayFx delay1;
    Delay_Config(&delay1, &delay_line1);

    daisysp::DelayLine<t_sample, 48000> delay_line2;
    DelayFx delay2;
    Delay_Config(&delay2, &delay_line2);


    daisysp::ReverbSc verb;
    ReverbFx reverb;
    ReverbSc_Config(&reverb, &verb);


    FxRack fxrack;
    FxRack_Config(&fxrack, &delay1, &delay2, &reverb);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    FxMidiCallback midi_callback(&fxrack);
    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }

    FxAudioCallback audio_callback(&deviceManager, &fxrack);

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