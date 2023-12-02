//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_SYNTHMIDICALLBACK_H
#define OL_DSP_SYNTHMIDICALLBACK_H
#include <juce_audio_devices/juce_audio_devices.h>

class SynthMidiCallback : public juce::MidiInputCallback {
public:
    explicit SynthMidiCallback(ol::synth::Voice *voice) : voice_(voice) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isNoteOn()) {
            voice_->NoteOn(voice_, static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            voice_->NoteOff(voice_, static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isController()) {
            voice_->UpdateMidiControl(voice_, message.getControllerNumber(),
                                       message.getControllerValue());
        }
    }

private:
    ol::synth::Voice *voice_;
};

#endif //OL_DSP_SYNTHMIDICALLBACK_H
