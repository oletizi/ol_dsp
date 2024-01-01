//
// Created by Orion Letizi on 12/31/23.
//
#include "gtest/gtest.h"
#include "ol_iolib.h"

using namespace ol::io;
using namespace ol::ctl;

class LoopbackSerial : public Serial {
private:
    uint8_t buf_[256]{};
    int read_index_ = -1;
    int write_index_ = -1;

public:
    size_t Write(const char *data, size_t size) override {
        return 0;
    }

    size_t Write(const uint8_t *data, size_t size) override {
        int i;
        if (write_index_ == sizeof(buf_)) {
            write_index_ = 0;
        }
        for (i = 0; i < size && write_index_ < sizeof(buf_); i++, write_index_++) {
            buf_[write_index_] = data[i];
        }
        return i;
    }

    size_t Available() override {
        return write_index_ + 1;
    }

    int Read() override {
        int rv = -1;
        if (write_index_ >= 0 && read_index_ < write_index_) {
            rv = buf_[++read_index_];
        }
        if (read_index_ + 1 == sizeof(buf_)) {
            read_index_ = -1;
        }
        return rv;
    }
};

TEST(Loopback, Basics) {
    LoopbackSerial lb;
    EXPECT_EQ(lb.Read(), -1);
}

TEST(Serializer, Basics) {
    LoopbackSerial loopback;
    Control c1{1, 2};
    Serializer *s = new SimpleSerialzer(&loopback);

}