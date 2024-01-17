//
// Created by Orion Letizi on 1/16/24.
//

#ifndef OL_DSP_MIDI_H
#define OL_DSP_MIDI_H
#include "daisy.h"
namespace ol_daisy::io {
    class MidiParser {
    public:
        MidiParser() {};

        ~MidiParser() {}

        inline void Init() { Reset(); }

        /**
         *
         * THIS IS COPIED DIRECTLY FROM libDaisy b/c I couldn't get it to link there, so this is a stopgap.
         *
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
                                = static_cast<daisy::MidiMessageType>((byte & kMessageMask) >> 4);
                        if ((byte & 0xF8) == 0xF8)
                            incoming_message_.type = daisy::SystemRealTime;

                        // Validate, and move on.
                        if (incoming_message_.type < daisy::MessageLast) {
                            pstate_ = ParserHasStatus;

                            if (incoming_message_.type == daisy::SystemCommon) {
                                incoming_message_.channel = 0;
                                incoming_message_.sc_type
                                        = static_cast<daisy::SystemCommonType>(byte & 0x07);
                                //sysex
                                if (incoming_message_.sc_type == daisy::SystemExclusive) {
                                    pstate_ = ParserSysEx;
                                    incoming_message_.sysex_message_len = 0;
                                }
                                    //short circuit
                                else if (incoming_message_.sc_type > daisy::SongSelect) {
                                    pstate_ = ParserEmpty;
                                    if (event_out != nullptr) {
                                        *event_out = incoming_message_;
                                    }
                                    did_parse = true;
                                }
                            } else if (incoming_message_.type == daisy::SystemRealTime) {
                                incoming_message_.srt_type
                                        = static_cast<daisy::SystemRealTimeType>(
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
                        if (running_status_ == daisy::ChannelPressure
                            || running_status_ == daisy::ProgramChange
                            || incoming_message_.sc_type == daisy::MTCQuarterFrame
                            || incoming_message_.sc_type == daisy::SongSelect) {
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
                        if (running_status_ == daisy::ChannelPressure
                            || running_status_ == daisy::ProgramChange
                            || incoming_message_.sc_type == daisy::MTCQuarterFrame
                            || incoming_message_.sc_type == daisy::SongSelect) {
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
                        if (running_status_ == daisy::ControlChange
                            && incoming_message_.data[0] > 119) {
                            incoming_message_.type = daisy::ChannelMode;
                            running_status_ = daisy::ChannelMode;
                            incoming_message_.cm_type = static_cast<daisy::ChannelModeType>(
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
                        if (running_status_ == daisy::NoteOn && incoming_message_.data[1] == 0) {
                            incoming_message_.type = daisy::NoteOff;
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
            incoming_message_.type = daisy::MessageLast;
        }

    private:
        enum ParserState {
            ParserEmpty,
            ParserHasStatus,
            ParserHasData0,
            ParserSysEx,
        };

        ParserState pstate_;
        daisy::MidiEvent incoming_message_;
        daisy::MidiMessageType running_status_;

        // Masks to check for message type, and byte content
        const uint8_t kStatusByteMask = 0x80;
        const uint8_t kMessageMask = 0x70;
        const uint8_t kDataByteMask = 0x7F;
        const uint8_t kChannelMask = 0x0F;
        const uint8_t kSystemRealTimeMask = 0x07;
    };
}

#endif //OL_DSP_MIDI_H
