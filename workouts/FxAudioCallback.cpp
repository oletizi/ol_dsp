//
// Created by Orion Letizi on 11/10/23.
//

#include "FxAudioCallback.h"

void FxAudioCallback::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    std::cout << "Audio device about to start..." << std::endl;
    fx_->Init(device->getCurrentSampleRate());
}

void FxAudioCallback::audioDeviceStopped() {
    std::cout << "Audio device stopped." << std::endl;
}

void FxAudioCallback::audioDeviceIOCallbackWithContext(const float *const *inputChannelData, int numInputChannels,
                                                       float *const *outputChannelData, int numOutputChannels,
                                                       int numSamples,
                                                       const juce::AudioIODeviceCallbackContext &context) {
    int errors = 0;
    int successes = 0;

    for (int i = 0; i < numSamples; i++) {
        count_++;
        if (fx_->Process(inputChannelData[0][i],
                         inputChannelData[1][i],
                         &outputChannelData[0][i],
                         &outputChannelData[1][i])) {
            outputChannelData[0][i] = inputChannelData[0][i];
            outputChannelData[1][i] = inputChannelData[1][i];
            errors++;
        } else {
            successes++;
        }
    }

//    for (int i = 0; i< numSamples; i++) {
//        outputChannelData[0][i] = inputChannelData[0][i];
//        outputChannelData[1][i] = inputChannelData[0][i];
//    }

    if (count_ % 4000 == 0) {
        std::cout << "num samples: " << numSamples << std::endl;
        std::cout << "verb errors: " << errors << "; successes: " << successes << std::endl;
        count_ = 0;
    }
}
