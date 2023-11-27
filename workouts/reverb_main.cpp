//
// Created by Orion Letizi on 11/25/23.
//
#include "juce_audio_devices/juce_audio_devices.h"
#include "SynthAudioCallback.h"
#include "SynthMidiCallback.h"
#include "fxlib/Fx.h"

class ReverbAudioCallback : public juce::AudioIODeviceCallback {
public:
    explicit ReverbAudioCallback(ol::fx::ReverbFx *fx, SynthAudioCallback *synth) :
            fx_(fx), synth_(synth) {}

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        synth_->audioDeviceAboutToStart(device);
        fx_->Init(fx_, device->getCurrentSampleRate());
    }

    /** Called to indicate that the device has stopped. */
    void audioDeviceStopped() override {
        synth_->audioDeviceStopped();
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
        synth_->audioDeviceIOCallbackWithContext(inputChannelData, numOutputChannels,
                                                 outputChannelData, numOutputChannels,
                                                 numSamples, context);
        for (int i = 0; i < numSamples; i++) {
            float in1, in2;
            float *out1, *out2;
            in1 = outputChannelData[0][i];
            in2 = outputChannelData[1][i];

            out1 = &outputChannelData[0][i];
            out2 = &outputChannelData[1][i];
            fx_->Process(fx_, in1, in2, out1, out2);
        }
    }


    ol::fx::ReverbFx *fx_;
    SynthAudioCallback *synth_;
};

class ReverbMidiCallback : public juce::MidiInputCallback {
public:
    explicit ReverbMidiCallback(ol::fx::ReverbFx *fx) : fx_(fx) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isController()) {
            std::cout << "Reverb midi: controller: " << message.getControllerNumber() << "; val: "
                      << message.getControllerValue() << std::endl;
            ol::fx::Reverb_UpdateMidiControl(fx_, message.getControllerNumber(), message.getControllerValue());
        }
    }

    ol::fx::ReverbFx *fx_;
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    juce::initialiseJuce_GUI();
    juce::AudioDeviceManager deviceManager = juce::AudioDeviceManager();
    deviceManager.initialiseWithDefaultDevices(2, 2);

    auto midiDevices = juce::MidiInput::getAvailableDevices();
    std::cout << "MIDI inputs:" << std::endl;

    ol::synthlib::ControlPanel control_panel;
    ol::synthlib::Voice voice(&control_panel);

    SynthAudioCallback synth(&voice);
    ol::fx::ReverbFx fx;

    SynthMidiCallback midi_callback(&control_panel, &voice);
    ReverbMidiCallback reverb_midi_callback(&fx);

    for (const auto &input: midiDevices) {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &midi_callback);
        deviceManager.addMidiInputDeviceCallback(input.identifier, &reverb_midi_callback);
        std::cout << " name: " << input.name << "; identifier: " << input.identifier << std::endl;
    }



    ol::fx::Dattorro_Config(&fx, DattorroVerb_create());
    ReverbAudioCallback reverb_callback(&fx, &synth);
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