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
#include "synthlib/ol_synthlib.h"

#define AUDIO_BLOCK_SIZE 4
#define CHANNEL_COUNT 2
using namespace daisy;
using Log = Logger<LOGGER_SEMIHOST>;
using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
static DaisySeed hw;

MyOledDisplay display;

daisysp::Oscillator osc;
t_sample gate = 0;
daisysp::Adsr adsr;
auto voice = ol::synth::SynthVoice<CHANNEL_COUNT>();
auto rms = ol::core::Rms();
t_sample rms_value = 0;
t_sample peak_value = 0;
t_sample peak_hold = 2 * 48000;
t_sample peak_count = 0;

void audio_callback(daisy::AudioHandle::InputBuffer in,
                    daisy::AudioHandle::OutputBuffer out,
                    size_t size) {
    for (int i = 0; i < size; i++) {
        //voice.Process(out[i]);

        *out[i] = osc.Process() * adsr.Process(gate);
        rms_value = rms.Process(*out[i]);
        peak_value = peak_value < abs(*out[i]) ? abs(*out[i]) : peak_value;
        peak_count++;
        if (peak_count == peak_hold) {
            peak_count = 0;
            peak_value = 0;
        }
    }
}

int main() {

    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    hw.StartAudio(audio_callback);
    auto sample_rate = hw.AudioSampleRate();

    osc.Init(sample_rate);
    osc.SetFreq(10000.0f);
    osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);

    ol::synth::Voice::Config vc = {};
    vc.filter_env_amount = 0;
    vc.filter_cutoff = 1;
    vc.amp_env_amount = 1;
    voice.UpdateConfig(vc);
    voice.Init(sample_rate);

    adsr.Init(sample_rate);
    adsr.SetAttackTime(0.5);
    adsr.SetDecayTime(0.5);
    adsr.SetSustainLevel(0);
    adsr.SetReleaseTime(0.5);

    rms.Init(sample_rate, 128);

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg = {};
    disp_cfg.driver_config.transport_config.pin_config.dc = daisy::DaisySeed::GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = daisy::DaisySeed::GetPin(30);
    /** And Initialize */
    display.Init(disp_cfg);
    // printf("showtime\n");
    Log::StartLog(true);
    Log::PrintLine("I should be printing to LOGGER_SEMIHOST");
    DaisySeed::StartLog(false);
    int counter = 0;
    char strbuff[128];
    int direction = 1;
    Switch button;
    int pin_number = 15;
    button.Init(hw.GetPin(pin_number), 1000);

    //ol::synth::SynthVoice<2>(ol::synth::OscillatorSoundSource<2>, ol::synth::Filter, ol::synth::Adsr, ol::synth::Adsr, ol::synth::Portamento);
    //ol::synth::SynthVoice<2> voice;

    int line_number;
    auto font = Font_11x18;//Font_7x10;
    auto peak_scaled = 0;
    auto rms_scaled = 0;
    while (true) {
        button.Debounce();
        // Set the onboard LED
        hw.SetLed(button.Pressed());
        // Toggle the LED state for the next time around.
        //led_state = !led_state;
        if (button.RisingEdge()) {
            voice.NoteOn(63, 100);
            gate = 1;
            adsr.Retrigger(true);
        }
        if (button.FallingEdge()) {
            voice.NoteOff(63, 100);
            gate = 0;
        }
        line_number = 0;
        display.Fill(false);

        display.SetCursor(0, 0);
        sprintf(strbuff, "counter: %d", counter);
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

        counter += direction;
        if (counter % 99 == 0) {
            direction *= -1;
        }
        System::Delay(1);

    }

}