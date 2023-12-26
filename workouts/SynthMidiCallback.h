//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_SYNTHMIDICALLBACK_H
#define OL_DSP_SYNTHMIDICALLBACK_H

#include <juce_audio_devices/juce_audio_devices.h>

template<int CHANNEL_COUNT>
class SynthMidiCallback : public juce::MidiInputCallback {
private:
    ol::synth::Voice *voice_;

public:
    explicit SynthMidiCallback(ol::synth::Voice *voice) : voice_(voice) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        //std::cout << "MIDI!" << std::endl;
        if (message.isNoteOn()) {
            voice_->NoteOn(static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            voice_->NoteOff(static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isController()) {
            voice_->UpdateMidiControl(message.getControllerNumber(), message.getControllerValue());
        }
    }


};

#endif //OL_DSP_SYNTHMIDICALLBACK_H
