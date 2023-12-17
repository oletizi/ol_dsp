//
// Created by Orion Letizi on 12/15/23.
//

#ifndef OL_DSP_PLUGINPROCESSOR_H
#define OL_DSP_PLUGINPROCESSOR_H
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "synthlib/ol_synthlib.h"
#include "fxlib/ol_fxlib.h"

#define CHANNEL_COUNT 2

using namespace ol::synth;
using namespace ol::fx;
class PluginProcessor : public juce::AudioProcessor {
private:
    daisysp::Oscillator osc;
    SynthVoice<CHANNEL_COUNT> voice;
    FxRack<CHANNEL_COUNT> fx;
    t_sample ibuf[128] {};
    t_sample obuf[128] {};
public:

    const juce::String getName() const override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;

    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    double getTailLengthSeconds() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

};


#endif //OL_DSP_PLUGINPROCESSOR_H
