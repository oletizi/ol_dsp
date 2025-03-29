//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"

namespace ol::jucehost {
    const juce::String OLJuceHost::getApplicationName() {
        return "JuceHello";
    }

    const juce::String OLJuceHost::getApplicationVersion() {
        return "0.1";
    }

    void OLJuceHost::initialise(const juce::String &commandLineParameters) {
        std::cout << "Initialising OLJuceHost..." << std::endl;

        auto controlMaps = new std::vector<ControlMap>();
        controlMaps->push_back(ControlMap{"Low Freq Decay", 80});
        controlMaps->push_back(ControlMap{"High Freq Decay", 81});
        controlMaps->push_back(ControlMap{"Gain", 82});
        controlMaps->push_back(ControlMap{"Dry/Wet Mix", 83});

        auto cfg = new PluginConfig();
        cfg->name = new juce::String("AUReverb");
        cfg->controlMaps = controlMaps;
        this->config.plugins.push_back(cfg);

        this->config.audioInputDevice = "Volt 4";
        this->config.audioOutputDevice = "Volt 4";
        this->config.midiInputDevice = "Volt 4";

        const juce::AudioDeviceManager::AudioDeviceSetup deviceSetup{
            .bufferSize = this->config.bufferSize,
            .sampleRate = this->config.sampleRate,
            .inputChannels = this->config.inputChannelCount,
            .outputChannels = this->config.outputChannelCount,
            .inputDeviceName = this->config.audioInputDevice,
            .outputDeviceName = this->config.audioOutputDevice,
            .useDefaultInputChannels = true,
            .useDefaultOutputChannels = true
        };

        // === Dump audio device info ===
        auto currentSetup = this->deviceManager.getAudioDeviceSetup();
        std::cout << "==============================" << std::endl;
        std::cout << "Audio devices" << std::endl;
        std::cout << "Current setup" << std::endl;
        std::cout << "  input device: " << currentSetup.inputDeviceName << std::endl;
        std::cout << "  output device: " << currentSetup.outputDeviceName << std::endl;
        std::cout << "  sample rate: " << currentSetup.sampleRate << std::endl;
        std::cout << "  buffer size: " << currentSetup.bufferSize << std::endl;
        std::cout << "  num input channels: " << currentSetup.inputChannels.toString(10) << std::endl;
        std::cout << "  num output channels: " << currentSetup.outputChannels.toString(10) << std::endl;
        for (const auto type: this->deviceManager.getAvailableDeviceTypes()) {
            std::cout << "Device type: " << type->getTypeName() << std::endl;
            for (const auto name: type->getDeviceNames(true)) {
                std::cout << "  input: " << name << std::endl;
            }
            for (const auto name: type->getDeviceNames()) {
                std::cout << "  output: " << name << std::endl;
            }
        }
        std::cout << std::endl;


        // === Scan & instantiate plugins ===
        std::cout << "==============================" << std::endl;
        std::cout << "Scanning plugins..." << std::endl;
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
                for (const auto pluginConfig: this->config.plugins) {
                    if (next.contains(* pluginConfig->name)) {
                        scanner->scanNextFile(true, pluginName);
                        std::cout << "  Scanned: " << pluginName << std::endl;
                    } else {
                        scanner->skipNextFile();
                    }
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
            if (plug != nullptr) {
                this->instances.push_back(std::move(plug));
            }
        }
        auto result = this->deviceManager.initialise(deviceSetup.inputChannels.toInteger(),
                                                     deviceSetup.outputChannels.toInteger(),
                                                     nullptr,
                                                     true,
                                                     this->config.audioOutputDevice,
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

        // === Dump midi device info ===
        const auto midiInputs = juce::MidiInput::getAvailableDevices();
        std::cout << "==============================" << std::endl;
        std::cout << "Midi input devices:" << std::endl;
        for (const auto midiInputDevice: midiInputs) {
            std::cout << "  Name: " << midiInputDevice.name << "; Identifier: " << midiInputDevice.identifier <<
                    std::endl;
            if (!this->deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)) {
                std::cout << "    Enabling: " << midiInputDevice.name << std::endl;
                this->deviceManager.setMidiInputDeviceEnabled(midiInputDevice.identifier, true);
                std::cout << "    Enabled: " << this->deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)
                        << std::endl;
            }
            std::cout << "    Adding this as a midi input device callback to: " << midiInputDevice.name << std::endl;
            this->deviceManager.addMidiInputDeviceCallback(midiInputDevice.identifier, this);
        }
        std::cout << std::endl;

        // for (int i = 0; i < this->instances.size(); ++i) {
        for (const auto &plug: this->instances) {
            // https://forum.juce.com/t/setting-buses-layout-of-hosted-plugin/55262
            // TODO: make number of inputs and outputs configurable
            auto layout = plug->getBusesLayout();
            for (auto bus: layout.getBuses(true)) {
                const auto count = bus.size();
                std::cout << "Bus size: " << count << std::endl;
            }
            plug->prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
        }
    }

    void OLJuceHost::audioDeviceIOCallbackWithContext(const float *const*inputChannelData, int numInputChannels,
                                                      float *const*outputChannelData, int numOutputChannels,
                                                      const int numSamples,
                                                      const juce::AudioIODeviceCallbackContext &context) {
        if (numInputChannels == 0) {
            return;
        }

        count++;
        const bool debug = count % 1000 == 0;
        if (debug) {
            count = 0;
        }

        audioBuffer.setSize(numOutputChannels, numSamples, false, false, true);

        if (debug) {
            std::cout << "count: " << count <<
                    "; input channels: " << numInputChannels <<
                    "; output channels: " << numOutputChannels <<
                    "; sample count: " << numSamples <<
                    "; audio buffer: channels: " << audioBuffer.getNumChannels() <<
                    std::endl;
        }

        for (int ch = 0; ch < numOutputChannels; ch++) {
            const int i = ch >= numInputChannels ? 0 : ch;
            if (debug) {
                std::cout << "  in->buf: ch: " << ch << "; i: " << i << std::endl;
            }
            if (inputChannelData[i] != nullptr) {
                audioBuffer.copyFrom(ch, 0, inputChannelData[i], numSamples);
            } else {
                audioBuffer.clear(ch, 0, numSamples);
            }
        }


        juce::MidiBuffer messages;
        for (const auto &plug: this->instances) {
            if (debug) {
                for (const auto param: plug->getParameters()) {
                    // TODO: remove me when plugin parameters are mapped
                    if (param->getName(100).startsWith("Dry")) {
                        param->setValue(.5);
                    }
                    std::cout << "    " << param->getName(100) << ": " << param->getValue() << std::endl;
                }
            }
            plug->processBlock(audioBuffer, messages);
        }

        // === Copy audioBuffer into output ===
        for (int ch = 0; ch < numOutputChannels; ch++) {
            float *dest = outputChannelData[ch];
            if (debug) {
                std::cout << "  buf->out  ch: " << ch << "; dest: " << dest << std::endl;
            }
            if (const float *src = audioBuffer.getReadPointer(ch); dest != nullptr && src != nullptr)
                std::memcpy(dest, src, sizeof(float) * numSamples);
        }
    }

    void OLJuceHost::audioDeviceStopped() {
        std::cout << "Audio device stopped..." << std::endl;
    }

    void OLJuceHost::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
        std::cout << "MIDI: " << message.getDescription() << std::endl;
    }
}
