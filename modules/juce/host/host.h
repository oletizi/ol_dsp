//
// Created by Orion Letizi on 3/26/25.
//
#pragma once
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>

#ifndef HOST_H
#define HOST_H
class OLJuceHost : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override;

    const juce::String getApplicationVersion() override;

    void initialise(const juce::String &commandLineParameters) override;

    void shutdown() override;
};
#endif //HOST_H
START_JUCE_APPLICATION(OLJuceHost)