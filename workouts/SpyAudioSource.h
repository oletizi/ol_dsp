//
// Created by Orion Letizi on 11/12/23.
//

#ifndef OL_DSP_SPYAUDIOSOURCE_H
#define OL_DSP_SPYAUDIOSOURCE_H

#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "SpyAudioSource.h"

class SpyAudioSource : public juce::PositionableAudioSource {
public:
    explicit SpyAudioSource(juce::AudioFormatReaderSource *source);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;

    void setNextReadPosition(juce::int64 newPosition) override;

    [[nodiscard]] juce::int64 getNextReadPosition() const override;

    [[nodiscard]] juce::int64 getTotalLength() const override;

    [[nodiscard]] bool isLooping() const override;

private:
    uint64_t counter_;
    juce::AudioFormatReaderSource *source_;
};

#endif //OL_DSP_SPYAUDIOSOURCE_H
