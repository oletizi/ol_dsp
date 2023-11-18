//
// Created by Orion Letizi on 11/11/23.
//
#include "FxMidiCallback.h"

FxMidiCallback::FxMidiCallback(FxControlPanel *control_panel) : control_panel_(control_panel) {}

void FxMidiCallback::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    if (message.isController()) {
        std::cout << "Midi CC: num: " << message.getControllerNumber() << "; val: " << message.getControllerValue() <<std::endl;
        control_panel_->UpdateMidi(message.getControllerNumber(),
                                   message.getControllerValue());
    }

}

