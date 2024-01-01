//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_SERIALIZER_H
#define OL_DSP_SERIALIZER_H

#include <vector>
#include "ol_ctllib.h"
#include "ol_iolib_core.h"
#include "Serial.h"

namespace ol::io {
    class ControlListener {
    public:
        virtual void HandleControl(ol::ctl::Control control) = 0;
    };

    class Serializer {
    public:
        virtual void WriteControl(ol::ctl::Control) = 0;
    };

    class Deserializer {
    public:
        virtual void Read(uint8_t *buffer) = 0;

        virtual void AddControlListener(ControlListener &) = 0;
    };

    class SimpleSerializer : public Serializer, public Deserializer {
    public:
        static inline uint8_t const START_BYTE = 42;
        static inline size_t const START_BYTE_COUNT = 5;
    private:
        static const size_t MESSAGE_SIZE_OFFSET = 0;
        static const size_t CONTROLLER_OFFSET = MESSAGE_SIZE_OFFSET + sizeof(int64_t);
        static const size_t VALUE_OFFSET = CONTROLLER_OFFSET + sizeof(int64_t);

        Serial &serial_;
        bool parsing_ = false;
        uint64_t parsed_bytes_ = 0;
        int64_t message_size_ = 0;
        size_t start_bytes_ = 0;
        std::vector<uint8_t> size_buffer_;
        std::vector<uint8_t> controller_buffer_;
        std::vector<uint8_t> value_buffer_;
        std::vector<ControlListener *> control_listeners_;

    public:
        explicit SimpleSerializer(Serial &serial) : serial_(serial) {
            size_buffer_.reserve(sizeof(int64_t));
            controller_buffer_.reserve(sizeof(int64_t));
            value_buffer_.reserve(sizeof(int64_t));
        }
        void Reset() {
            parsing_ = false;
            parsed_bytes_ = 0;
            message_size_ = 0;
            start_bytes_ = 0;
            size_buffer_.clear();
            controller_buffer_.clear();
            value_buffer_.clear();
        }

        void Process() {
            while (serial_.Available()) {
                auto byte_read = uint8_t(serial_.Read());

                if (!parsing_ && byte_read == START_BYTE) { start_bytes_++; continue;}
                // we're parsing if we've seen the start bytes and we've still got bytes to parse OR we don't know the
                // message size yet.
                parsing_ = start_bytes_ == START_BYTE_COUNT && (parsed_bytes_ < message_size_ || message_size_ == 0);
                if (parsing_ && parsed_bytes_ < CONTROLLER_OFFSET) {
                    // we're parsing the message size
                    size_buffer_.push_back(byte_read);
                    parsed_bytes_++;
                    if (parsed_bytes_ == CONTROLLER_OFFSET) {
                        // we're done parsing the message size.
                        message_size_ = bytes_to_int64(size_buffer_);
                    }
                } else if (parsing_ && parsed_bytes_ < message_size_) {
                    if (parsed_bytes_ < VALUE_OFFSET) {
                        controller_buffer_.push_back(byte_read);
                    } else {
                        value_buffer_.push_back(byte_read);
                    }
                    parsed_bytes_++;
                }
                if (parsed_bytes_ == message_size_) {
                    // we're done. notify listeners and reset
                    ctl::Control c{
                            bytes_to_int64(controller_buffer_),
                            bytes_to_int64(value_buffer_)
                    };
                    for (auto &l: control_listeners_) {
                        l->HandleControl(c);
                    }
                    Reset();
                }
            }
        }

        void WriteControl(ol::ctl::Control control) override {
            // XXX: I'm sure there's a more efficient/elegant way to do this.
            auto controller_data = int64_to_bytes(control.controller);
            auto value_data = int64_to_bytes(control.value);

            std::vector<uint8_t> serialized;
            serialized.reserve(sizeof(uint8_t) * START_BYTE_COUNT + controller_data.size() + value_data.size());

            // write the start bytes...
            for (int i = 0; i < START_BYTE_COUNT; i++) {
                serialized.push_back(START_BYTE);
            }

            // write the message size (including message size field)...
            auto message_size = sizeof(int64_t) + controller_data.size() + value_data.size();
            auto message_size_data = int64_to_bytes(int64_t(message_size));
            serialized.insert(serialized.end(), message_size_data.begin(), message_size_data.end());
            serialized.insert(serialized.end(), controller_data.begin(), controller_data.end());
            serialized.insert(serialized.end(), value_data.begin(), value_data.end());
            serial_.Write(serialized, serialized.size());
        }

        void AddControlListener(ControlListener &listener) override {
            // XXX: this is probably bad.
            control_listeners_.push_back(&listener);
        }

        void Read(uint8_t *buffer) override {

        }


    };
}


#endif //OL_DSP_SERIALIZER_H
