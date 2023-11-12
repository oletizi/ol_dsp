//
// Created by Orion Letizi on 11/12/23.
//
#include "SpyAudioSource.h"

SpyAudioSource::SpyAudioSource(juce::AudioFormatReaderSource *source) : source_(source), counter_(0) {}

void SpyAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    source_->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void SpyAudioSource::releaseResources() {
    source_->releaseResources();
}

void SpyAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
    source_->getNextAudioBlock(bufferToFill);
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


