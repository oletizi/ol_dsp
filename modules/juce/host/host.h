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

namespace ol::jucehost {
    struct ControlMap {
        const juce::String parameterName;
        const int midiCC;
    };

    struct PluginConfig {
        const juce::String *name;
        const std::vector<ControlMap> *controlMaps;
    };

    struct HostConfig {
        juce::String midiInputDevice, audioInputDevice, audioOutputDevice;
        std::vector<PluginConfig *> plugins;
        int inputChannelCount = 2;
        int outputChannelCount = 2;
        double sampleRate = 44100;
        int bufferSize = 128;
    };

    class OLJuceHost final : public juce::JUCEApplication,
                             public juce::AudioIODeviceCallback,
                             public juce::MidiInputCallback {
    public:
        const juce::String getApplicationName() override;

        const juce::String getApplicationVersion() override;

        void initialise(const juce::String &commandLineParameters) override;

        void shutdown() override;

        void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

        void audioDeviceIOCallbackWithContext(const float *const*inputChannelData, int numInputChannels,
                                              float *const*outputChannelData, int numOutputChannels, int
                                              numSamples, const juce::AudioIODeviceCallbackContext &context) override;

        void audioDeviceStopped() override;

        void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

    private:
        HostConfig config;
        juce::AudioDeviceManager deviceManager;
        juce::AudioPluginFormatManager formatManager;
        juce::KnownPluginList knownPlugins;
        std::vector<std::unique_ptr<juce::AudioPluginInstance> > instances;
        juce::AudioBuffer<float> audioBuffer;
        int count = 0;
    };
}
#endif //HOST_H
START_JUCE_APPLICATION(ol::jucehost::OLJuceHost)
