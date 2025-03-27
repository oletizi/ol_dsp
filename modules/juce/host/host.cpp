//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"

const juce::String OLJuceHost::getApplicationName() {
    return "JuceHello";
}

const juce::String OLJuceHost::getApplicationVersion() {
    return "0.1";
}

void OLJuceHost::initialise(const juce::String &commandLineParameters) {
    std::cout << "Initialising OLJuceHost..." << std::endl;
    formatManager.addDefaultFormats();
    std::cout << "Num formats: " << formatManager.getNumFormats() << std::endl;
    const juce::String toLoad = "TAL"; //TAL Reverb 4 Plugin"; // TODO: make this configurable
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        constexpr int scanMax = 10000; // TODO: make this configurable
        constexpr bool recursive = true;
        juce::FileSearchPath path;
        juce::File deadMansPedalFile("~/tmp/deadPedals");
        juce::AudioPluginFormat *format = formatManager.getFormat(i);
        juce::String pluginName;
        juce::PluginDirectoryScanner *scanner = new juce::PluginDirectoryScanner(
            this->knownPlugins, *format, path, recursive, deadMansPedalFile);

        std::cout << "Scanning " << format->getName() << std::endl;
        bool more = true;
        int count = 0;
        while (more && ++count < scanMax) {
            juce::String next = scanner->getNextPluginFileThatWillBeScanned();
            more = next.length();
            if (next.contains(toLoad)) {
                scanner->scanNextFile(true, pluginName);
                std::cout << "  Scanned: " << pluginName << std::endl;
            } else {
                scanner->skipNextFile();
            }
        }
    }
    // Add plugin
    // formatManager.createPluginInstanceAsync (desc.pluginDescription,
    //                                      graph.getSampleRate(),
    //                                      graph.getBlockSize(),
    //                                      [this, pos, dpiDisabler, useARA = desc.useARA] (std::unique_ptr<AudioPluginInstance> instance, const String& error)
    //                                      {
    //                                          addPluginCallback (std::move (instance), error, pos, useARA);
    //                                      });
    const juce::Array<juce::PluginDescription> plugs = this->knownPlugins.getTypes();
    std::cout << "plugs count: " << plugs.size() << std::endl;
    for (int i = 0; i < plugs.size(); ++i) {
        const juce::PluginDescription &plugDescription = plugs[i];
        juce::String errorMessage("barf.");
        std::cout << "instantiating: " << plugDescription.name << std::endl;
        std::unique_ptr<juce::AudioPluginInstance> plug = formatManager.createPluginInstance(plugDescription, 441000, 128, errorMessage);
    }
}

void OLJuceHost::shutdown() {
    std::cout << "Shutdown OLJuceHost..." << std::endl;
}
