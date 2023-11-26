#include <iostream>
#include <ol_fxlib.h>
#include "FxAudioCallback.h"
#include "FxMidiCallback.h"

using namespace ol::fx;

int main() {
    std::cout << "Hello, world!" << std::endl;
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    DelayControlPanel delay_control_panel;
    LpfControlPanel lpf_control_panel;
    FxControlPanel fx_control_panel(&delay_control_panel, &lpf_control_panel);

    daisysp::DelayLine<t_sample, 48000> delay_line1;
    Delay delay1(&delay_control_panel, &delay_line1);

    daisysp::DelayLine<t_sample, 48000> delay_line2;
    Delay delay2(&delay_control_panel, &delay_line2);

    LPF lpf1(&lpf_control_panel);
    LPF lpf2(&lpf_control_panel);

    FxChain fx(&fx_control_panel, &delay1, &delay2, &lpf1, &lpf2);
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    FxMidiCallback midi_callback(&fx_control_panel);
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