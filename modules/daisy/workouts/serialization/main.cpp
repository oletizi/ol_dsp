#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"
#else


#endif

#include <cstdio>
#include <deque>
#include "daisy.h"
#include "daisy_seed.h"

#include "corelib/ol_corelib.h"
#include "daisy/io/io.h"


#define STRING_BUF_SIZE 256
#define OUT_BUF_SIZE 8
#define IN_BUF_SIZE 8
#define DISPLAY_ON false
#define DISPLAY_UPDATE_FREQUENCY 250
#define CHANNEL_COUNT 2
#define VOICE_COUNT 1
#define AUDIO_BLOCK_SIZE 4

using namespace daisy;
using namespace ol::synth;


static DaisySeed hw;

std::deque<ol::ctl::Control> control_queue;

class MyControlListener : public ol::io::ControlListener {
public:
    void HandleControl(ol::ctl::Control control) override {
        control_queue.push_back(control);
    }
};

//Example from: https://forum.electro-smith.com/t/daisy-seed-pinout-usart-1-rx-pin-37-or-pin-15/1175/10
////Init TX PB4 UART7_TX / AF11 for the Nextion screen - SPI1 MISO Pin n°10
//UartHandler::Config       Nextion_Tx;
//Nextion_Tx.baudrate      = 115200;
//Nextion_Tx.periph        = UartHandler::Config::Peripheral::UART_7;
//Nextion_Tx.stopbits      = UartHandler::Config::StopBits::BITS_1;
//Nextion_Tx.parity        = UartHandler::Config::Parity::NONE;
//Nextion_Tx.mode          = UartHandler::Config::Mode::TX;
//Nextion_Tx.wordlength    = UartHandler::Config::WordLength::BITS_8;
//Nextion_Tx.pin_config.tx = {DSY_GPIOB, 4};
//Nextion_Tx.pin_config.rx = {DSY_GPIOX,0};       // Always define the two pins TX and RX
//
//// /** UART communication initialization */
//Nextion_TX_handler.Init(Nextion_Tx);

daisy::MidiUartHandler midi;
UartHandler::Config usart_a;
UartHandler a_handler;
ol_daisy::io::DaisySerial serial(a_handler);
ol::io::SimpleSerializer serializer(serial);
MyControlListener control_listener;

SynthVoice<1> voice;

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    for (size_t i = 0; i < size; i += 2) {
        t_sample voice_out = 0;
        voice.Process(&voice_out);
        out[i] = voice_out;
        out[i + 1] = voice_out;

    }
}


int main() {

    hw.Configure();
    hw.Init();


    voice.UpdateMidiControl(CC_CTL_PORTAMENTO, 0);
    voice.UpdateMidiControl(CC_FILTER_CUTOFF, 0);
    voice.UpdateMidiControl(CC_FILTER_RESONANCE, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_A, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_D, 100);
    voice.UpdateMidiControl(CC_ENV_FILT_S, 0);
    voice.UpdateMidiControl(CC_ENV_FILT_R, 15);
    voice.UpdateMidiControl(CC_ENV_FILT_AMT, 127);

    voice.UpdateMidiControl(CC_ENV_AMP_A, 0);
    voice.UpdateMidiControl(CC_ENV_AMP_D, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_S, 127);
    voice.UpdateMidiControl(CC_ENV_AMP_R, 25);
    voice.UpdateMidiControl(CC_OSC_1_VOLUME, 127);
    voice.UpdateMidiControl(CC_CTL_VOLUME, 100);


    serializer.AddControlListener(control_listener);

//    a_tx.baudrate = 115200;

    const dsy_gpio_pin &a_rx_pin = DaisySeed::GetPin(14);
    const dsy_gpio_pin &a_tx_pin = DaisySeed::GetPin(13);


//    usart_a.baudrate = 9600;
    usart_a.baudrate = 57600;
//    usart_a.baudrate = 115200;
    usart_a.periph = UartHandler::Config::Peripheral::USART_1;
    usart_a.pin_config.rx = a_rx_pin;//{DSY_GPIOB, 7};
    usart_a.pin_config.tx = a_tx_pin;//{DSY_GPIOB, 6};

    usart_a.stopbits = UartHandler::Config::StopBits::BITS_1;
    usart_a.parity = UartHandler::Config::Parity::NONE;
    usart_a.mode = UartHandler::Config::Mode::TX_RX;
    usart_a.wordlength = UartHandler::Config::WordLength::BITS_8;

    auto a_init_result = a_handler.Init(usart_a);

    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    auto sample_rate = hw.AudioSampleRate();
    voice.Init(sample_rate);
    voice.SetFrequency(440);
    hw.StartAudio(audio_callback);





    uint64_t counter = 0;
    int direction = 1;
    while (true) {
        if (!a_handler.RxActive()) {
            a_handler.FlushRx();
            a_handler.StartRx();
        }

        serializer.Process();
        while (!control_queue.empty()) {
            auto &c = control_queue.front();
            switch (c.controller) {
                case CC_VOICE_GATE:
                    c.value ? voice.GateOn() : voice.GateOff();
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

        counter += direction;
        if (counter == 4000000 || counter == 0) {
            direction *= -1;
        }
    }

}