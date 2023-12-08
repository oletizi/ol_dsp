//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_MYCALLBACK_H
#define JUCE_TEST_MYCALLBACK_H

#include "daisysp.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "synthlib/ol_synthlib.h"


class SynthAudioCallback : public juce::AudioIODeviceCallback {
private:
    uint32_t counter_ = 0;
    juce::AudioDeviceManager *device_manager_ = nullptr;
public:
    explicit SynthAudioCallback(ol::synth::Polyvoice &voices) : poly_(voices) {}

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override {
        counter_++;
        t_sample output_buffer = 0;
        for (int i = 0; i < numSamples; i++) {
            poly_.Process(&output_buffer);
            for (int j = 0; j < numOutputChannels; j++) {
                outputChannelData[j][i] = output_buffer;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        poly_.Init(static_cast<float>(device->getCurrentSampleRate()));
    }

    void audioDeviceStopped() override {
    }

private:
    ol::synth::Polyvoice poly_;
};


#endif //JUCE_TEST_MYCALLBACK_H
