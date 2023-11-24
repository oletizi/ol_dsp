//
// Created by Orion Letizi on 11/10/23.
//

#ifndef OL_DSP_SYNTHMIDICALLBACK_H
#define OL_DSP_SYNTHMIDICALLBACK_H
#include <juce_audio_devices/juce_audio_devices.h>

class SynthMidiCallback : public juce::MidiInputCallback {
public:
    explicit SynthMidiCallback(ol::synthlib::ControlPanel *control_panel, ol::synthlib::Voice *voice) : control_panel_(control_panel), voice_(voice) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        if (message.isNoteOn()) {
            voice_->NoteOn(static_cast<unsigned char>(message.getNoteNumber()), message.getVelocity());
        } else if (message.isNoteOff()) {
            voice_->NoteOff(static_cast<unsigned char>(message.getNoteNumber()));
        } else if (message.isController()) {
            control_panel_->UpdateMidi(message.getControllerNumber(),
                                       message.getControllerValue());
        }
    }

private:
    ol::synthlib::ControlPanel *control_panel_;
    ol::synthlib::Voice *voice_;
};

#endif //OL_DSP_SYNTHMIDICALLBACK_H
