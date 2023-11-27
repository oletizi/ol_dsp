//
// Created by Orion Letizi on 11/11/23.
//

#ifndef OL_DSP_FXMIDICALLBACK_H
#define OL_DSP_FXMIDICALLBACK_H

#include <juce_audio_devices/juce_audio_devices.h>
#include "fxlib/Fx.h"

using namespace ol::fx;

class FxMidiCallback : public juce::MidiInputCallback {
public:
    explicit FxMidiCallback(FxRack *rack) : rack_(rack) {}

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override;

private:
    FxRack *rack_;
};

#endif //OL_DSP_FXMIDICALLBACK_H
