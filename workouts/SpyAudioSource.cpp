//
// Created by Orion Letizi on 11/12/23.
//
#include "SpyAudioSource.h"

SpyAudioSource::SpyAudioSource(ol::perflib::Profile *profile, ol::fx::FxChain *fx,
                               juce::AudioFormatReaderSource *source) :
        profile_(profile),
        fx_(fx), source_(source),
        counter_(0),
        processed_(0) {}

void SpyAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    source_->prepareToPlay(samplesPerBlockExpected, sampleRate);
    fx_->Init(sampleRate);
}

void SpyAudioSource::releaseResources() {
    source_->releaseResources();
}

void SpyAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
    source_->getNextAudioBlock(bufferToFill);
    processed_ += bufferToFill.numSamples;
    counter_++;
    if (processed_ >= source_->getTotalLength()) {
        processed_ = 0;
    }
    //const juce::AudioBuffer<float> *buf = bufferToFill.buffer;
    const int start_sample = bufferToFill.startSample;

    for (int i = start_sample; i < bufferToFill.numSamples; i++) {
        const float in1 = bufferToFill.buffer->getSample(0, i);
        const float in2 = bufferToFill.buffer->getSample(1, i);
        float *out1 = bufferToFill.buffer->getWritePointer(0, i);
        float *out2 = bufferToFill.buffer->getWritePointer(1, i);
        fx_->Process(in1, in2, out1, out2);
    }
    if (counter_ % 100 == 0) {
        std::cout << "Count: " << counter_ << std::endl;
        std::cout << "  Max execution time    : " << profile_->MaxExecutionTime() << std::endl;
        std::cout << "  Average execution time: " << profile_->AverageExecutionTime() << std::endl;
        std::cout << "  Max input value       :  " << profile_->MaxIn1Value() << std::endl;
        std::cout << "  Min input value       : " << profile_->MinIn1Value() << std::endl;
        std::cout << "  Max output value      :  " << profile_->MaxOut1Value() << std::endl;
        std::cout << "  Min output value      : " << profile_->MinOut1Value() << std::endl;
        std::cout << "  Max delay input value :  " << profile_->MaxVal1Value() << std::endl;
        std::cout << "  Min delay output value: " << profile_->MinVal1Value() << std::endl;
        std::cout << "  Avg abs dely in value :  " << profile_->AvgVal1Value() << std::endl;
    }
}

void SpyAudioSource::setNextReadPosition(juce::int64 newPosition) {
    source_->setNextReadPosition(newPosition);
}

juce::int64 SpyAudioSource::getNextReadPosition() const {
    return source_->getNextReadPosition();
}

juce::int64 SpyAudioSource::getTotalLength() const {
    return source_->getTotalLength();
}

bool SpyAudioSource::isLooping() const {
    return source_->isLooping();
}