//
// Created by Orion Letizi on 11/25/23.
//
#include "juce_audio_devices/juce_audio_devices.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"
#include "fxlib/Fx.h"
#include "synthlib/ol_synthlib.h"

#define CHANNEL_COUNT 1
#define VOICE_COUNT 1

using namespace ol::fx;
using namespace ol::synth;

class ReverbAudioCallback : public juce::AudioIODeviceCallback {
public:
    explicit ReverbAudioCallback(ol::fx::ReverbFx<CHANNEL_COUNT> *fx, SynthAudioCallback<CHANNEL_COUNT, VOICE_COUNT> &synth) :
            fx_(fx), synth_(synth) {}

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        synth_.audioDeviceAboutToStart(device);
        fx_->Init(device->getCurrentSampleRate());
    }

    /** Called to indicate that the device has stopped. */
    void audioDeviceStopped() override {
        synth_.audioDeviceStopped();
    };

    /** This can be overridden to be told if the device generates an error while operating.
        Be aware that this could be called by any thread! And not all devices perform
        this callback.
    */
    void audioDeviceError(const juce::String &errorMessage) override {}

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context)
    override {
        synth_.audioDeviceIOCallbackWithContext(inputChannelData, numOutputChannels,
                                                outputChannelData, numOutputChannels,
                                                numSamples, context);

        t_sample input_buffer[] = {0, 0};
        t_sample output_buffer[] = {0, 0};
        for (int i = 0; i < numSamples; i++) {
            float in1, in2;
            float *out1, *out2;
            input_buffer[0] = outputChannelData[0][i];
            input_buffer[1] = outputChannelData[1][i];

            fx_->Process(input_buffer, output_buffer);
            outputChannelData[0][i] = output_buffer[0];
            outputChannelData[1][i] = output_buffer[1];
        }
    }


    ol::fx::ReverbFx<CHANNEL_COUNT> *fx_;
    SynthAudioCallback<CHANNEL_COUNT, VOICE_COUNT> &synth_;
};

class ReverbMidiCallback : public juce::MidiInputCallback {
public:
    explicit ReverbMidiCallback(ol::fx::ReverbFx<CHANNEL_COUNT> *fx) : fx_(fx) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isController()) {
            std::cout << "Reverb midi: controller: " << message.getControllerNumber() << "; val: "
                      << message.getControllerValue() << std::endl;
            //ol::fx::Reverb_UpdateMidiControl(fx_, message.getControllerNumber(), message.getControllerValue());
            fx_->UpdateMidiControl(message.getControllerNumber(), message.getControllerValue());
        }
    }

    ReverbFx<CHANNEL_COUNT> *fx_;
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    Polyvoice<CHANNEL_COUNT, VOICE_COUNT> poly;

    auto synthCallback = SynthAudioCallback(poly);

    ReverbFx<CHANNEL_COUNT> reverb;

    auto midi_callback = SynthMidiCallback(poly);
    ReverbMidiCallback reverb_midi_callback(&reverb);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &reverb_midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }



    ReverbAudioCallback reverb_callback(&reverb, synthCallback);
    deviceManager.addAudioCallback(&reverb_callback);

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