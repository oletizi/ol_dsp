//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_MYCALLBACK_H
#define JUCE_TEST_MYCALLBACK_H

#include <daisysp.h>
#include <juce_audio_devices/juce_audio_devices.h>

class MyCallback : public juce::AudioIODeviceCallback {
private:
    uint32_t counter_ = 0;
    juce::AudioDeviceManager * device_manager_;
    daisysp::Oscillator osc_;
public:
    explicit MyCallback(juce::AudioDeviceManager *device_manager) : device_manager_(device_manager){
        std::cout << "MyCallback ctor" << std::endl;
        device_manager->addAudioCallback(this);
    }

    ~MyCallback() override {
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
            float value = osc_.Process();
            for (int j =0; j<numOutputChannels; j++) {
                outputChannelData[j][i] = value;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        std::cout << "Device about to start..." << std::endl;
        osc_.Init(static_cast<float>(device->getCurrentSampleRate()));
        osc_.SetFreq(440);
        osc_.SetAmp(1);
        osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    }

    void audioDeviceStopped() override {
        std::cout << "Device stopped." << std::endl;
    }

private:

};


#endif //JUCE_TEST_MYCALLBACK_H
