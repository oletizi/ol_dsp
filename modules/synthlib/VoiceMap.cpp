//
// Created by Orion Letizi on 12/8/23.
//
#include "VoiceMap.h"

void ol::synth::VoiceMap::NoteOn(uint8_t note, uint8_t velocity) {
    if (note < 128) {
        Voice *voice = note2voice[note];
        if (voice != nullptr) {
            voice->NoteOn(note, velocity);
        }
    }
}

void ol::synth::VoiceMap::NoteOff(uint8_t note, uint8_t velocity) {
    if (note < 128) {
        Voice *voice = note2voice[note];
        if (voice != nullptr) {
            voice->NoteOff(note, velocity);
        }
    }
}

void ol::synth::VoiceMap::SetVoice(uint8_t note, ol::synth::Voice &voice) {
    if (note < 128) {
        note2voice[note] = &voice;
    }
}

void ol::synth::VoiceMap::Init(t_sample sample_rate) {
    for (auto &v: note2voice) { if (v != nullptr) v->Init(sample_rate); }
}

void ol::synth::VoiceMap::Process(t_sample *frame_out) {
    for (auto &v: note2voice) { if (v != nullptr) v->Process(frame_out); }
}

void ol::synth::VoiceMap::UpdateMidiControl(uint8_t control, uint8_t value) {
    for (auto &v: note2voice) { if (v != nullptr) v->UpdateMidiControl(control, value); }
}
