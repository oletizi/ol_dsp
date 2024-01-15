//
// Created by Orion Letizi on 11/29/23.
//

// This is a dummy implementation to get CMake to work outside the daisy environment.
#include "daisy_dummy.h"

void daisy::Led::SetRed(float) {}

void daisy::Led::SetGreen(float) {}

void daisy::Led::SetBlue(float) {}

bool daisy::Button::RisingEdge() { return true; }

void daisy::Knob::Process() {}

float daisy::Knob::Value() { return 0; }

int daisy::System::GetNow() { return 0; }

void daisy::System::Delay(int) {}

daisy::NoteOnEvent daisy::MidiEvent::AsNoteOn() { return NoteOnEvent{}; }

daisy::NoteOffEvent daisy::MidiEvent::AsNoteOff() { return NoteOffEvent{}; }

daisy::ControlChangeEvent daisy::MidiEvent::AsControlChange() { return ControlChangeEvent(); }

void daisy::MidiUartHandler::StartReceive() {}

void daisy::MidiUartHandler::Listen() {}

bool daisy::MidiUartHandler::HasEvents() { return false; }

daisy::MidiEvent daisy::MidiUartHandler::PopEvent() { return {}; }

void daisy::DaisyPod::Init() {}

void daisy::DaisyPod::UpdateLeds() {}

void daisy::DaisyPod::ProcessAllControls() {}

void daisy::DaisyPod::SetAudioBlockSize(int) {}

float daisy::DaisyPod::AudioSampleRate() { return 0; }

void daisy::DaisyPod::StartAdc() {}

void daisy::DaisyPod::StartAudio(daisy::AudioHandle::InterleavingAudioCallback cb) {}

void daisy::DaisyPod::StartAudio(daisy::AudioHandle::AudioCallback cb) {}

void daisy::DaisySeed::StartLog(bool b) {}

void daisy::DaisySeed::PrintLine(const char *message) {}