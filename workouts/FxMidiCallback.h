//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_FXMIDICALLBACK_H
#define OL_DSP_FXMIDICALLBACK_H
#include <juce_audio_devices/juce_audio_devices.h>
#include "ol_fxlib.h"
using namespace ol::fx;

class FxMidiCallback : public juce::MidiInputCallback {
public:
    explicit FxMidiCallback(FxControlPanel* control_panel);
    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;
private:
    FxControlPanel* control_panel_;
};
#endif //OL_DSP_FXMIDICALLBACK_H
