//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_MYCALLBACK_H
#define JUCE_TEST_MYCALLBACK_H

#include "daisysp.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "synthlib/Voice.h"


class SynthAudioCallback : public juce::AudioIODeviceCallback {
private:
    uint32_t counter_ = 0;
    juce::AudioDeviceManager *device_manager_ = nullptr;
public:
    explicit SynthAudioCallback(ol::synth::Multivoice *voices) : voices_(voices) {}

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override {
        counter_++;
        for (int i = 0; i < numSamples; i++) {
            float value = voices_->Process(voices_);
            for (int j = 0; j < numOutputChannels; j++) {
                outputChannelData[j][i] = value;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        voices_->Init(voices_, static_cast<float>(device->getCurrentSampleRate()));
    }

    void audioDeviceStopped() override {
    }

private:
    ol::synth::Multivoice *voices_;
};


#endif //JUCE_TEST_MYCALLBACK_H
