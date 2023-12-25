#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"

#else


#endif

#include <cstdio>
#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"


#include "corelib/ol_corelib.h"
#include "fxlib/ol_fxlib.h"
#include "synthlib/ol_synthlib.h"

#define AUDIO_BLOCK_SIZE 4
#define DISPLAY_ON true
#define CHANNEL_COUNT 2

using namespace daisy;
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

//FxRack<CHANNEL_COUNT> fx(delay, reverb, filter);

t_sample cv_freq = 220;
//ol::synth::SynthVoice<CHANNEL_COUNT> voice;
std::vector<Voice *> voices = {new SynthVoice<CHANNEL_COUNT>, new SynthVoice<CHANNEL_COUNT>,
                               new SynthVoice<CHANNEL_COUNT>, new SynthVoice<CHANNEL_COUNT>};
Polyvoice<CHANNEL_COUNT> poly(voices);

auto rms = ol::core::Rms();
t_sample rms_value = 0;
t_sample peak_value = 0;
t_sample peak_hold = 2 * 48000;
t_sample peak_count = 0;

Switch cv_gate_1;
Switch cv_gate_2;
Switch cv_gate_3;
Switch cv_gate_4;
std::vector<Switch *> gates = {&cv_gate_1, &cv_gate_2, &cv_gate_3, &cv_gate_4};

t_sample frame_buffer[CHANNEL_COUNT]{};

uint8_t cv_pitch_to_midi(t_sample cv_pitch) {
    t_sample voct = daisysp::fmap(cv_pitch, 0.f, 40.f);
    t_sample coarse_tune = 30.93;
    t_sample midi_nn = daisysp::fclamp(coarse_tune + voct, 0.f, 127.f);
    return uint8_t(daisysp::mtof(midi_nn));
}

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    for (int i = 0; i < size; i += 2) {
        // CV Frequency
//        t_sample voct_cv = hw.adc.GetFloat(0);
//        t_sample voct = daisysp::fmap(voct_cv, 0.f, 40.f);
//        t_sample coarse_tune = 30.93;
//        t_sample midi_nn = daisysp::fclamp(coarse_tune + voct, 0.f, 127.f);
//        t_sample freq = daisysp::mtof(midi_nn);
//        cv_freq = freq;

        for (int j = 0; j < gates.size(); j++) {
            auto &g = gates.at(j);
            auto note = cv_pitch_to_midi(hw.adc.GetFloat(j));
            // CV Gate
            if (g->FallingEdge()) {
                poly.NoteOn(note, 100);
            }
            if (g->RisingEdge()) {
                poly.NoteOff(note, 100);
            }
        }

        // Synth voice
//        voice.SetFrequency(cv_freq);
//        voice.Process(frame_buffer);
        poly.Process(frame_buffer);

        // FX
//        fx.Process(frame_buffer, frame_buffer);
        delay.Process(frame_buffer, frame_buffer);
        reverb.Process(frame_buffer, frame_buffer);
        filter.Process(frame_buffer, frame_buffer);


        // RMS and Peak
        rms_value = rms.Process(frame_buffer[0]);
        peak_value = peak_value < abs(frame_buffer[0]) ? abs(frame_buffer[0]) : peak_value;
        peak_count++;
        if (peak_count == peak_hold) {
            peak_count = 0;
            peak_value = 0;
        }

        // Write buffer to output
        for (int j = 0; j < CHANNEL_COUNT; j++) {
            out[i + j] = frame_buffer[j];
        }
    }
}

int main() {

    hw.Configure();
    hw.Init();

    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    auto sample_rate = hw.AudioSampleRate();

    poly.Init(sample_rate);
    //fx.Init(sample_rate);
    delay.Init(sample_rate);
    reverb.Init(sample_rate);
    filter.Init(sample_rate);
    rms.Init(sample_rate, 128);

    hw.StartAudio(audio_callback);

    poly.UpdateMidiControl( CC_CTL_PORTAMENTO, 30);

    poly.UpdateMidiControl( CC_FILTER_CUTOFF, 0);
    poly.UpdateMidiControl( CC_FILTER_RESONANCE, 0);
    poly.UpdateMidiControl( CC_ENV_FILT_A, 0);
    poly.UpdateMidiControl( CC_ENV_FILT_D, 60);
    poly.UpdateMidiControl( CC_ENV_FILT_S, 0);
    poly.UpdateMidiControl( CC_ENV_FILT_R, 15);
    poly.UpdateMidiControl( CC_ENV_FILT_AMT, 24);

    poly.UpdateMidiControl( CC_ENV_AMP_A, 0);
    poly.UpdateMidiControl( CC_ENV_AMP_D, 127);
    poly.UpdateMidiControl( CC_ENV_AMP_S, 127);
    poly.UpdateMidiControl( CC_ENV_AMP_R, 25);
    poly.UpdateMidiControl( CC_OSC_1_VOLUME, 127);
    poly.UpdateMidiControl( CC_CTL_VOLUME, 100);

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

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg = {};
    disp_cfg.driver_config.transport_config.pin_config.dc = daisy::DaisySeed::GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = daisy::DaisySeed::GetPin(30);
    /** And Initialize */
    display.Init(disp_cfg);
    // printf("showtime\n");

    //DaisySeed::StartLog(false);
    int counter = 0;
    char strbuff[128];
    int direction = 1;


    // Voice 1
    int pin_number = 15;
    AdcChannelConfig cv_pitch_1_config{};
    //Configure pin 21 as an ADC input. This is where we'll read the knob.
    cv_pitch_1_config.InitSingle(hw.GetPin(pin_number));

    pin_number = 16;
    cv_gate_1.Init(hw.GetPin(pin_number), 1000);

    // Voice 2
    pin_number = 17;
    AdcChannelConfig cv_pitch_2_config{};
    cv_pitch_2_config.InitSingle(hw.GetPin(pin_number));

    pin_number = 18;
    cv_gate_2.Init(hw.GetPin(pin_number), 1000);

    // Voice 3
    pin_number = 19;
    AdcChannelConfig cv_pitch_3_config{};
    cv_pitch_3_config.InitSingle(hw.GetPin(pin_number));

    pin_number = 20;
    cv_gate_3.Init(hw.GetPin(pin_number), 1000);

    //Voice 4
    pin_number = 21;
    AdcChannelConfig cv_pitch_4_config{};
    cv_pitch_4_config.InitSingle(hw.GetPin(pin_number));

    pin_number = 22;
    cv_gate_4.Init(hw.GetPin(pin_number), 1000);



    //Initialize the adc with the config we just made
    AdcChannelConfig adc_configs[4] = {cv_pitch_1_config, cv_pitch_2_config, cv_pitch_3_config, cv_pitch_4_config};
//    adc_configs[0] = cv_pitch_config;
//    adc_configs[1] = cv_1_config;
    hw.adc.Init(adc_configs, 4);
    //hw.adc.Init(&ch1Config, 1);


    //Start reading values
    hw.adc.Start();

    int line_number;
    auto font = Font_11x18;//Font_7x10;
    auto peak_scaled = 0;
    auto rms_scaled = 0;
    t_sample cv_1_previous = 0;
    t_sample window = 0.05f;
    bool led_state = false;
    while (true) {
        led_state = false;
//        button.Debounce();
//        cv_gate_1.Debounce();
        // Set the onboard LED

        for (auto &g: gates) {
            g->Debounce();
            led_state = g->RisingEdge() || led_state;
        }
        hw.SetLed(led_state);

//        if (cv_1_previous > cv_1 + window || cv_1_previous < cv_1 - window) {
//            delay.UpdateHardwareControl(CC_DELAY_TIME, 1 - cv_1);
//            cv_1_previous = cv_1;
//        }

//        if (button.RisingEdge()) {
//            voice.NoteOn(63, 100);
//            gate = 1;
//            adsr.Retrigger(true);
//        }
//        if (button.FallingEdge()) {
//            voice.NoteOff(63, 100);
//            gate = 0;
//        }

        if (DISPLAY_ON && counter % 100 == 0) {
            line_number = 0;
            display.Fill(false);

            display.SetCursor(0, 0);
            sprintf(strbuff, "freq: %d", int(cv_freq));
            display.WriteString(strbuff, font, false);

            line_number++;
            rms_scaled = int(ol::core::scale(rms_value, 0, 1, 0, 127, 1));
            display.DrawRect(0, 16, rms_scaled, 24, true, true);

            peak_scaled = int(ol::core::scale(peak_value, 0, 1, 0, 127, 1));
            display.DrawLine(peak_scaled, 16, peak_scaled, 25, true);
            line_number++;
            display.SetCursor(0, 24);
//        sprintf(strbuff, "rms : 0.%d", int(rms_value * 10000));
            sprintf(strbuff, "peak: %d", int(peak_value * 10000));
            display.WriteString(strbuff, font, true);

//            line_number++;
//            display.SetCursor(0, 44);
//            sprintf(strbuff, "ch2: %d", int(cv_1 * 1000));
//            display.WriteString(strbuff, font, true);

            display.Update();
        }
        counter += direction;
        if (counter % 99 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}