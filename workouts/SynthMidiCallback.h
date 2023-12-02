//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_SYNTHMIDICALLBACK_H
#define OL_DSP_SYNTHMIDICALLBACK_H
#include <juce_audio_devices/juce_audio_devices.h>

class SynthMidiCallback : public juce::MidiInputCallback {
public:
    explicit SynthMidiCallback(ol::synth::Multivoice *voices) : voices_(voices) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isNoteOn()) {
            voices_->NoteOn(voices_, static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            voices_->NoteOff(voices_, static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isController()) {
            voices_->UpdateMidiControl(voices_, message.getControllerNumber(),
                                       message.getControllerValue());
        }
    }

private:
    ol::synth::Multivoice *voices_;
};

#endif //OL_DSP_SYNTHMIDICALLBACK_H
