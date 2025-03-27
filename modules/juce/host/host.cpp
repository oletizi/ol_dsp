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
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        constexpr bool recursive = true;
        juce::FileSearchPath path;
        juce::File deadMansPedalFile("~/tmp/deadPedals");
        juce::AudioPluginFormat *format = formatManager.getFormat(i);
        juce::KnownPluginList list;
        juce::String pluginName;
        juce::PluginDirectoryScanner *scanner = new juce::PluginDirectoryScanner(
            list, *format, path, recursive, deadMansPedalFile);

        std::cout << "Scanning " << format->getName() << std::endl;
        while (scanner->scanNextFile(true, pluginName)) {
            std::cout << "  Scanned: " << pluginName << std::endl;
        }
    }
}

void OLJuceHost::shutdown() {
    std::cout << "Shutdown OLJuceHost..." << std::endl;
}
