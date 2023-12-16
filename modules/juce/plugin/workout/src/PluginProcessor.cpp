//
// Created by Orion Letizi on 12/15/23.
//

#include "PluginProcessor.h"

const juce::String PluginProcessor::getName() const {
    return juce::String("OL Workout");
}

void PluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    voice.Init(sampleRate);
    osc.Init(sampleRate);
    osc.SetFreq(4000);
}

void PluginProcessor::releaseResources() {

}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    //const juce::MidiBufferIterator &iterator = midiMessages.begin();
    for (const auto &m: midiMessages) {
        const auto message = m.getMessage();
        if (message.isNoteOn()) {
            voice.NoteOn(message.getNoteNumber(), message.getVelocity());
        } else if (message.isNoteOff()) {
            voice.NoteOff(message.getNoteNumber(), message.getVelocity());
        } else if (message.isController()) {
            voice.UpdateMidiControl(message.getControllerNumber(), message.getControllerValue());
        }
    }
    const int channel_count = buffer.getNumChannels();
    t_sample voice_out = 0;
    for (int i = 0; i < buffer.getNumSamples(); i++) {
        voice.Process(&voice_out);
        //voice_out = osc.Process();
        for (int j = 0; j < channel_count; j++) {
            *buffer.getWritePointer(j, i) = voice_out;
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
    return nullptr;
}

bool PluginProcessor::hasEditor() const {
    return false;
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
