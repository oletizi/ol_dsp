//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_DAISYSERIAL_H
#define OL_DSP_DAISYSERIAL_H

#include "iolib/ol_iolib.h"

namespace ol_daisy::io {
    class DaisySerial : public ol::io::Serial {

    public:
        explicit DaisySerial(daisy::UartHandler &uart) : uart_(uart) {}

        void Print(const std::string &msg) {
            uint8_t data[msg.length() + 1];
            strcpy((char *) data, msg.data());
            Write(data, msg.length());
        }

        void Printf(const char *format, ...) {
            va_list va;
            va_start(va, format);
            PrintfV(format, va);
            va_end(va);
        }

        void PrintfV(const char *format, const va_list va) {
            vsnprintf(string_buffer, sizeof(string_buffer), format, va);
            Print(string_buffer);
        }

        void Println(const std::string &msg) {
            Print(msg + "\n");
        }

        int Write(const char *data, const int size) override {
            return Write(reinterpret_cast<const uint8_t *>(data), size);
        }

        int Write(const uint8_t *data, const int size) override {
            int bytes_written = 0;
            int bytes_to_write = 0;
            while (bytes_written < size) {
                bytes_to_write = 0;
                for (uint16_t i = 0; i < sizeof(outbuf_) && bytes_written <= size; i++, bytes_written++, bytes_to_write++) {
                    outbuf_[i] = data[bytes_written];
                }
                uart_.PollTx(outbuf_, bytes_to_write);
            }
            return bytes_written;
        }

        int Write(std::vector<uint8_t> data, int size) override {
            uint8_t byte_buffer[size];
            for (int i=0; i<size; i++) {
                byte_buffer[i] = data[i];
            }
            return Write(byte_buffer, size);
        }

    private:
        daisy::UartHandler &uart_;
        uint8_t outbuf_[8]{};
        char string_buffer[256]{};
    };
}
#endif //OL_DSP_DAISYSERIAL_H
