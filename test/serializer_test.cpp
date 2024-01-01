//
// Created by Orion Letizi on 12/31/23.
//
#include "gtest/gtest.h"
#include "ol_iolib.h"

using namespace ol::io;
using namespace ol::ctl;

class LoopbackSerial : public Serial {
private:
    uint8_t *write_buf_;
    int read_index_ = 0;
    int write_index_ = 0;

public:
    explicit LoopbackSerial(uint8_t *buf) : write_buf_(buf) {}

    int Write(const char *data, const size_t size) override {
        return 0;
    }

    int Write(const uint8_t *data, const size_t size) override {
        int bytes_written = 0;
        for (int i = 0; i < size; i++, write_index_++) {
            if (write_index_ == sizeof(write_buf_)) {
                write_index_ = 0;
            }
            write_buf_[write_index_] = data[i];
            bytes_written++;

        }
        return bytes_written;
    }


    int Available() override {
        return write_index_ - read_index_;
    }

    int Read() override {
        int rv = -1;
        if (Available() > 0 && read_index_ < write_index_) {
            rv = write_buf_[read_index_++];
        }
        if (read_index_ == sizeof(write_buf_)) {
            read_index_ = 0;
        }
        return rv;
    }

    int Write(const std::vector<uint8_t> data, const size_t size) override {

        uint8_t byte_array[size];
        for (size_t i = 0; i < size; i++) {
            byte_array[i] = data[i];
        }
        return Write(byte_array, size);
    }
};

TEST(Loopback, Basics) {
    int bytes_written = 0;
    const int WRITE_BUF_SIZE = 8;
    uint8_t write_buf[WRITE_BUF_SIZE]{};
    uint8_t byte_read;
    LoopbackSerial lb(write_buf);
    EXPECT_EQ(lb.Available(), 0);
    EXPECT_EQ(lb.Read(), -1);

    uint8_t data[]{42, 43, 44};
    bytes_written = lb.Write(data, sizeof(data));
    EXPECT_EQ(bytes_written, sizeof(data));
    EXPECT_EQ(lb.Available(), sizeof(data));

    for (int i = 0; i < sizeof(data); i++) {
        byte_read = lb.Read();
        EXPECT_EQ(byte_read, data[i]);
        EXPECT_EQ(lb.Available(), sizeof(data) - (i + 1));
    }

    EXPECT_EQ(lb.Available(), 0);
    const uint8_t BIG_DATA_SIZE = 100;
    uint8_t big_data[BIG_DATA_SIZE]{};
    for (int i = 0; i < sizeof(big_data); i++) {
        big_data[i] = i;
    }

    bytes_written = lb.Write(big_data, sizeof(big_data));
    EXPECT_EQ(bytes_written, sizeof(big_data));
    EXPECT_EQ(lb.Available(), sizeof(big_data) % sizeof(write_buf));
    EXPECT_GT(lb.Available(), 0);
    int offset = (BIG_DATA_SIZE / WRITE_BUF_SIZE) * WRITE_BUF_SIZE;
    for (int i = 0; i < sizeof(big_data) % sizeof(write_buf); i++) {
        byte_read = lb.Read();
        EXPECT_EQ(byte_read, big_data[offset + i]);
    }

}


TEST(Serializer, Conversion) {
    int64_t a = 55;
    int64_t b;
    std::vector<uint8_t> serialized;
//    serialized.reserve(sizeof(int64_t));
    serialized = int64_to_byte_array(a);
    b = byte_array_to_int64(serialized);

    EXPECT_EQ(a, b);

    a = -999999999;
    serialized = int64_to_byte_array(a);
    b = byte_array_to_int64(serialized);
    EXPECT_EQ(a, b);
}

class MockControlListener : public ControlListener {
public:
    std::vector<Control> handled_controls;

    void HandleControl(Control control) override {
        handled_controls.push_back(control);
    }
};

TEST(Serializer, Basics) {
    const int WRITE_BUF_SIZE = 256;
    uint8_t write_buf[WRITE_BUF_SIZE]{};
    Control c1{1, 2};
    MockControlListener listener;
    LoopbackSerial loopback(write_buf);

    SimpleSerializer simple_serializer(loopback);
    Serializer &s = simple_serializer;
    Deserializer &d = simple_serializer;

    d.AddControlListener(listener);
    EXPECT_EQ(listener.handled_controls.size(), 0);
    s.WriteControl(c1);
    EXPECT_GT(loopback.Available(), 0);

//    simple_serializer.Process();
//    EXPECT_EQ(listener.handled_controls.size(), 1);
}