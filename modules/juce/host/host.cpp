//
// Created by Orion Letizi on 3/26/25.
//

#include "host.h"
const juce::String OLJuceHost::getApplicationName()  {
        return "JuceHello";
}

const juce::String OLJuceHost::getApplicationVersion() {
    return "0.1";
}

void OLJuceHost::initialise(const juce::String &commandLineParameters)  {
    std::cout << "Initialising OLJuceHost..." << std::endl;
}

void OLJuceHost::shutdown()  {
    std::cout << "Shutdown OLJuceHost..." << std::endl;
}
