#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"
#else


#endif

#include <deque>
#include "daisy.h"
#include "daisy_seed.h"

#include "corelib/ol_corelib.h"
#include "fxlib/ol_fxlib.h"
#include "ol_daisy/io/ol_daisy_io.h"
#include "app/synth/ol_app_synth.h"

#define IN_BUF_SIZE 8
#define DISPLAY_ON false
#define DISPLAY_UPDATE_FREQUENCY 250
#define CHANNEL_COUNT 2
#define VOICE_COUNT 1
#define AUDIO_BLOCK_SIZE 128
#define MAX_DELAY_SAMPLES 48000

static daisy::DaisySeed hw;

std::deque<ol::ctl::Control> control_queue;

class MyControlListener : public ol::io::ControlListener {
public:
    void HandleControl(ol::ctl::Control control) override {
        control_queue.push_back(control);
    }
};

void handleControlQueue();

void handleMidiMessage(daisy::MidiEvent event);

void handleMidi(daisy::MidiEvent event);

daisy::UartHandler::Config usart_a;
daisy::UartHandler a_handler;
ol_daisy::io::DaisySerial serial(a_handler);
ol::io::SimpleSerializer serializer(serial);
MyControlListener control_listener;

//SynthVoice<1> voice;
ol::synth::SynthVoice v1;
ol::synth::SynthVoice v2;
ol::synth::SynthVoice v3;
ol::synth::SynthVoice v4;
std::vector<ol::synth::Voice *> voices{&v1, &v2, &v3, &v4};
ol::synth::Polyvoice<1> voice(voices);

// init delay
daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> DSY_SDRAM_BSS delay_line1;
std::vector<daisysp::DelayLine<t_sample, MAX_DELAY_SAMPLES> *> delay_lines{&delay_line1};
ol::fx::DelayFx<1> delay_fx(delay_lines);

// init reverb
daisysp::ReverbSc verb;
ol::fx::DaisyVerb<CHANNEL_COUNT> daisy_verb(verb);
ol::fx::ReverbFx<CHANNEL_COUNT> reverb_fx(daisy_verb);

// init filter
ol::fx::FilterFx<CHANNEL_COUNT> filter_fx;

// CPU load meter
daisy::CpuLoadMeter load_meter;

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    load_meter.OnBlockStart();
    for (size_t i = 0; i < size; i += 2) {

        t_sample buf[CHANNEL_COUNT]{};

        voice.Process(buf);

//        delay_fx.Process(buf, buf);
        buf[1] = buf[0];
//        reverb_fx.Process(buf, buf);
        // filter_fx.Process(buf, buf);

        out[i] = buf[0];
        out[i + 1] = buf[0 + 1];
        // out[i+1] = frame_buffer[0+1];
    }
    load_meter.OnBlockEnd();
}


int main() {

    hw.Configure();
    hw.Init();
    hw.StartLog(false);
    // MIDI IO

    daisy::MidiUartHandler::Config midi_uart_config;
    daisy::MidiUartHandler midi;
    //    daisy::UartHandler midi_uart_handler;
    daisy::UartHandler::Result midi_uart_init_result = daisy::UartHandler::Result::ERR;

    const dsy_gpio_pin &midi_rx_pin = daisy::DaisySeed::GetPin(16);
    const dsy_gpio_pin &midi_tx_pin = daisy::DaisySeed::GetPin(28);

    midi_uart_config.transport_config.periph = daisy::UartHandler::Config::Peripheral::USART_2;
    midi_uart_config.transport_config.rx = midi_rx_pin;
    midi_uart_config.transport_config.tx = midi_tx_pin;
    midi.Init(midi_uart_config);

    // Voice init
    voice.UpdateMidiControl(CC_CTL_PORTAMENTO, 48);
    voice.UpdateMidiControl(CC_FILTER_CUTOFF, 0);
    voice.UpdateMidiControl(CC_FILTER_RESONANCE, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_A, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_D, 100);
    voice.UpdateMidiControl(CC_ENV_FILT_S, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_R, 24);
    voice.UpdateMidiControl(CC_ENV_FILT_AMT, 127);

    voice.UpdateMidiControl(CC_ENV_AMP_A, 0);
    voice.UpdateMidiControl(CC_ENV_AMP_D, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_S, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_R, 100);
    voice.UpdateMidiControl(CC_OSC_1_VOLUME, 100);
    voice.UpdateMidiControl(CC_CTL_VOLUME, 80);

    serializer.AddControlListener(control_listener);

    /*
    // Daisy <-> Teensy IO
    const dsy_gpio_pin &a_rx_pin = daisy::DaisySeed::GetPin(14);
    const dsy_gpio_pin &a_tx_pin = daisy::DaisySeed::GetPin(13);

    usart_a.baudrate = 115200;
    usart_a.periph = daisy::UartHandler::Config::Peripheral::USART_1;
    usart_a.pin_config.rx = a_rx_pin;//{DSY_GPIOB, 7};
    usart_a.pin_config.tx = a_tx_pin;//{DSY_GPIOB, 6};

    usart_a.stopbits = daisy::UartHandler::Config::StopBits::BITS_1;
    usart_a.parity = daisy::UartHandler::Config::Parity::NONE;
    usart_a.mode = daisy::UartHandler::Config::Mode::TX_RX;
    usart_a.wordlength = daisy::UartHandler::Config::WordLength::BITS_8;

    auto a_init_result = a_handler.Init(usart_a);
*/
    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    auto sample_rate = hw.AudioSampleRate();
    voice.Init(sample_rate);
    delay_fx.Init(sample_rate);
    reverb_fx.Init(sample_rate);
    filter_fx.Init(sample_rate);
    load_meter.Init(sample_rate, AUDIO_BLOCK_SIZE);
    hw.StartAudio(audio_callback);

    uint64_t counter = 0;
    int direction = 1;
    auto checkpoint = daisy::System::GetNow();
    while (true) {
        uint8_t midi_byte = 0;
        int read_status = 0;
        //read_status = midi_uart_handler.PollReceive(&midi_byte, 1, 10);
        midi.Listen();
        while (midi.HasEvents()) {
            handleMidi(midi.PopEvent());
        }

        auto now = daisy::System::GetNow();
        if (now - checkpoint >= 1000) {
            checkpoint = now;
            auto min = load_meter.GetMinCpuLoad();
            auto max = load_meter.GetMaxCpuLoad();
            auto avg = load_meter.GetAvgCpuLoad();
            hw.PrintLine("CPU: min: %d%%; max: %d%%; avg: %d%%", int(min * 100), int(max * 100), int(avg * 100));
            load_meter.Reset();
        }
        counter += direction;
        if (counter == 1000 || counter == 0) {
            direction *= -1;
        }
    }

}

void handleMidi(daisy::MidiEvent event) {
    switch (event.type) {
        case daisy::MidiMessageType::NoteOn: {
            hw.SetLed(true);
            auto note_on = event.AsNoteOn();
            voice.NoteOn(note_on.note, note_on.velocity);
            break;
        }
        case daisy::MidiMessageType::NoteOff: {
            hw.SetLed(false);
            auto note_off = event.AsNoteOff();
            voice.NoteOff(note_off.note, note_off.velocity);
            break;
        }
        case daisy::MidiMessageType::ControlChange: {
            hw.SetLed(true);
            auto control = event.AsControlChange();
            voice.UpdateMidiControl(control.control_number, control.value);
            break;
        }
        default:
            break;
    }
}

void handleMidiMessage(daisy::MidiEvent event) {
    switch (event.type) {
        case daisy::MidiMessageType::NoteOn: {
            hw.SetLed(true);
            const auto &onEvent = event.AsNoteOn();
            voice.NoteOn(onEvent.note, onEvent.velocity);
            break;
        }
        case daisy::MidiMessageType::NoteOff: {
            hw.SetLed(false);
            const auto &offEvent = event.AsNoteOff();
            voice.NoteOff(offEvent.note, offEvent.velocity);
            break;
        }
        default:
            break;
    }
}

void handleControlQueue() {
    serializer.Process();
    while (!control_queue.empty()) {
        auto &c = control_queue.front();
        switch (c.controller) {
            case CC_VOICE_GATE_ON:
                voice.NoteOn(uint8_t(c.value), 100);
                break;
            case CC_VOICE_GATE_OFF:
                voice.NoteOff(uint8_t(c.value), 100);
                break;
            case CC_VOICE_PITCH:
                voice.SetFrequency(daisysp::mtof(c.value));
                break;
            default:
                auto int_value = t_sample(c.value);
                auto scaled_value = ol::core::scale(int_value, 0, 4096, 0, 1, 1);
                voice.UpdateHardwareControl(c.controller, scaled_value);
                break;
        }
        control_queue.pop_front();
    }
}
