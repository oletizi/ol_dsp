//
// Created by Orion Letizi on 12/15/23.
//

#include "PluginProcessor.h"
#include "Editor.h"

const juce::String PluginProcessor::getName() const {
    return juce::String("OL Workout");
}

void PluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    voice.Init(sampleRate);
    fx.Init(sampleRate);
    osc.Init(sampleRate);
}

void PluginProcessor::releaseResources() {

}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    //const juce::MidiBufferIterator &iterator = midiMessages.begin();
    // XXX: figure out how to handle the midi messages aligned with the proper sample frame
    for (const auto &m: midiMessages) {
        const auto message = m.getMessage();
        if (message.isNoteOn()) {
            DBG("Note on");
            voice.NoteOn(message.getNoteNumber(), message.getVelocity());
        } else if (message.isNoteOff()) {
            DBG("Note off");
            voice.NoteOff(message.getNoteNumber(), message.getVelocity());
        } else if (message.isController()) {
            DBG("Controller");
            voice.UpdateMidiControl(message.getControllerNumber(), message.getControllerValue());
            fx.UpdateMidiControl(message.getControllerNumber(), message.getControllerValue());
        }
    }
    const int channel_count = buffer.getNumChannels();
    for (int i = 0; i < buffer.getNumSamples(); i++) {
        voice.Process(ibuf);
        fx.Process(ibuf, obuf);

        for (int j = 0; j < channel_count; j++) {

            *buffer.getWritePointer(j, i) = obuf[j];
        }
    }
    //for (auto &message: midiMessages.begin()) {}
}

double PluginProcessor::getTailLengthSeconds() const {
    return 0;
}

bool PluginProcessor::acceptsMidi() const {
    return true;
}

bool PluginProcessor::producesMidi() const {
    return false;
}

juce::AudioProcessorEditor *PluginProcessor::createEditor() {
    //return new juce::GenericAudioProcessorEditor (*this);
    return new Editor(*this);
}

bool PluginProcessor::hasEditor() const {
    return true;
}

int PluginProcessor::getNumPrograms() {
    return 0;
}

int PluginProcessor::getCurrentProgram() {
    return 0;
}

void PluginProcessor::setCurrentProgram(int index) {

}

const juce::String PluginProcessor::getProgramName(int index) {
    return juce::String();
}

void PluginProcessor::changeProgramName(int index, const juce::String &newName) {

}

void PluginProcessor::getStateInformation(juce::MemoryBlock &destData) {

}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes) {

}

void PluginProcessor::UpdateHardwareControl(uint8_t controller, t_sample value) {
    voice.UpdateHardwareControl(controller, value);
}
