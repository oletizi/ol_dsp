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
    juce::AudioDeviceManager * device_manager_;
public:
    explicit SynthAudioCallback(juce::AudioDeviceManager *device_manager, ol::synthlib::Voice *pVoice)
            : device_manager_(device_manager), voice_(pVoice){
        device_manager->addAudioCallback(this);
    }

    ~SynthAudioCallback() override {
        device_manager_->removeAudioCallback(this);
    }

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override {
        counter_++;
        for (int i = 0; i < numSamples; i++) {
            float value = voice_->Process();
            for (int j =0; j<numOutputChannels; j++) {
                outputChannelData[j][i] = value;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        voice_->Init(static_cast<float>(device->getCurrentSampleRate()));
    }

    void audioDeviceStopped() override {
    }

private:
    ol::synthlib::Voice *voice_;
};


#endif //JUCE_TEST_MYCALLBACK_H
