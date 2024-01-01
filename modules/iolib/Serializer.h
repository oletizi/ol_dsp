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


    using BinaryStreamByteVector = std::vector<std::uint8_t>;


    class SimpleSerializer : public Serializer, public Deserializer {
    private:
        Serial &serial_;
        std::vector<uint8_t> read_buf_;
        bool parsing_ = false;
        uint64_t parsed_bytes_ = 0;
        uint64_t message_size = 0;

    public:
        static const uint8_t START_BYTE = 42;
        static const size_t START_BYTE_COUNT = 5;

        explicit SimpleSerializer(Serial &serial) : serial_(serial) {}

        void Process() {
            size_t start_bytes = 0;
            while (serial_.Available()) {
                auto byte_read = uint8_t(serial_.Read());
                read_buf_.push_back(byte_read);

                if (!parsing_ && byte_read == START_BYTE) { start_bytes++; }
                parsing_ = start_bytes == START_BYTE_COUNT;
                if (parsing_ && parsed_bytes_ == 0) {

                }

            }
        }

        void WriteControl(ol::ctl::Control control) override {
            // XXX: I'm sure there's a more efficient/elegant way to do this.
            auto controller_data = int64_to_byte_array(control.controller);
            auto value_data = int64_to_byte_array(control.value);
            std::vector<uint8_t> serialized;
            serialized.reserve(controller_data.size() + value_data.size());
            serialized.insert(serialized.end(), controller_data.begin(), controller_data.end());
            serialized.insert(serialized.end(), value_data.begin(), value_data.end());
            serial_.Write(serialized, serialized.size());
        }

        void AddControlListener(ControlListener &listener) override {

        }

        void Read(uint8_t *buffer) override {

        }


    };
}


#endif //OL_DSP_SERIALIZER_H
