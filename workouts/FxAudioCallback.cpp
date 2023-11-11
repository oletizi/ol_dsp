//
// Created by Orion Letizi on 11/10/23.
//

#include "FxAudioCallback.h"

void FxAudioCallback::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    std::cout << "Audio device about to start..." << std::endl;
}

void FxAudioCallback::audioDeviceStopped() {
    std::cout << "Audio device stopped." << std::endl;
}

void FxAudioCallback::audioDeviceIOCallbackWithContext(const float *const *inputChannelData, int numInputChannels,
                                                       float *const *outputChannelData, int numOutputChannels,
                                                       int numSamples,
                                                       const juce::AudioIODeviceCallbackContext &context) {
    for (int i=0; i<numInputChannels; i++) {
        for (int j=0; j<numSamples; j++) {
            outputChannelData[i][j] = inputChannelData[i][j];
        }
    }
}
