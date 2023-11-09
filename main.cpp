#include <iostream>
#include <ol_synthlib.h>
#include "MyCallback.h"

using namespace ol::synthlib;

class MyMidiCallback : public juce::MidiInputCallback {
public:
    explicit MyMidiCallback(ControlPanel *control_panel, Voice *voice) : control_panel_(control_panel), voice_(voice) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isNoteOn()) {
            std::cout << "NoteOn: " << message.getNoteNumber() << std::endl;
            voice_->NoteOn(static_cast<uint8_t>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            std::cout << "NoteOff: " << message.getNoteNumber() << std::endl;
            voice_->NoteOff(static_cast<uint8_t>(message.getNoteNumber()));
        }
    }

private:
    ControlPanel *control_panel_;
    Voice *voice_;
};

int main() {
    ControlPanel control_panel;
    Voice voice = Voice(&control_panel);

    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    MyMidiCallback midi_callback(&control_panel, &voice);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    MyCallback callback = MyCallback(&deviceManager, &voice);

    std::cout << "Send me some " << std::endl;
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

