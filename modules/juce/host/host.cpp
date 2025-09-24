//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"
#include <ol_corelib.h>
#include <fstream>
#include <string>
#include <iomanip>

#define DEBUG 0

namespace ol::jucehost {
    const juce::String OLJuceHost::getApplicationName() {
        return "JuceHello";
    }

    const juce::String OLJuceHost::getApplicationVersion() {
        return "0.5";
    }

    juce::String parseConfigValue(const juce::String &line, const juce::String &startToken) {
        if (const auto index = line.indexOf(startToken); index >= 0) {
            const auto start = index + startToken.length();
            const auto end = line.substring(start).indexOf(">") + start;
            return line.substring(start, end);
        }
        return "";
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
            const auto osc = parseConfigValue(line, "<OSC: ");
            std::cout << "OSC Config: " << osc << std::endl;
            PluginConfig *plug = nullptr;
            for (const auto test: config.plugins) {
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
            const auto controlMap = new ControlMapConfig{.parameterName = parameterName, .midiCC = cc, .oscPath = osc};
            plug->controlMaps.push_back(controlMap);
        }
    }

    void OLJuceHost::initialise(const juce::String &commandLineParameters) {
        std::cout << commandLineParameters << std::endl;
        std::cout << "Initialising OLJuceHost..." << std::endl;
        const bool doList = commandLineParameters.contains("--list");
        const bool doInterrogate = commandLineParameters.contains("--interrogate");
        const bool outputJson = commandLineParameters.contains("--json");
        const bool showHelp = commandLineParameters.contains("--help");

        // Extract plugin name for interrogation
        juce::String interrogatePluginName;
        if (doInterrogate) {
            auto args = juce::StringArray::fromTokens(commandLineParameters, true);
            for (int i = 0; i < args.size() - 1; i++) {
                if (args[i] == "--interrogate") {
                    interrogatePluginName = args[i + 1];
                    break;
                }
            }
        }

        if (showHelp) {
            std::cout << "Usage: plughost [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help                    Show this help message" << std::endl;
            std::cout << "  --list                    List all available plugins" << std::endl;
            std::cout << "  --interrogate <name>      Extract parameters from specific plugin" << std::endl;
            std::cout << "  --json                    Output in JSON format (use with --interrogate)" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  plughost --list" << std::endl;
            std::cout << "  plughost --interrogate \"Jup-8 V3\"" << std::endl;
            std::cout << "  plughost --interrogate \"Jup-8 V3\" --json" << std::endl;
            quit();
            return;
        }

        const juce::String configDir = juce::String(std::getenv("HOME")) + "/.config/plughost";
        if (!doList && !doInterrogate) {
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
            constexpr int port = 3819;
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

            // Add default plugin search paths when in interrogation mode
            if (doInterrogate || doList) {
                if (format->getName() == "AudioUnit") {
                    path.add(juce::File("/Library/Audio/Plug-Ins/Components"));
                    path.add(juce::File("~/Library/Audio/Plug-Ins/Components").getFullPathName());
                } else if (format->getName() == "VST3") {
                    path.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
                    path.add(juce::File("~/Library/Audio/Plug-Ins/VST3").getFullPathName());
                    path.add(juce::File("/usr/local/lib/vst3"));
                } else if (format->getName() == "VST") {
                    path.add(juce::File("/Library/Audio/Plug-Ins/VST"));
                    path.add(juce::File("~/Library/Audio/Plug-Ins/VST").getFullPathName());
                }
            }

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

                if ((doList || doInterrogate) && !shouldIgnore) {
                    // For listing or interrogation, scan all plugins
                    scanner->scanNextFile(true, pluginName);
                    if (doList) {
                        std::cout << "Next Plugin: <Format:" << format->getName() << ">, <Name: " << pluginName << ">" <<
                                std::endl;
                    }
                } else if (!shouldIgnore) {
                    for (const auto pluginConfig: config.plugins) {
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
        auto plugs = knownPlugins.getTypes();
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
            } else if (doInterrogate) {
                // For interrogation mode, match by plugin name
                if (plugDescription.name.contains(interrogatePluginName)) {
                    toInstantiate.push_back(plugDescription);
                    std::cout << "Found plugin for interrogation: " << plugDescription.name << std::endl;
                }
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
        if (doList || doInterrogate) {
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
                plugDescription, 48000, 128, errorMessage);
            if (plug != nullptr) {
                std::cout << "Plugin: <Format: " << plugDescription.pluginFormatName << ">, <Name: " << plug->getName() << ">" << std::endl;

                if (doInterrogate) {
                    // Enhanced parameter extraction for interrogation mode
                    auto parameters = plug->getParameters();

                    if (outputJson) {
                        // Output JSON format
                        std::cout << "{" << std::endl;
                        std::cout << "  \"plugin\": {" << std::endl;
                        std::cout << "    \"manufacturer\": \"" << plugDescription.manufacturerName << "\"," << std::endl;
                        std::cout << "    \"name\": \"" << plug->getName() << "\"," << std::endl;
                        std::cout << "    \"version\": \"" << plugDescription.version << "\"," << std::endl;
                        std::cout << "    \"format\": \"" << plugDescription.pluginFormatName << "\"," << std::endl;
                        std::cout << "    \"uid\": \"" << plugDescription.uniqueId << "\"" << std::endl;
                        std::cout << "  }," << std::endl;
                        std::cout << "  \"metadata\": {" << std::endl;
                        std::cout << "    \"parameter_count\": " << parameters.size() << "," << std::endl;
                        std::cout << "    \"extracted_by\": \"plughost\"," << std::endl;
                        std::cout << "    \"timestamp\": \"" << juce::Time::getCurrentTime().toISO8601(false) << "\"" << std::endl;
                        std::cout << "  }," << std::endl;
                        std::cout << "  \"parameters\": [" << std::endl;

                        for (int i = 0; i < parameters.size(); ++i) {
                            const auto parameter = parameters[i];
                            std::cout << "    {" << std::endl;
                            std::cout << "      \"index\": " << i << "," << std::endl;
                            std::cout << "      \"name\": \"" << parameter->getName(100) << "\"," << std::endl;
                            std::cout << "      \"label\": \"" << parameter->getLabel() << "\"," << std::endl;
                            std::cout << "      \"text\": \"" << parameter->getText(parameter->getValue(), 100) << "\"," << std::endl;
                            std::cout << "      \"default_value\": " << parameter->getDefaultValue() << "," << std::endl;
                            std::cout << "      \"current_value\": " << parameter->getValue() << "," << std::endl;
                            std::cout << "      \"automatable\": " << (parameter->isAutomatable() ? "true" : "false") << "," << std::endl;
                            std::cout << "      \"meta_parameter\": " << (parameter->isMetaParameter() ? "true" : "false") << "," << std::endl;
                            std::cout << "      \"discrete\": " << (parameter->isDiscrete() ? "true" : "false") << std::endl;
                            std::cout << "    }" << (i < parameters.size() - 1 ? "," : "") << std::endl;
                        }

                        std::cout << "  ]" << std::endl;
                        std::cout << "}" << std::endl;
                    } else {
                        // Human-readable format
                        std::cout << std::endl << "=== Parameter Interrogation Report ===" << std::endl;
                        std::cout << "Plugin: " << plug->getName() << " (" << plugDescription.pluginFormatName << ")" << std::endl;
                        std::cout << "Manufacturer: " << plugDescription.manufacturerName << std::endl;
                        std::cout << "Parameter Count: " << parameters.size() << std::endl;
                        std::cout << std::endl;

                        for (int i = 0; i < parameters.size(); ++i) {
                            const auto parameter = parameters[i];
                            std::cout << "Parameter " << std::setw(2) << i << ": " << parameter->getName(100) << std::endl;
                            std::cout << "    Label: " << parameter->getLabel() << std::endl;
                            std::cout << "    Current: " << parameter->getText(parameter->getValue(), 100) << " (" << parameter->getValue() << ")" << std::endl;
                            std::cout << "    Default: " << parameter->getDefaultValue() << std::endl;
                            std::cout << "    Automatable: " << (parameter->isAutomatable() ? "Yes" : "No") << std::endl;
                            std::cout << "    Discrete: " << (parameter->isDiscrete() ? "Yes" : "No") << std::endl;
                            std::cout << std::endl;
                        }
                    }
                } else {
                    // Original format for backward compatibility
                    for (const auto parameter: plug->getParameters()) {
                        std::cout << "Plugin Parameter: <Format: " << plugDescription.pluginFormatName <<
                                ">, <Plugin Name: " << plug->getName() << ">, <Parameter Name: " <<
                                parameter->getName(100) << ">" << std::endl;
                    }
                }
                instances.push_back(std::move(plug));
            }
        }

        if (doList || doInterrogate) {
            quit();
            return;
        }
        mapControls();
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


        for (const auto &plug: instances) {
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

    void OLJuceHost::mapControls() {
        std::cout << "Map controls..." << std::endl;
        for (const auto cfg: this->config.plugins) {
            for (const auto map: cfg->controlMaps) {
                for (const auto &instance: instances) {
                    for (const auto parameter: instance->getParameters()) {
                        if (parameter->getName(100).startsWith(map->parameterName)) {
                            std::cout << "Adding to control map: oscPath: " << map->oscPath << ", cc: " << map->midiCC
                                    << "; Parameter: " << parameter->getName(100) << std::endl;
                            oscMap.emplace(map->oscPath, parameter);
                            ccMap.emplace(map->midiCC, parameter);
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
            auto parameter = ccMap.at(message.getControllerNumber());
            if (parameter != nullptr) {
                const auto value = core::scale(message.getControllerValue(), 0, 127, 0, 1, 1);
                std::cout << "  Midi CC parameter change: " << parameter->getName(100) << ": " << value << std::endl;
                // parameter->setValue(value);
                {
                    std::cout << "  Acquiring lock to add control change to queue: " << value << std::endl;
                    std::lock_guard lock(q_mutex);
                    std::cout << "  SUCCESS acquiring lock to add control change to queue." << value << std::endl;
                    controlChanges.push(new ControlChange{parameter, value});
                }
            }
        }
    }

    void OLJuceHost::oscMessageReceived(const juce::OSCMessage &message) {
        std::cout << "OSC Message: size: " << message.size() << std::endl;
        if (message.isEmpty()) {
            return;
        }
        std::cout << "  Address pattern: " << message.getAddressPattern().toString() << std::endl;
        const auto pattern = message.getAddressPattern();
        for (const auto &[path, parameter]: oscMap) {
                std::cout << "  Checking: " << path << " for " << parameter->getName(100) << std::endl;
            if (path.length() > 0 && pattern.matches(path)) {
                std::cout << "  Matches: " << path << " for " << parameter->getName(100) << std::endl;
                const auto arg = message.begin();
                if (arg->isFloat32()) {
                    const auto value = arg->getFloat32();
                    std::cout << "  Float value: " << value << std::endl; {
                        std::cout << "  Acquiring lock to add control change to queue: " << value << std::endl;
                        std::lock_guard lock(q_mutex);
                        std::cout << "  SUCCESS acquiring lock to add control change to queue." << value << std::endl;
                        this->controlChanges.push(new ControlChange{parameter, value});
                    }
                } else {
                    std::cout << "  OSC argument is not a float: " << arg->getType() << std::endl;
                }
            }
        }
    }
}
