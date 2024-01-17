
//#include "daisy.h"
#include "daisy_seed.h"

#define IN_BUF_SIZE 8
#define USE_DAISY_MIDI true
#define DISPLAY_ON false
#define DISPLAY_UPDATE_FREQUENCY 250
using namespace daisy;

static DaisySeed hw;

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

namespace ol::midi {
    class MidiParser {
    public:
        MidiParser() {};

        ~MidiParser() {}

        inline void Init() { Reset(); }

        /**
         * @brief Parse one MIDI byte. If the byte completes a parsed event,
         *        its value will be assigned to the dereferenced output pointer.
         *        Otherwise, status is preserved in anticipation of the next sequential
         *        byte. Return value indicates if a new event was parsed or not.
         *
         * @param byte      Raw MIDI byte to parse
         * @param event_out Pointer to output event object, value assigned on parse success
         * @return true     If a new event was parsed
         * @return false    If no new event was parsed
         */
        bool Parse(uint8_t byte, daisy::MidiEvent *event_out) {

            // reset parser when status byte is received
            bool did_parse = false;

            if ((byte & kStatusByteMask
                ) && pstate_ != ParserSysEx) {
                pstate_ = ParserEmpty;
            }
            switch (pstate_) {
                case ParserEmpty:
                    // check byte for valid Status Byte
                    if (byte & kStatusByteMask) {
                        // Get MessageType, and Channel
                        incoming_message_.channel = byte & kChannelMask;
                        incoming_message_.type
                                = static_cast<MidiMessageType>((byte & kMessageMask) >> 4);
                        if ((byte & 0xF8) == 0xF8)
                            incoming_message_.type = SystemRealTime;

                        // Validate, and move on.
                        if (incoming_message_.type < MessageLast) {
                            pstate_ = ParserHasStatus;

                            if (incoming_message_.type == SystemCommon) {
                                incoming_message_.channel = 0;
                                incoming_message_.sc_type
                                        = static_cast<SystemCommonType>(byte & 0x07);
                                //sysex
                                if (incoming_message_.sc_type == SystemExclusive) {
                                    pstate_ = ParserSysEx;
                                    incoming_message_.sysex_message_len = 0;
                                }
                                    //short circuit
                                else if (incoming_message_.sc_type > SongSelect) {
                                    pstate_ = ParserEmpty;
                                    if (event_out != nullptr) {
                                        *event_out = incoming_message_;
                                    }
                                    did_parse = true;
                                }
                            } else if (incoming_message_.type == SystemRealTime) {
                                incoming_message_.srt_type
                                        = static_cast<SystemRealTimeType>(
                                        byte & kSystemRealTimeMask);

                                //short circuit to start
                                pstate_ = ParserEmpty;
                                if (event_out != nullptr) {
                                    *event_out = incoming_message_;
                                }
                                did_parse = true;
                            } else // Channel Voice or Channel Mode
                            {
                                running_status_ = incoming_message_.type;
                            }
                        }
                        // Else we'll keep waiting for a valid incoming status byte
                    } else {
                        // Handle as running status
                        incoming_message_.type = running_status_;
                        incoming_message_.data[0] = byte & kDataByteMask;
                        //check for single byte running status, really this only applies to channel pressure though
                        if (running_status_ == ChannelPressure
                            || running_status_ == ProgramChange
                            || incoming_message_.sc_type == MTCQuarterFrame
                            || incoming_message_.sc_type == SongSelect) {
                            //Send the single byte update
                            pstate_ = ParserEmpty;
                            if (event_out != nullptr) {
                                *event_out = incoming_message_;
                            }
                            did_parse = true;
                        } else {
                            pstate_ = ParserHasData0; //we need to get the 2nd byte yet.
                        }
                    }
                    break;
                case ParserHasStatus:
                    if ((byte & kStatusByteMask) == 0) {
                        incoming_message_.data[0] = byte & kDataByteMask;
                        if (running_status_ == ChannelPressure
                            || running_status_ == ProgramChange
                            || incoming_message_.sc_type == MTCQuarterFrame
                            || incoming_message_.sc_type == SongSelect) {
                            //these are just one data byte, so we short circuit back to start
                            pstate_ = ParserEmpty;
                            if (event_out != nullptr) {
                                *event_out = incoming_message_;
                            }
                            did_parse = true;
                        } else {
                            pstate_ = ParserHasData0;
                        }

                        //ChannelModeMessages (reserved Control Changes)
                        if (running_status_ == ControlChange
                            && incoming_message_.data[0] > 119) {
                            incoming_message_.type = ChannelMode;
                            running_status_ = ChannelMode;
                            incoming_message_.cm_type = static_cast<ChannelModeType>(
                                    incoming_message_.data[0] - 120);
                        }
                    } else {
                        // invalid message go back to start ;p
                        pstate_ = ParserEmpty;
                    }
                    break;
                case ParserHasData0:
                    if ((byte & kStatusByteMask) == 0) {
                        incoming_message_.data[1] = byte & kDataByteMask;

                        //velocity 0 NoteOns are NoteOffs
                        if (running_status_ == NoteOn && incoming_message_.data[1] == 0) {
                            incoming_message_.type = NoteOff;
                        }

                        // At this point the message is valid, and we can complete this MidiEvent
                        if (event_out != nullptr) {
                            *event_out = incoming_message_;
                        }
                        did_parse = true;
                    } else {
                        // invalid message go back to start ;p
                        pstate_ = ParserEmpty;
                    }
                    // Regardless, of whether the data was valid or not we go back to empty
                    // because either the message is queued for handling or its not.
                    pstate_ = ParserEmpty;
                    break;
                case ParserSysEx:
                    // end of sysex
                    if (byte == 0xf7) {
                        pstate_ = ParserEmpty;
                        if (event_out != nullptr) {
                            *event_out = incoming_message_;
                        }
                        did_parse = true;
                    } else if (incoming_message_.sysex_message_len < SYSEX_BUFFER_LEN) {
                        incoming_message_
                                .sysex_data[incoming_message_.sysex_message_len]
                                = byte;
                        incoming_message_.sysex_message_len++;
                    }
                    break;
                default:
                    break;
            }

            return
                    did_parse;
        }

        /**
         * @brief Reset parser to default state
         */
        void Reset() {
            pstate_ = ParserEmpty;
            incoming_message_.type = MessageLast;
        }

    private:
        enum ParserState {
            ParserEmpty,
            ParserHasStatus,
            ParserHasData0,
            ParserSysEx,
        };

        ParserState pstate_;
        MidiEvent incoming_message_;
        MidiMessageType running_status_;

        // Masks to check for message type, and byte content
        const uint8_t kStatusByteMask = 0x80;
        const uint8_t kMessageMask = 0x70;
        const uint8_t kDataByteMask = 0x7F;
        const uint8_t kChannelMask = 0x0F;
        const uint8_t kSystemRealTimeMask = 0x07;
    };
}

void handleMidi(MidiEvent event);

int main() {

    hw.Configure();
    hw.Init();
    MidiUartHandler midi;
    MidiUartHandler::Config midi_config;
    const dsy_gpio_pin &midi_rx_pin = DaisySeed::GetPin(16);
    const dsy_gpio_pin &midi_tx_pin = DaisySeed::GetPin(28);
    const auto periph = UartHandler::Config::Peripheral::USART_2;
    midi_config.transport_config.rx = midi_rx_pin;
    midi_config.transport_config.tx = midi_tx_pin;
    midi_config.transport_config.periph = periph;
    if (USE_DAISY_MIDI) {
        midi.Init(midi_config);
        midi.StartReceive();
    }
    daisy::MidiEvent midi_event{};
    ol::midi::MidiParser midi_parser;
    midi_parser.Init();

    UartHandler::Config usart_midi_config;
    UartHandler uart_midi_handler;
    UartHandler::Result uart_midi_handler_init_result = UartHandler::Result::ERR;
    const dsy_gpio_pin &a_rx_pin = DaisySeed::GetPin(16);
    const dsy_gpio_pin &a_tx_pin = DaisySeed::GetPin(28);
    usart_midi_config.baudrate = 31250;//9600;
    usart_midi_config.periph = UartHandler::Config::Peripheral::USART_2;
    usart_midi_config.stopbits = UartHandler::Config::StopBits::BITS_1;
    usart_midi_config.parity = UartHandler::Config::Parity::NONE;
    usart_midi_config.mode = UartHandler::Config::Mode::RX;
    usart_midi_config.wordlength = UartHandler::Config::WordLength::BITS_8;
    usart_midi_config.pin_config.rx = a_rx_pin;
    usart_midi_config.pin_config.tx = a_tx_pin;
    if (!USE_DAISY_MIDI) {
        uart_midi_handler_init_result = uart_midi_handler.Init(usart_midi_config);
    }
    uint8_t counter = 0;
    uint8_t inbuf[IN_BUF_SIZE]{};
    int direction = 1;
    UartHandler::Result read_status = UartHandler::Result::ERR;
    uint64_t read_byte_sum = 0;
    hw.StartLog(true);
    hw.PrintLine("Starting loop...");
    while (true) {
        if (USE_DAISY_MIDI) {
            midi.Listen();
            while (midi.HasEvents()) {
                handleMidi(midi.PopEvent());
            }
        } else if (uart_midi_handler_init_result == UartHandler::Result::OK) {
            read_status = uart_midi_handler.BlockingReceive(inbuf, 1, 10);
            if (read_status == UartHandler::Result::OK && midi_parser.Parse(inbuf[0], &midi_event)) {
                handleMidi(midi_event);
            }
        }
        counter += direction;
        if (counter % 100 == 0) {
            direction *= -1;
        }
    }

}

void handleMidi(MidiEvent event) {
    switch (event.type) {
        case MidiMessageType::NoteOn: {
            hw.PrintLine("%d: Note ON!", System::GetNow());
            hw.SetLed(true);
            break;
        }
        case MidiMessageType::NoteOff: {
            hw.PrintLine("%d: Note OFF!", System::GetNow());
            hw.SetLed(false);
            break;
        }
        default:
            break;
    }
}
