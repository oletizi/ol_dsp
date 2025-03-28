//
// Created by Orion Letizi on 3/26/25.
//
#pragma once
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#ifndef HOST_H
#define HOST_H
class OLJuceHost : public juce::JUCEApplication  {
public:
    const juce::String getApplicationName() override;

    const juce::String getApplicationVersion() override;

    void initialise(const juce::String &commandLineParameters) override;

    void shutdown() override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::Array<std::unique_ptr<juce::AudioPluginInstance>> instances;
};
#endif //HOST_H
START_JUCE_APPLICATION(OLJuceHost)