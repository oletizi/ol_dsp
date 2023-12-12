//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_SYNTHMIDICALLBACK_H
#define OL_DSP_SYNTHMIDICALLBACK_H

#include <juce_audio_devices/juce_audio_devices.h>

template<int CHANNEL_COUNT, int VOICE_COUNT>
class SynthMidiCallback : public juce::MidiInputCallback {
private:
    ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> &poly_;

public:
    explicit SynthMidiCallback(ol::synth::Polyvoice<CHANNEL_COUNT, VOICE_COUNT> &poly) : poly_(poly) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        std::cout << "MIDI!" << std::endl;
        if (message.isNoteOn()) {
            poly_.NoteOn(static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            poly_.NoteOff(static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isController()) {
            poly_.UpdateMidiControl(message.getChannel(), message.getControllerNumber(),
                                    message.getControllerValue());
        }
    }


};

#endif //OL_DSP_SYNTHMIDICALLBACK_H
