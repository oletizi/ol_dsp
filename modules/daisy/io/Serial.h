//
// Created by Orion Letizi on 12/31/23.
//

#ifndef OL_DSP_SERIAL_H
#define OL_DSP_SERIAL_H

namespace ol_daisy::io {
    class Serial {
    public:
        explicit Serial(daisy::UartHandler &uart) : uart_(uart) {}

        void Write(const char* data, const size_t size) {
            Write(reinterpret_cast<const uint8_t *>(data), size);
        }

        void Write(const uint8_t *data, const size_t size) {
            size_t bytes_written = 0;
            size_t bytes_to_write = 0;
            while (bytes_written < size) {
                bytes_to_write = 0;
                for (int i = 0; i < sizeof(outbuf_) && bytes_written <= size; i++, bytes_written++, bytes_to_write++) {
                    outbuf_[i] = data[bytes_written];
                }
                uart_.PollTx(outbuf_, bytes_to_write);
            }
        }

    private:
        daisy::UartHandler &uart_;
        uint8_t outbuf_[8]{};
    };
}
#endif //OL_DSP_SERIAL_H
