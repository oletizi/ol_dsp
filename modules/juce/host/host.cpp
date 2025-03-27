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
    const juce::String toLoad = "TAL"; // TODO: make this configurable
    const int scanMax = 10000;
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
}

void OLJuceHost::shutdown() {
    std::cout << "Shutdown OLJuceHost..." << std::endl;
}
