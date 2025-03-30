//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"
#include <ol_corelib.h>
#define DEBUG 0

namespace ol::jucehost {
    const juce::String OLJuceHost::getApplicationName() {
        return "JuceHello";
    }

    const juce::String OLJuceHost::getApplicationVersion() {
        return "0.5";
    }

    void OLJuceHost::initialise(const juce::String &commandLineParameters) {
        std::cout << commandLineParameters << std::endl;
        std::cout << "Initialising OLJuceHost..." << std::endl;

        const juce::String configDir = "~/.ol_juce_host";
        const bool doList = commandLineParameters.contains("--list");

        auto controlMaps = new std::vector<ControlMapConfig>();
        controlMaps->push_back(ControlMapConfig{"Low Freq Decay", 80});
        controlMaps->push_back(ControlMapConfig{"High Freq Decay", 81});
        controlMaps->push_back(ControlMapConfig{"Gain", 82});
        controlMaps->push_back(ControlMapConfig{"Dry/Wet Mix", 83});

        auto cfg = new PluginConfig();
        cfg->name = juce::String("AUReverb");
        cfg->controlMaps = controlMaps;
        this->config.plugins.push_back(cfg);

        this->config.audioInputDevice = "Volt 4";
        this->config.audioOutputDevice = "Volt 4";
        this->config.midiInputDevice = "Volt 4";

        this->config.ignore.push_back("DrumGizmo");

        const juce::AudioDeviceManager::AudioDeviceSetup deviceSetup{
            .outputDeviceName = this->config.audioOutputDevice,
            .inputDeviceName = this->config.audioInputDevice,
            .sampleRate = this->config.sampleRate,
            .bufferSize = this->config.bufferSize,
            .inputChannels = this->config.inputChannelCount,
            .useDefaultInputChannels = true,
            .outputChannels = this->config.outputChannelCount,
            .useDefaultOutputChannels = true
        };

        // === Dump audio device info ===
        for (const auto type: this->deviceManager.getAvailableDeviceTypes()) {
            auto typeName = type->getTypeName();
            for (const auto name: type->getDeviceNames(true)) {
                std::cout << "Audio Input Device: <Type: " << typeName << ">, <Name: " << name << ">" << std::endl;
            }
            for (const auto name: type->getDeviceNames()) {
                std::cout << "Audio Output Device: <Type: " << typeName << ">, <Name: " << name << ">" << std::endl;
            }
        }

        // === Dump midi device info ===
        const auto midiInputs = juce::MidiInput::getAvailableDevices();
        for (const auto midiInputDevice: midiInputs) {
            std::cout << "Midi Input Device: <" << midiInputDevice.name << ">, <Identifier: " << midiInputDevice.
                    identifier << ">" << std::endl;

            if (!doList && !this->deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)) {
                std::cout << "    Enabling: " << midiInputDevice.name << std::endl;
                this->deviceManager.setMidiInputDeviceEnabled(midiInputDevice.identifier, true);
                std::cout << "    Enabled: " << this->deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)
                        << std::endl;
            }
            if (!doList) {
                std::cout << "    Adding this as a midi input device callback to: " << midiInputDevice.name <<
                        std::endl;
                this->deviceManager.addMidiInputDeviceCallback(midiInputDevice.identifier, this);
            }
        }
        // === Scan & instantiate plugins ===
        formatManager.addDefaultFormats();
        for (int i = 0; i < formatManager.getNumFormats(); ++i) {
            constexpr int scanMax = 10000; // TODO: make this configurable
            constexpr bool recursive = true;
            juce::FileSearchPath path;
            juce::File deadMansPedalFile(configDir + "/deadPedals");
            auto format = formatManager.getFormat(i);
            juce::String pluginName;
            auto scanner = new juce::PluginDirectoryScanner(
                this->knownPlugins, *format, path, recursive, deadMansPedalFile);
            bool more = true;
            int count = 0;
            while (more && ++count < scanMax) {
                auto next = scanner->getNextPluginFileThatWillBeScanned();
                // std::cout << "Next Plugin: " << next << std::endl;
                more = next.length();
                bool shouldIgnore = false;
                for (const auto ignore: config.ignore) {
                    if (next.startsWith(ignore)) {
                        shouldIgnore = true;
                        break;
                    }
                }

                if (shouldIgnore) {
                    std::cout << "  Ignore: " << next << std::endl;
                    scanner->skipNextFile();
                }

                if (doList && !shouldIgnore) {
                    // we want to print out all the plugin names
                    scanner->scanNextFile(true, pluginName);
                    // std::cout << "Plugin: <Format:" << format->getName() << ">, <Name: " << pluginName << ">" <<
                    //         std::endl;
                } else if (!shouldIgnore) {
                    for (const auto pluginConfig: this->config.plugins) {
                        if (next.contains(pluginConfig->name)) {
                            scanner->scanNextFile(true, pluginName);
                            std::cout << "  Scanned: " << pluginName << std::endl;
                        } else {
                            scanner->skipNextFile();
                        }
                    }
                }
            }
        }
        auto plugs = this->knownPlugins.getTypes();
        for (auto plugDescription: plugs) {
            juce::String errorMessage("barf.");

            bool shouldIgnore = false;
            for (const auto ignore: config.ignore) {
                if (plugDescription.name.startsWith(ignore)) {
                    std::cout << "  Ignore: " << plugDescription.name << std::endl;
                    shouldIgnore = true;
                    break;
                }
            }
            if (shouldIgnore) { continue; }
            std::cout << "Instantiating " << plugDescription.name << std::endl;
            auto plug = formatManager.createPluginInstance(
                plugDescription, 441000, 128, errorMessage);
            if (plug != nullptr) {
                std::cout << "Plugin: <Name: " << plug->getName() << ">" << std::endl;
                for (const auto parameter: plug->getParameters()) {
                    std::cout << "Plugin Parameter: <Format: " << plugDescription.pluginFormatName <<
                            ">, <Plugin Name: " << plug->getName() << ">, <Parameter Name: " <<
                            parameter->getName(100) << ">" << std::endl;
                }
                this->instances.push_back(std::move(plug));
            }
        }

        if (doList) {
            quit();
            return;
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

        this->mapCCs();
    }

    void OLJuceHost::audioDeviceIOCallbackWithContext(const float *const*inputChannelData, int numInputChannels,
                                                      float *const*outputChannelData, int numOutputChannels,
                                                      const int numSamples,
                                                      const juce::AudioIODeviceCallbackContext &context) {
        if (numInputChannels == 0) {
            return;
        }

        count++;
        constexpr bool debug = DEBUG && count % 1000 == 0;
        if (count >= 1000) {
            count = 0;
        } {
            std::lock_guard lock(q_mutex);
            while (!controlChanges.empty()) {
                std::cout << "  Applying parameter change; q.size(): " << controlChanges.size() << std::endl;
                const auto cc = controlChanges.front();
                cc->parameter->setValue(cc->value);
                controlChanges.pop();
                delete cc;
            }
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

    void OLJuceHost::mapCCs() {
        for (const auto cfg: this->config.plugins) {
            for (const auto [parameterName, midiCC]: *cfg->controlMaps) {
                for (const auto &instance: this->instances) {
                    for (const auto parameter: instance->getParameters()) {
                        if (parameter->getName(100).startsWith(parameterName)) {
                            this->ccMap.emplace(midiCC, parameter);
                        }
                    }
                }
            }
        }
    }

    void OLJuceHost::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) {
        // TODO:
        // * push parameter changes to a queue; dequeue and apply them in the render loop so they're applied atomically
        //   per buffer cycle
        if (message.isController()) {
            std::cout << std::endl << "MIDI CC: " << message.getControllerNumber() << std::endl;
            auto parameter = this->ccMap.at(message.getControllerNumber());
            if (parameter != nullptr) {
                const auto value = core::scale(message.getControllerValue(), 0, 127, 0, 1, 1);
                std::cout << "  Midi CC parameter change: " << parameter->getName(100) << ": " << value << std::endl;
                // parameter->setValue(value);
                {
                    std::lock_guard lock(q_mutex);
                    this->controlChanges.push(new ControlChange{parameter, value});
                }
            }
        }
    }
}
