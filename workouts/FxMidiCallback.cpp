//
// Created by Orion Letizi on 11/11/23.
//
#include "FxMidiCallback.h"

void FxMidiCallback::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
    if (message.isController()) {
        std::cout << "Midi CC: num: " << message.getControllerNumber() << "; val: " << message.getControllerValue() <<std::endl;
        FxRack_UpdateMidiControl(rack_,message.getControllerNumber(), message.getControllerValue());
    }
}

