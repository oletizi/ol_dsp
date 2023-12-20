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
#define CHANNEL_COUNT 2
using namespace daisy;

using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
static DaisySeed hw;
MyOledDisplay display;

bool display_on = true;
//t_sample gate = 0;
t_sample cv_freq = 220;

auto voice = ol::synth::SynthVoice<CHANNEL_COUNT>();

auto rms = ol::core::Rms();
t_sample rms_value = 0;
t_sample peak_value = 0;
t_sample peak_hold = 2 * 48000;
t_sample peak_count = 0;

Switch cv_gate;

t_sample frame_buffer[CHANNEL_COUNT]{};

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    for (int i = 0; i < size; i += 2) {

        frame_buffer[i] = 0;
        frame_buffer[i + 1] = 0;

        // CV Frequency
        t_sample voct_cv = hw.adc.GetFloat(0);
        t_sample voct = daisysp::fmap(voct_cv, 0.f, 40.f);
        t_sample coarse_tune = 30.93;
        t_sample midi_nn = daisysp::fclamp( coarse_tune + voct, 0.f, 127.f);
        t_sample freq = daisysp::mtof(midi_nn);
        cv_freq    = freq;

        // CV Gate
        if (cv_gate.FallingEdge()) {
            voice.GateOn();
        }
        if (cv_gate.RisingEdge()) {
            voice.GateOff();
        }

        // Synth voice
        voice.SetFrequency(cv_freq);
        voice.Process(frame_buffer);

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
    hw.StartAudio(audio_callback);
    auto sample_rate = hw.AudioSampleRate();

    ol::synth::Voice::Config vc = {};
    vc.filter_env_amount = 0;
    vc.filter_cutoff = 1;
    vc.amp_env_amount = 1;
    voice.UpdateConfig(vc);
    voice.Init(sample_rate);

    rms.Init(sample_rate, 128);

    voice.UpdateMidiControl(CC_CTL_PORTAMENTO, 30);

    voice.UpdateMidiControl(CC_FILTER_CUTOFF, 0);
    voice.UpdateMidiControl(CC_FILTER_RESONANCE, 30);
    voice.UpdateMidiControl(CC_ENV_FILT_A, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_D, 60);
    voice.UpdateMidiControl(CC_ENV_FILT_S, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_R, 15);
    voice.UpdateMidiControl(CC_ENV_FILT_AMT, 60);

    voice.UpdateMidiControl(CC_ENV_AMP_A, 0);
    voice.UpdateMidiControl(CC_ENV_AMP_D, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_S, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_R, 25);
    voice.UpdateMidiControl(CC_OSC_1_VOLUME, 127);
    voice.UpdateMidiControl(CC_CTL_VOLUME, 100);



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
    Switch button;
    int pin_number = 15;
    button.Init(hw.GetPin(pin_number), 1000);


    AdcChannelConfig adcConfig{};
    //Configure pin 21 as an ADC input. This is where we'll read the knob.
    pin_number = 16;
    adcConfig.InitSingle(hw.GetPin(pin_number));
    //Initialize the adc with the config we just made
    hw.adc.Init(&adcConfig, 1);

    pin_number = 17;
    cv_gate.Init(hw.GetPin(pin_number), 1000);

    //Start reading values
    hw.adc.Start();

    int line_number;
    auto font = Font_11x18;//Font_7x10;
    auto peak_scaled = 0;
    auto rms_scaled = 0;
    while (true) {

        button.Debounce();
        cv_gate.Debounce();
        // Set the onboard LED
        hw.SetLed(button.Pressed() || !cv_gate.Pressed());


//        if (button.RisingEdge()) {
//            voice.NoteOn(63, 100);
//            gate = 1;
//            adsr.Retrigger(true);
//        }
//        if (button.FallingEdge()) {
//            voice.NoteOff(63, 100);
//            gate = 0;
//        }

        if (display_on) {
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

            line_number++;
            display.SetCursor(0, 44);
            sprintf(strbuff, "note: %d", voice.Playing());
            display.WriteString(strbuff, font, true);

            display.Update();
        }
        counter += direction;
        if (counter % 99 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}