//
// Created by Orion Letizi on 11/12/23.
//
#include "SpyAudioSource.h"

SpyAudioSource::SpyAudioSource(juce::AudioFormatReaderSource *source) : source_(source), counter_(0), processed_(0) {}

void SpyAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    source_->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void SpyAudioSource::releaseResources() {
    source_->releaseResources();
}

void SpyAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
    source_->getNextAudioBlock(bufferToFill);
    processed_ += bufferToFill.numSamples;
    counter_++;
    if (processed_ >= source_->getTotalLength()) {
        counter_ = 0;
        processed_ = 0;
    }
    if (counter_ % 100 == 0) {
        std::cout << "Buffer size: " << bufferToFill.numSamples << std::endl;
        std::cout << "  Buffers processed: " << counter_ << std::endl;
        std::cout << "  Samples processed: " << processed_ << std::endl;
        std::cout << "  Total length     : " << source_->getTotalLength() << std::endl;
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


