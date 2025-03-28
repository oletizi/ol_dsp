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

class OLJuceHost final : public juce::JUCEApplication, public juce::AudioIODeviceCallback {
public:
    const juce::String getApplicationName() override;

    const juce::String getApplicationVersion() override;

    void initialise(const juce::String &commandLineParameters) override;

    void shutdown() override;

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float *const*outputChannelData, int numOutputChannels, int
                                          numSamples, const juce::AudioIODeviceCallbackContext &context) override;

    void audioDeviceStopped() override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::Array<std::unique_ptr<juce::AudioPluginInstance>> instances;
    juce::AudioBuffer<float> audioBuffer;
};
#endif //HOST_H
START_JUCE_APPLICATION(OLJuceHost)
