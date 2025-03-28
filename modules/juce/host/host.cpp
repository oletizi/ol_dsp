//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"
#include <cstdio>

const juce::String OLJuceHost::getApplicationName() {
    return "JuceHello";
}

const juce::String OLJuceHost::getApplicationVersion() {
    return "0.1";
}

void OLJuceHost::initialise(const juce::String &commandLineParameters) {
    std::cout << "Initialising OLJuceHost..." << std::endl;

    const juce::String inputDevice = "Volt 4";
    const juce::String outputDevice = "Volt 4";
    const juce::String toLoad = "AUReverb"; //"TAL Reverb 4 Plugin"; // TODO: make this configurable

    const juce::AudioDeviceManager::AudioDeviceSetup deviceSetup{
        .bufferSize = 128,
        .sampleRate = 44100,
        .inputChannels = 2,
        .outputChannels = 2,
        .inputDeviceName = inputDevice,
        .outputDeviceName = outputDevice,
        .useDefaultInputChannels = true,
        .useDefaultOutputChannels = true
    };

    auto currentSetup = this->deviceManager.getAudioDeviceSetup();
    std::cout << "Current setup" << std::endl;
    std::cout << "  input device: " << currentSetup.inputDeviceName << std::endl;
    std::cout << "  output device: " << currentSetup.outputDeviceName << std::endl;
    std::cout << "  sample rate: " << currentSetup.sampleRate << std::endl;
    std::cout << "  num channels: " << currentSetup.outputChannels.toString(10) << std::endl;
    for (auto type: this->deviceManager.getAvailableDeviceTypes()) {
        std::cout << "Device type: " << type->getTypeName() << std::endl;
        for (auto name: type->getDeviceNames(true)) {
            std::cout << "  input: " << name << std::endl;
        }
        for (auto name: type->getDeviceNames()) {
            std::cout << "  output: " << name << std::endl;
        }
    }

    formatManager.addDefaultFormats();
    std::cout << "Num formats: " << formatManager.getNumFormats() << std::endl;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        constexpr int scanMax = 10000; // TODO: make this configurable
        constexpr bool recursive = true;
        juce::FileSearchPath path;
        juce::File deadMansPedalFile("~/tmp/deadPedals");
        auto format = formatManager.getFormat(i);
        juce::String pluginName;
        auto scanner = new juce::PluginDirectoryScanner(
            this->knownPlugins, *format, path, recursive, deadMansPedalFile);

        std::cout << "Scanning " << format->getName() << std::endl;
        bool more = true;
        int count = 0;
        while (more && ++count < scanMax) {
            auto next = scanner->getNextPluginFileThatWillBeScanned();
            more = next.length();
            if (next.contains(toLoad)) {
                scanner->scanNextFile(true, pluginName);
                std::cout << "  Scanned: " << pluginName << std::endl;
            } else {
                scanner->skipNextFile();
            }
        }
    }
    auto plugs = this->knownPlugins.getTypes();
    std::cout << "plugs count: " << plugs.size() << std::endl;
    for (auto plugDescription: plugs) {
        juce::String errorMessage("barf.");
        std::cout << "instantiating: " << plugDescription.name << std::endl;
        auto plug = formatManager.createPluginInstance(
            plugDescription, 441000, 128, errorMessage);
        // auto params = plug->getParameters();
        // for ( auto param : params) {
        //     std::cout << "  param: " << param->getName(100) << std::endl;
        // }
        if (plug != nullptr) {
            // plug->enableAllBuses();
            this->instances.add(std::move(plug));
        }
    }
    // String initialise (int numInputChannelsNeeded,
    //                int numOutputChannelsNeeded,
    //                const XmlElement* savedState,
    //                bool selectDefaultDeviceOnFailure,
    //                const String& preferredDefaultDeviceName = String(),
    //                const AudioDeviceSetup* preferredSetupOptions = nullptr);
    auto result = this->deviceManager.initialise(1,
                                                 2,
                                                 nullptr,
                                                 true,
                                                 outputDevice,
                                                 &deviceSetup
    );
    std::cout << "Initialize result: " << result << std::endl;
    if (result.length() == 0) {
        std::cout << "Audio device initialized. Starting pipeline..." << std::endl;
        this->deviceManager.addAudioCallback(this);
    }
}

void OLJuceHost::shutdown() {
    std::cout << "Shutdown OLJuceHost..." << std::endl;
}

void OLJuceHost::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    std::cout << "Audio device starting..." << std::endl;
    std::cout << "Audio device: " << device->getName() << std::endl;

    for (int i = 0; i < this->instances.size(); ++i) {
        // https://forum.juce.com/t/setting-buses-layout-of-hosted-plugin/55262
        // TODO: make number of inputs and outputs configurable
        auto layout = this->instances.getReference(i)->getBusesLayout();
        for (auto bus: layout.getBuses(true)) {
            auto count = bus.size();
            std::cout << "Bus size: " << count << std::endl;
        }
        this->instances.getReference(i)->prepareToPlay(device->getCurrentSampleRate(),
                                                       device->getCurrentBufferSizeSamples());
    }
}

void OLJuceHost::audioDeviceIOCallbackWithContext(const float *const*inputChannelData, int numInputChannels,
                                                  float *const*outputChannelData, int numOutputChannels,
                                                  const int numSamples,
                                                  const juce::AudioIODeviceCallbackContext &context) {
    if (numInputChannels == 0) {
        return;
    }
    audioBuffer.setSize(numOutputChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        int i = ch + 1 > numInputChannels ? 0 : ch;
        if (inputChannelData[ch] != nullptr) {
            audioBuffer.copyFrom(ch, 0, inputChannelData[i], numSamples);
        } else {
            audioBuffer.clear(ch, 0, numSamples);
        }
    }

    count++;
    if (count % 1000 == 0) {
        std::cout << "count: " << count <<
                "; input channels: " << numInputChannels <<
                "; output channels: " << numOutputChannels <<
                "; sample count: " << numSamples <<
                "; audio buffer: channels: " << audioBuffer.getNumChannels() <<
                std::endl;
        count = 0;
    }

    juce::MidiBuffer messages;
    for (int i = 0; i < this->instances.size(); ++i) {
        this->instances.getReference(i)->processBlock(audioBuffer, messages);
    }

    // === Copy audioBuffer into output ===
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        float *dest = outputChannelData[ch];

        if (const float *src = audioBuffer.getReadPointer(ch); dest != nullptr && src != nullptr)
            std::memcpy(dest, src, sizeof(float) * numSamples);
    }
}

void OLJuceHost::audioDeviceStopped() {
    std::cout << "Audio device stopped..." << std::endl;
}
