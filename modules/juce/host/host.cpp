//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"
#include <ol_corelib.h>
#include <fstream>
#include <string>

#define DEBUG 0

namespace ol::jucehost {
    const juce::String OLJuceHost::getApplicationName() {
        return "JuceHello";
    }

    const juce::String OLJuceHost::getApplicationVersion() {
        return "0.5";
    }

    juce::String parseConfigValue(const juce::String &line, const juce::String &startToken) {
        const auto start = line.indexOf(startToken) + startToken.length();
        const auto end = line.substring(start).indexOf(">") + start;
        return line.substring(start, end);
    }

    juce::String OLJuceHost::parseDeviceName(const juce::String &line) {
        return parseConfigValue(line, "<Name: ");
    }

    void OLJuceHost::parseConfigLine(const juce::String &line) {
        std::cout << "Config line: " << line << std::endl;
        if (line.startsWith("Audio Input Device")) {
            config.audioInputDevice = parseDeviceName(line);
        }
        if (line.startsWith("Audio Output Device")) {
            config.audioOutputDevice = parseDeviceName(line);
        }
        if (line.startsWith("Midi Input Device")) {
            config.midiInputDevice = parseDeviceName(line);
        }
        if (line.startsWith("Plugin Parameter")) {
            const auto pluginFormat = parseConfigValue(line, "<Format: ");
            const auto pluginName = parseConfigValue(line, "<Plugin Name: ");
            const auto parameterName = parseConfigValue(line, "<Parameter Name: ");
            const auto cc = parseConfigValue(line, "<CC: ").getIntValue();
            PluginConfig *plug = nullptr;
            for (auto test: config.plugins) {
                if (test->name.startsWith(pluginName)) {
                    plug = test;
                }
            }
            if (plug == nullptr) {
                plug = new PluginConfig();
                plug->name = pluginName;
                plug->format = pluginFormat;
                config.plugins.push_back(plug);
            }
            const auto ccMap = new ControlMapConfig{.parameterName = parameterName, .midiCC = cc};
            plug->controlMaps.push_back(ccMap);
        }
    }

    void OLJuceHost::initialise(const juce::String &commandLineParameters) {
        std::cout << commandLineParameters << std::endl;
        std::cout << "Initialising OLJuceHost..." << std::endl;
        const bool doList = commandLineParameters.contains("--list");

        const juce::String configDir = juce::String(std::getenv("HOME")) + "/.ol_juce_host";
        if (!doList) {
            auto path = configDir + "/config";
            std::cout << "Loading config from: " << path << std::endl;
            if (std::ifstream file(path.toStdString()); file.is_open()) {
                std::string line;
                while (getline(file, line)) {
                    parseConfigLine(juce::String(line));
                }
            } else {
                std::cerr << "Unable to open config file: " << path << std::endl;
            }
        }

        std::cout << "INPUT DEVICE     : " << config.audioInputDevice << std::endl;
        std::cout << "OUTPUT DEVICE    : " << config.audioOutputDevice << std::endl;
        std::cout << "MIDI INPUT DEVICE: " << config.midiInputDevice << std::endl;
        // config.ignore.push_back("DrumGizmo");
        config.ignore.push_back("drumkv1");
        config.ignore.push_back("padthv1");
        config.ignore.push_back("samplv1");
        config.ignore.push_back("synthv1");

        const juce::AudioDeviceManager::AudioDeviceSetup deviceSetup{
            .outputDeviceName = config.audioOutputDevice,
            .inputDeviceName = config.audioInputDevice,
            .sampleRate = config.sampleRate,
            .bufferSize = config.bufferSize,
            .inputChannels = config.inputChannelCount,
            .useDefaultInputChannels = true,
            .outputChannels = config.outputChannelCount,
            .useDefaultOutputChannels = true
        };

        // === Dump audio device info ===
        for (const auto type: deviceManager.getAvailableDeviceTypes()) {
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
            std::cout << "Midi Input Device: <Name: " << midiInputDevice.name << ">" << std::endl;

            if (!doList && !deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)) {
                std::cout << "    Enabling: " << midiInputDevice.name << std::endl;
                deviceManager.setMidiInputDeviceEnabled(midiInputDevice.identifier, true);
                std::cout << "    Enabled: " << deviceManager.isMidiInputDeviceEnabled(midiInputDevice.identifier)
                        << std::endl;
            }
            if (!doList) {
                std::cout << "    Adding this as a midi input device callback to: " << midiInputDevice.name <<
                        std::endl;
                deviceManager.addMidiInputDeviceCallback(midiInputDevice.identifier, this);
            }
        }

        // Start OSC receiver
        if (!doList) {
            const int port = 3819;
            if (!oscReceiver.connect(port)) {
                std::cerr << "OSC connect error: Unable to connect to UDP port: " << port << std::endl;
            } else {
                std::cout << "OSC connect success: Connected to UDP port: " << port << std::endl;
            }
            oscReceiver.addListener(this);
        }

        // === Scan & instantiate plugins ===
        formatManager.addDefaultFormats();
        // for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        for (const auto format: formatManager.getFormats()) {
            constexpr int scanMax = 10000; // TODO: make this configurable
            constexpr bool recursive = true;
            juce::FileSearchPath path;
            juce::File deadMansPedalFile(configDir + "/deadPedals");
            //auto format = formatManager.getFormat(i);
            juce::String pluginName;
            auto scanner = new juce::PluginDirectoryScanner(
                knownPlugins, *format, path, recursive, deadMansPedalFile);
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
                    std::cout << "Plugin: <Format:" << format->getName() << ">, <Name: " << pluginName << ">" <<
                             std::endl;
                } else if (!shouldIgnore) {
                    for (const auto pluginConfig: this->config.plugins) {
                        std::cout << "Checking to see if : " << next << " contains " << pluginConfig->name << std::endl;
                        if (next.contains(pluginConfig->name)) {
                            std::cout << "  Next plugin: " << next << " matches plugin config: " << pluginConfig->name
                                    << std::endl;
                            scanner->scanNextFile(true, pluginName);
                            std::cout << "  Scanned: " << pluginName << std::endl;
                        } else {
                            // std::cout << " Skipping next plugin: " << next << std::endl;
                            // scanner->skipNextFile();
                            scanner->scanNextFile(true, pluginName);
                        }
                    }
                }
            }
        }
        auto plugs = this->knownPlugins.getTypes();
        std::cout << "Filtering " << plugs.size() << " plugins..." << std::endl;
        std::vector<juce::PluginDescription> toInstantiate;
        for (auto plugDescription: plugs) {
            bool shouldIgnore = false;
            for (const auto ignore: config.ignore) {
                if (plugDescription.name.startsWith(ignore)) {
                    std::cout << "  Ignore: " << plugDescription.name << std::endl;
                    shouldIgnore = true;
                    break;
                }
            }
            if (shouldIgnore) { continue; }
            if (doList) {
                toInstantiate.push_back(plugDescription);
            } else {
                for (const auto pluginConfig: this->config.plugins) {
                    const auto format = plugDescription.pluginFormatName;
                    const auto name = plugDescription.name;

                    if (format.startsWith(pluginConfig->format) && name.startsWith(pluginConfig->name)) {
                        // MATCH!
                        toInstantiate.push_back(plugDescription);
                        break;
                    }
                }
            }
        }
        std::vector<juce::PluginDescription> sorted;
	if (doList) {
	  sorted = toInstantiate;
	} else {
	  for (auto plugConfig: config.plugins) {
            for (auto plugDescription: toInstantiate) {
	      if (plugDescription.name.startsWith(plugConfig->name)) {
		sorted.push_back(plugDescription);
	      }
            }
	  }
	}
        // TODO: Sort instantiations by config order
        // Instantiate the selected plugins
        for (const auto plugDescription: sorted) {
            juce::String errorMessage("barf.");
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
            for (const auto map: cfg->controlMaps) {
                for (const auto &instance: this->instances) {
                    for (const auto parameter: instance->getParameters()) {
                        if (parameter->getName(100).startsWith(map->parameterName)) {
                            this->ccMap.emplace(map->midiCC, parameter);
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
        std::cout << "MIDI Message: " << message.getDescription() << std::endl;
        if (message.isController()) {
            std::cout << std::endl << "MIDI CC: " << message.getControllerNumber() << std::endl;
            auto parameter = this->ccMap.at(message.getControllerNumber());
            if (parameter != nullptr) {
                const auto value = core::scale(message.getControllerValue(), 0, 127, 0, 1, 1);
                std::cout << "  Midi CC parameter change: " << parameter->getName(100) << ": " << value << std::endl;
                // parameter->setValue(value);
                {
                    std::cout << "  Acquiring lock to add control change to queue: " << value << std::endl;
                    std::lock_guard lock(q_mutex);
                    std::cout << "  SUCCESS acquiring lock to add control change to queue." << value << std::endl;
                    this->controlChanges.push(new ControlChange{parameter, value});
                }
            }
        }
    }

    void OLJuceHost::oscMessageReceived(const juce::OSCMessage &message) {
        std::cout << "OSC Message: size: " << message.size() << std::endl;
        std::cout << "  Address pattern: " << message.getAddressPattern().toString() << std::endl;
        for (const auto thingy: message) {
            std::cout << "  Type: " << thingy.getType() << std::endl;
            std::cout << "  Value: " << thingy.getFloat32() << std::endl;
        }
    }
}
