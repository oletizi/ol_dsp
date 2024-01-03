#ifdef DAISY_NATIVE
//#include "daisy/daisy_dummy.h"
//#include "hid/logger.h"
#else


#endif

#include <cstdio>
#include <deque>
#include "daisy.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

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


static DaisySeed hw;
using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
MyOledDisplay display;

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


UartHandler::Config usart_a;
UartHandler a_handler;
ol_daisy::io::DaisySerial serial(a_handler);
ol::io::SimpleSerializer serializer(serial);
MyControlListener control_listener;
UartHandler::Result a_init_result = UartHandler::Result::ERR;

daisysp::Oscillator voice;

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
    for(size_t i = 0; i < size; i += 2) {
        t_sample voice_out = voice.Process();
        out[i]     = voice_out;
        out[i + 1] = voice_out;

    }
}

int main() {

    hw.Configure();
    hw.Init();

    serializer.AddControlListener(control_listener);

//    a_tx.baudrate = 115200;

    const dsy_gpio_pin &a_rx_pin = DaisySeed::GetPin(14);
    const dsy_gpio_pin &a_tx_pin = DaisySeed::GetPin(13);


//    usart_a.baudrate = 9600;
//    usart_a.baudrate = 57600;
    usart_a.baudrate = 115200;
    usart_a.periph = UartHandler::Config::Peripheral::USART_1;
    usart_a.stopbits = UartHandler::Config::StopBits::BITS_1;
    usart_a.parity = UartHandler::Config::Parity::NONE;
    usart_a.mode = UartHandler::Config::Mode::TX_RX;
    usart_a.wordlength = UartHandler::Config::WordLength::BITS_8;
    usart_a.pin_config.rx = a_rx_pin;//{DSY_GPIOB, 7};
    usart_a.pin_config.tx = a_tx_pin;//{DSY_GPIOB, 6};

    a_init_result = a_handler.Init(usart_a);

    hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
    auto sample_rate = hw.AudioSampleRate();
    voice.Init(sample_rate);
    voice.SetFreq(440);
    hw.StartAudio(audio_callback);

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg = {};
    if (DISPLAY_ON) {
        disp_cfg.driver_config.transport_config.pin_config.dc = daisy::DaisySeed::GetPin(9);
        disp_cfg.driver_config.transport_config.pin_config.reset = daisy::DaisySeed::GetPin(30);
        /** And Initialize */
        display.Init(disp_cfg);
    }
    uint64_t counter = 0;
    char strbuff[STRING_BUF_SIZE];
    uint8_t outbuf[OUT_BUF_SIZE];
    uint8_t inbuf[IN_BUF_SIZE];
    int direction = 1;
    size_t bytes_read = 0;


    while (true) {
        if (!a_handler.RxActive()) {
            a_handler.FlushRx();
            a_handler.StartRx();
        }
//        serial.Printf("Hello from daisy! Counter: %d\n", counter);
        //ol::ctl::Control ctl{CC_FILTER_CUTOFF, 0};
        serializer.Process();
        // while (serial.Available()) {
        //     serial.Printf("Daisy found: %d\n", serial.Read());
        // }
        while (!control_queue.empty()) {
            auto &c = control_queue.front();
            auto int_value = c.value;
            auto float_value = ol::core::scale(int_value, 0, 4096, 0, 20000, 1);
            voice.SetFreq(float_value);
            serial.Printf("control: controller: %d, value: %d\n", c.controller, c.value);
            control_queue.pop_front();
        }

        counter += direction;
        if (counter == 4000000 || counter == 0) {
            direction *= -1;
        }
//        for (bytes_read = 0; bytes_read< IN_BUF_SIZE && a_handler.Readable() > 0; bytes_read++) {
//            inbuf[bytes_read] = a_handler.PopRx();
//        }
//        while (a_handler.Readable() > 0) {
//            bytes_read = a_handler.PollReceive(inbuf, sizeof(inbuf), 10);
//            serial.Write(inbuf, bytes_read);
//        }
        // System::Delay(1000);

    }

}