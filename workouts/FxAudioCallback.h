//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_FXAUDIOCALLBACK_H
#define OL_DSP_FXAUDIOCALLBACK_H

#include <juce_audio_devices/juce_audio_devices.h>
#include "ol_fxlib.h"
#include "FxChain.h"

using namespace ol::fxlib;
class FxAudioCallback : public juce::AudioIODeviceCallback {
public:
    explicit FxAudioCallback(juce::AudioDeviceManager *device_manager, FxChain *fx)
            : device_manager_(device_manager), fx_(fx){
        device_manager->addAudioCallback(this);
    }
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override;

private:
    juce::AudioDeviceManager *device_manager_;
    FxChain *fx_;
    uint64_t count_;
};


#endif //OL_DSP_FXAUDIOCALLBACK_H
