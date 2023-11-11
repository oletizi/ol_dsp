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
    count_++;
    int errors = 0;
    int successes = 0;
    if (numInputChannels > 0 && numOutputChannels > 0) {
        for (int i = 0; i < numSamples; i++) {
            float in1, in2;
            float *out1, *out2;
            in1 = inputChannelData[0][i];
            in2 = numInputChannels == 1 ? in1 : inputChannelData[1][i];

            out1 = &outputChannelData[0][i];
            out2 = numOutputChannels == 1 ? out1 : &outputChannelData[1][i];

            if (fx_->Process(in1, in2, out1, out2)) {
                outputChannelData[0][i] = inputChannelData[0][i];
                outputChannelData[1][i] = inputChannelData[1][i];
                errors++;
            } else {
                successes++;
            }
        }
    }

    if (count_ % 400 == 0) {
        std::cout << "input channels: " << numInputChannels << ";, output channels: " << numOutputChannels << std::endl;
        std::cout << "num samples: " << numSamples << std::endl;
        std::cout << "verb errors: " << errors << "; successes: " << successes << std::endl;
        count_ = 0;
    }
}
