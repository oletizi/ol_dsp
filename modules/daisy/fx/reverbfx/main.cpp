#include "daisy.h"
#include "daisy_pod.h"
#include "daisysp.h"
#include "corelib/ol_corelib.h"
#include "fxlib/ReverbControlPanel.h"
#include "fxlib/Reverb.h"

#define AUDIO_BLOCK_SIZE 4
using namespace daisy;
static DaisyPod hw;
static DSY_SDRAM_BSS ol::fx::ReverbControlPanel reverb_control_panel;
static DSY_SDRAM_BSS daisysp::ReverbSc verb;
static DSY_SDRAM_BSS ol::fx::Reverb reverb(&reverb_control_panel, &verb);


static void callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t size) {
    for (size_t i = 0; i < size; i += 2) {
        t_sample in1 = in[i];
        t_sample in2 = in[i + 1];
        t_sample out1;
        t_sample out2;
        reverb.Process(in1, in2, &out1, &out2);
        t_sample balance = reverb_control_panel.reverb_balance.Value();
        out[i] = (out1 * balance) + (in1 * (1 - balance));
        out[i + 1] = (out2 * balance) + (in2 * (1 - balance));
    }
}

static void passthrough(AudioHandle::InterleavingInputBuffer in,
                        AudioHandle::InterleavingOutputBuffer out,
                        size_t size) {
    for (size_t i = 0; i < size; i += 2) {
        out[i] = in[i];
        out[i + 1] = in[i + 1];
    }
}

void handleMidiMessage(MidiEvent m) {
    if (m.type == daisy::NoteOn) {
        NoteOnEvent n = m.AsNoteOn();
        DaisySeed::PrintLine("NoteOn : chan: %d, note: %d, vel: %d", n.channel, n.note, n.velocity);
    }
    if (m.type == daisy::NoteOff) {
        NoteOffEvent n = m.AsNoteOff();
        DaisySeed::PrintLine("NoteOff: chan: %d, note: %d, vel: %d", n.channel, n.note, n.velocity);
    }
    if (m.type == daisy::ControlChange) {
        ControlChangeEvent p = m.AsControlChange();
        DaisySeed::PrintLine("Channel: %d, cc: %d; value: %d", p.channel, p.control_number, p.value);
        uint16_t control_number = p.control_number;
        uint16_t control_value = p.value;
        reverb_control_panel.UpdateMidi(control_number, control_value);
    }
};


int main() {
    float sample_rate;
    hw.Init();
    DaisySeed::StartLog(true);
    DaisySeed::PrintLine("Hi!");
    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    sample_rate = hw.AudioSampleRate();
    hw.StartAdc();
    DaisySeed::PrintLine("starting audio...");
    reverb.Init(sample_rate);
    hw.StartAudio(callback);
    hw.StartAudio(passthrough);
    hw.midi.StartReceive();
    long count = 0;
    //fx_chain.Init(sample_rate);
    while (true) {
        // Control loop
        System::Delay(1);
        hw.midi.Listen(); // ???: Can this be done outside control loop?
        // Handle MIDI Events
        while (hw.midi.HasEvents()) {
            const MidiEvent m = hw.midi.PopEvent();
            handleMidiMessage(m);
        }
        if (count % 1000 == 0) {
            DaisySeed::PrintLine("HERE! Sample rate: %d", sample_rate);
            count = 0;
        }
        count++;
    }
}