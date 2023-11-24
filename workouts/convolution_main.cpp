#include "juce_audio_devices/juce_audio_devices.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"
#include "juce_dsp/juce_dsp.h"

using namespace juce::dsp;

class ConvolutionCallback : public juce::AudioIODeviceCallback {
public:
    explicit ConvolutionCallback(Convolution *convolution, SynthAudioCallback *synthCallback) : synth_callback(
            synthCallback) {}

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override {
        // let the synth do its thing
        this->synth_callback->audioDeviceIOCallbackWithContext(inputChannelData, numInputChannels,
                                                               outputChannelData, numOutputChannels,
                                                               numSamples,
                                                               context);
        // Now do whatever else needs to happen
        // ...
    }

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        this->synth_callback->audioDeviceAboutToStart(device);
    }

    void audioDeviceStopped() override {
        this->synth_callback->audioDeviceStopped();
    }

private:
    SynthAudioCallback *synth_callback;
};

//
// Created by Orion Letizi on 11/24/23.
//
int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    const char *ir_file = "/Library/Audio/Impulse Responses/Big Gee's Lexicon 480L/Silver Plate.aif";
    juce::File file(ir_file);
    if (!file.existsAsFile()) {
        std::cout << "Dude. Give me a file that exists: " << ir_file << std::endl;
        exit(1);
    }
    Convolution convolution;
    convolution.loadImpulseResponse(file,
                                    Convolution::Stereo::yes,
                                    Convolution::Trim::no, 0);


    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    ol::synthlib::ControlPanel control_panel;
    ol::synthlib::Voice voice(&control_panel);
    SynthMidiCallback midi_callback(&control_panel, &voice);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }


    SynthAudioCallback synth(&voice);
    ConvolutionCallback convolution_callback(&convolution, &synth);
    deviceManager.addAudioCallback(&convolution_callback);

    std::cout << "Send me some MIDI" << std::endl;
    std::cout << "t: play test sound" << std::endl;
    std::cout << "q: quit" << std::endl;
    while (auto c = getchar()) {
        if (c == 't') {
            deviceManager.playTestSound();
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
    }
    std::cout << "Goodbye!" << std::endl;
    juce::shutdownJuce_GUI();
    return 0;
}