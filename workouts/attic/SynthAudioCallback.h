//
// Created by Orion Letizi on 11/8/23.
//

#ifndef JUCE_TEST_MYCALLBACK_H
#define JUCE_TEST_MYCALLBACK_H

#include "daisysp.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "synthlib/ol_synthlib.h"

template<int CHANNEL_COUNT>
class SynthAudioCallback : public juce::AudioIODeviceCallback {
private:
    ol::synth::Voice *voice_;
    uint32_t counter_ = 0;
    t_sample frame_buffer_[CHANNEL_COUNT]{};
    juce::AudioDeviceManager *device_manager_ = nullptr;
public:
    explicit SynthAudioCallback(ol::synth::Voice *voice) : voice_(voice) {}

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override {
        counter_++;
        for (int i = 0; i < numSamples; i++) {
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                frame_buffer_[i] = 0;
            }
            voice_->Process(frame_buffer_);
            for (int j = 0; j < numOutputChannels; j++) {
                outputChannelData[j][i] = frame_buffer_[j];
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        voice_->Init(static_cast<float>(device->getCurrentSampleRate()));
        voice_->UpdateMidiControl(CC_FILTER_CUTOFF, 0);
        voice_->UpdateMidiControl(CC_FILTER_RESONANCE, 24);
        voice_->UpdateMidiControl(CC_ENV_FILT_D, 64);
        voice_->UpdateMidiControl(CC_ENV_FILT_S, 0);
        voice_->UpdateMidiControl(CC_ENV_FILT_AMT, 64);
    }

    void audioDeviceStopped() override {
    }


};


#endif //JUCE_TEST_MYCALLBACK_H
