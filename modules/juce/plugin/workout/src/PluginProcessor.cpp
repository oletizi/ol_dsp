//
// Created by Orion Letizi on 12/15/23.
//

#include "PluginProcessor.h"

const juce::String PluginProcessor::getName() const {
    return juce::String( "OL Workout");
}

void PluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {

}

void PluginProcessor::releaseResources() {

}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {

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

juce::AudioProcessor *PluginProcessor::createPluginFilter() JUCE_CALLTYPE {
     return new PluginProcessor();
}
