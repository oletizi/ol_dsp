#include <iostream>
#include <daisysp.h>
#include <ol_synthlib.h>
#include "MyCallback.h"

using namespace ol::synthlib;

class MyMidiCallback : public juce::MidiInputCallback {
public:
    explicit MyMidiCallback(ControlPanel *control_panel) : control_panel_(control_panel) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isNoteOn()) {
            std::cout << "NoteOn: " << message.getNoteNumber() << std::endl;
            control_panel_->osc_frequency.UpdateValueHardware(daisysp::mtof(static_cast<uint8_t>(message.getNoteNumber())));
        } else if (message.isNoteOff()) {
            std::cout << "NoteOff: " << message.getNoteNumber() << std::endl;
        }
    }

private:
    ControlPanel *control_panel_;
};

int main() {
    ControlPanel control_panel;
    Voice voice = Voice(&control_panel);
    t_sample frequency = 440;
    control_panel.osc_frequency.UpdateValueHardware(frequency);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    MyMidiCallback midi_callback(&control_panel);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    MyCallback callback = MyCallback(&deviceManager, &voice);

    std::cout << "Hello, World!" << std::endl;
    std::cout << "u: increase frequency" << std::endl;
    std::cout << "d: decrease frequency" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 't') {
            deviceManager.playTestSound();
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
        if (c == 'u') {
            frequency *= 2;
            control_panel.osc_frequency.UpdateValueHardware(frequency);
            std::cout << "New frequency: " << frequency << std::endl;
        }
        if (c == 'd') {
            frequency /= 2;
            control_panel.osc_frequency.UpdateValueHardware(frequency);
            std::cout << "New frequency: " << frequency << std::endl;
        }
    }
    std::cout << "Goodbye!" << std::endl;
    juce::shutdownJuce_GUI();
    return 0;
}

