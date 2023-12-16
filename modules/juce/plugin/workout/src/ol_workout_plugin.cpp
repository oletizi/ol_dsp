//
// Created by Orion Letizi on 12/15/23.
//
#include "PluginProcessor.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}