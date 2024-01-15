#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"
#else


#endif

#include <cstdio>
#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"


#include "daisy/io/io.h"
#include "corelib/ol_corelib.h"
#include "fxlib/ol_fxlib.h"
#include "synthlib/ol_synthlib.h"

#define AUDIO_BLOCK_SIZE 4
#define DISPLAY_ON true
#define DISPLAY_UPDATE_FREQUENCY 100
#define CHANNEL_COUNT 2
#define VOICE_COUNT 1
#define MAX_CONTROLS 5
using namespace daisy;
using namespace ol_daisy::io;
using namespace ol::fx;
using namespace ol::synth;

using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
static DaisySeed hw;
MyOledDisplay display;

daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS d1;
daisysp::DelayLine<t_sample, MAX_DELAY> DSY_SDRAM_BSS d2;
std::vector<daisysp::DelayLine<t_sample, MAX_DELAY> *> delay_lines = {&d1, &d2};

DelayFx<CHANNEL_COUNT> delay(delay_lines);
daisysp::ReverbSc DSY_SDRAM_BSS verb;
DaisyVerb<CHANNEL_COUNT> daisyVerb(verb);
ReverbFx<CHANNEL_COUNT> reverb(daisyVerb);
FilterFx<CHANNEL_COUNT> filter;

SynthVoice<1> sv1;
SynthVoice<1> sv2;
SynthVoice<1> sv3;
SynthVoice<1> sv4;

//std::vector<Voice *> voices = {&sv1, &sv2, &sv3, &sv4};
std::vector<Voice *> voices = {&sv1, &sv2, &sv3, &sv4};
Polyvoice<1> voice(voices);

class ControlListener : public VoiceControlListener {
private:
    int notes_on = 0;
public:

    void PitchCv(int channel, t_sample volts_per_octave) override {
//        auto v = voices.at(channel);
//        auto f = ol_daisy::io::cv_to_frequency(volts_per_octave);
//        v->SetFrequency(f);
    }

    void GateOn(int channel) override {
//        hw.SetLed(true);
//        auto v = voices.at(channel);
//        v->GateOn();
//        notes_on++;
    }

    void GateOff(int channel) override {
//        hw.SetLed(false);
//        auto v = voices.at(channel);
//        v->GateOff();
//        notes_on = notes_on > 0 ? notes_on - 1 : 0;
    }

    void UpdateHardwareControl(uint8_t control, t_sample value) override {
//        for (auto v: voices) {
//            v->UpdateHardwareControl(control, value);
//        }
    };
};

daisy::MidiUartHandler midi;

GpioPool<MAX_CONTROLS> gpio(hw);
ControlListener input_listener;
Control cv_1(CC_FILTER_CUTOFF);
Control cv_2(CC_FILTER_RESONANCE);
Control cv_3(CC_FILTER_DRIVE);
Control cv_4(CC_ENV_FILT_AMT);
Control cv_5(CC_ENV_FILT_D);
std::vector<Control *> controls = {&cv_1, &cv_2, &cv_3, &cv_4};
PolyvoiceControls<0, MAX_CONTROLS> inputs(gpio, controls, input_listener);

auto rms = ol::core::Rms();
t_sample rms_value = 0;
t_sample peak_value = 0;
t_sample peak_hold = 2 * 48000;
t_sample peak_count = 0;

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    inputs.Process();
    for (size_t i = 0; i < size; i += 2) {
        t_sample frame_buffer[CHANNEL_COUNT]{};
        t_sample voices_out = 0;

        voice.Process(frame_buffer);
        for (auto &f : frame_buffer) {
            f = frame_buffer[0];
        }
//        for (auto &v: voices) {
//            v->Process(frame_buffer);
//            voices_out += frame_buffer[0];
//        }

//        for (auto &f: frame_buffer) {
//            f = voices_out;
//        }

        // FX
//        delay.Process(frame_buffer, frame_buffer);
//        reverb.Process(frame_buffer, frame_buffer);
//        filter.Process(frame_buffer, frame_buffer);
//
//        // RMS and Peak
        rms_value = rms.Process(frame_buffer[0]);
        peak_value = peak_value < abs(frame_buffer[0]) ? abs(frame_buffer[0]) : peak_value;
        peak_count++;
        if (peak_count == peak_hold) {
            peak_count = 0;
            peak_value = 0;
        }

        // Write buffer to output
        for (int j = 0; j < 2; j++) {
            out[i + j] = frame_buffer[0];
        }
    }
}

int main() {

    hw.Configure();
    hw.Init();
    MidiUartHandler::Config midi_config;
    // midi_config.transport_config.rx.pin = 14;
    midi.Init(midi_config);
    gpio.Start();
    midi.StartReceive();

    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    auto sample_rate = hw.AudioSampleRate();

    voice.Init(sample_rate);
    delay.Init(sample_rate);
    reverb.Init(sample_rate);
    filter.Init(sample_rate);
    rms.Init(sample_rate, 128);


    voice.UpdateMidiControl(CC_CTL_PORTAMENTO, 0);
    voice.UpdateMidiControl(CC_FILTER_CUTOFF, 0);
    voice.UpdateMidiControl(CC_FILTER_RESONANCE, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_A, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_D, 60);
    voice.UpdateMidiControl(CC_ENV_FILT_S, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_R, 15);
    voice.UpdateMidiControl(CC_ENV_FILT_AMT, 100);

    voice.UpdateMidiControl(CC_ENV_AMP_A, 0);
    voice.UpdateMidiControl(CC_ENV_AMP_D, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_S, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_R, 25);
    voice.UpdateMidiControl(CC_OSC_1_VOLUME, 127);
    voice.UpdateMidiControl(CC_CTL_VOLUME, 100);

    // Delay defaults
    delay.UpdateMidiControl(CC_DELAY_BALANCE, 32);
    delay.UpdateMidiControl(CC_DELAY_CUTOFF, 32);
    delay.UpdateMidiControl(CC_DELAY_RESONANCE, 32);

    // Reverb defaults
    reverb.UpdateMidiControl(CC_REVERB_BALANCE, 24);
    reverb.UpdateMidiControl(CC_REVERB_TIME, 120);
    reverb.UpdateMidiControl(CC_REVERB_CUTOFF, 32);

    // FX Filter defaults
    filter.UpdateMidiControl(CC_FX_FILTER_CUTOFF, 127);
    filter.UpdateMidiControl(CC_FX_FILTER_RESONANCE, 9);

    hw.StartAudio(audio_callback);

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg = {};
    disp_cfg.driver_config.transport_config.pin_config.dc = daisy::DaisySeed::GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = daisy::DaisySeed::GetPin(30);
    /** And Initialize */
    display.Init(disp_cfg);

    int counter = 0;
    char strbuff[128];
    int direction = 1;


    int line_number;
    auto font = Font_11x18;//Font_7x10;
    auto peak_scaled = 0;
    auto rms_scaled = 0;
    bool note_on = false;
    uint8_t note_value = 0;
    while (true) {
        midi.Listen();
        while (midi.HasEvents()) {
            MidiEvent e = midi.PopEvent();
            switch (e.type) {
                case NoteOff:
                    note_on = false;
                    voice.NoteOff(e.AsNoteOff().note, e.AsNoteOff().velocity);
                    break;
                case NoteOn:
                    note_on = true;
                    note_value = e.AsNoteOn().note;
                    voice.NoteOn(note_value, e.AsNoteOn().velocity);
                    break;
                case ControlChange:
                default:
                    break;
            }
        }

        hw.SetLed(note_on);

        if (DISPLAY_ON && (counter % DISPLAY_UPDATE_FREQUENCY == 0)) {
            line_number = 0;
            display.Fill(false);

//            display.SetCursor(0, 0);
//            display.WriteString(strbuff, font, false);

            rms_scaled = int(ol::core::scale(rms_value, 0, 1, 0, 127, 1));
            display.DrawRect(0, 16, rms_scaled, 24, true, true);

            peak_scaled = int(ol::core::scale(peak_value, 0, 1, 0, 127, 1));
            display.DrawLine(peak_scaled, 16, peak_scaled, 25, true);

            display.SetCursor(0, 24);
            sprintf(strbuff, "midi: %d", note_on);
            display.WriteString(strbuff, font, true);

            display.SetCursor(0, 44);
            sprintf(strbuff, "note: %d", note_value);
            display.WriteString(strbuff, font, true);

            display.Update();
        }
        counter += direction;
        if (counter % 1000 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}