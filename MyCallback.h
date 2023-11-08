//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_MYCALLBACK_H
#define JUCE_TEST_MYCALLBACK_H

#include <juce_audio_devices/juce_audio_devices.h>

class MyCallback : public juce::AudioIODeviceCallback {
public:
    void audioDeviceIOCallback(const float **inputChannelData, int numInputChannels, float **outputChannelData,
                               int numOutputChannels, int numSamples) {
        for (int i = 0; i < numInputChannels; i++) {
            for (int j = 0; j < numSamples; j++) {
                //outputChannelData[i][j] = inputChannelData[i][j];
            }
        }

    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        std::cout << "Device about to start..." << std::endl;
    }

    void audioDeviceStopped() override {
        std::cout << "Device stopped." << std::endl;
    }

private:

};


#endif //JUCE_TEST_MYCALLBACK_H
