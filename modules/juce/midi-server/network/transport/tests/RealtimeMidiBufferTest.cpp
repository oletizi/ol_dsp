#include <gtest/gtest.h>
#include "../RealtimeMidiBuffer.h"
#include <thread>
#include <vector>

using namespace NetworkMidi;

/**
 * Unit tests for RealtimeMidiBuffer.
 *
 * Tests cover:
 * - Basic write/read operations
 * - Burst handling (2000 msg/sec for 1 second)
 * - Overflow behavior (drop oldest)
 * - Statistics accuracy
 * - Multi-threaded stress test
 */

class RealtimeMidiBufferTest : public ::testing::Test {
protected:
    RealtimeMidiBuffer buffer;

    RealtimeMidiBuffer::MidiPacket createTestPacket(uint8_t note) {
        RealtimeMidiBuffer::MidiPacket packet;
        packet.data[0] = 0x90;  // Note On
        packet.data[1] = note;
        packet.data[2] = 100;   // Velocity
        packet.length = 3;
        packet.deviceId = 0;
        packet.timestamp = 12345;
        return packet;
    }
};

TEST_F(RealtimeMidiBufferTest, WriteAndReadSingleMessage) {
    auto packet = createTestPacket(60);  // Middle C

    EXPECT_TRUE(buffer.write(packet));

    RealtimeMidiBuffer::MidiPacket readPackets[1];
    int count = buffer.readBatch(readPackets, 1);

    EXPECT_EQ(count, 1);
    EXPECT_EQ(readPackets[0].data[0], 0x90);
    EXPECT_EQ(readPackets[0].data[1], 60);
    EXPECT_EQ(readPackets[0].data[2], 100);
    EXPECT_EQ(readPackets[0].length, 3);
}

TEST_F(RealtimeMidiBufferTest, WriteMultipleReadBatch) {
    // Write 10 messages
    for (uint8_t i = 0; i < 10; ++i) {
        auto packet = createTestPacket(60 + i);
        EXPECT_TRUE(buffer.write(packet));
    }

    // Read in batch
    RealtimeMidiBuffer::MidiPacket readPackets[10];
    int count = buffer.readBatch(readPackets, 10);

    EXPECT_EQ(count, 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(readPackets[i].data[1], 60 + i);
    }
}

TEST_F(RealtimeMidiBufferTest, BurstHandling2000MessagesPerSecond) {
    // Simulate 2000 msg/sec burst for 1 second
    const int BURST_COUNT = 2000;

    for (int i = 0; i < BURST_COUNT; ++i) {
        auto packet = createTestPacket(static_cast<uint8_t>(60 + (i % 12)));
        buffer.write(packet);
    }

    auto stats = buffer.getStats();

    // Verify: drop rate should be <1% for 2000 msg burst with 2048 capacity
    EXPECT_LT(stats.dropRate, 1.0f);
    EXPECT_GE(stats.written, static_cast<uint64_t>(BURST_COUNT * 0.99));  // At least 99% written
}

TEST_F(RealtimeMidiBufferTest, OverflowBehaviorDropOldest) {
    // Fill buffer to capacity
    for (int i = 0; i < RealtimeMidiBuffer::CAPACITY; ++i) {
        auto packet = createTestPacket(60);
        EXPECT_TRUE(buffer.write(packet));
    }

    // Write one more message (should trigger drop-oldest)
    auto newPacket = createTestPacket(72);  // Special note
    EXPECT_TRUE(buffer.write(newPacket));

    auto stats = buffer.getStats();
    EXPECT_EQ(stats.dropped, 1u);
    EXPECT_EQ(stats.numReady, RealtimeMidiBuffer::CAPACITY);

    // Read all messages - last one should be our special note
    RealtimeMidiBuffer::MidiPacket readPackets[RealtimeMidiBuffer::CAPACITY + 1];
    int count = buffer.readBatch(readPackets, RealtimeMidiBuffer::CAPACITY + 1);

    EXPECT_EQ(count, RealtimeMidiBuffer::CAPACITY);
    // The special note should be in the buffer (oldest was dropped)
    bool foundSpecialNote = false;
    for (int i = 0; i < count; ++i) {
        if (readPackets[i].data[1] == 72) {
            foundSpecialNote = true;
            break;
        }
    }
    EXPECT_TRUE(foundSpecialNote);
}

TEST_F(RealtimeMidiBufferTest, StatisticsAccuracy) {
    // Write 100 messages
    for (int i = 0; i < 100; ++i) {
        buffer.write(createTestPacket(60));
    }

    auto stats1 = buffer.getStats();
    EXPECT_EQ(stats1.written, 100u);
    EXPECT_EQ(stats1.dropped, 0u);
    EXPECT_EQ(stats1.numReady, 100);

    // Read 50 messages
    RealtimeMidiBuffer::MidiPacket readPackets[50];
    buffer.readBatch(readPackets, 50);

    auto stats2 = buffer.getStats();
    EXPECT_EQ(stats2.read, 50u);
    EXPECT_EQ(stats2.numReady, 50);
    EXPECT_EQ(stats2.freeSpace, RealtimeMidiBuffer::CAPACITY - 50);
}

TEST_F(RealtimeMidiBufferTest, MultiThreadedStressTest) {
    std::atomic<int> totalWritten{0};
    std::atomic<int> totalRead{0};
    std::atomic<bool> stop{false};

    // Writer thread (producer)
    std::thread writer([&] {
        while (!stop) {
            auto packet = createTestPacket(60);
            if (buffer.write(packet)) {
                totalWritten++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));  // 2000 msg/sec
        }
    });

    // Reader thread (consumer)
    std::thread reader([&] {
        while (!stop) {
            RealtimeMidiBuffer::MidiPacket packets[32];
            int count = buffer.readBatch(packets, 32);
            totalRead += count;
            if (count == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop = true;

    writer.join();
    reader.join();

    // Verify: should have written ~2000 messages, read close to that
    EXPECT_GT(totalWritten, 1500);  // At least 75% of target (allow for timing variance)
    EXPECT_GT(totalRead, totalWritten * 0.95);  // At least 95% read (allow for small buffer lag)

    auto stats = buffer.getStats();
    EXPECT_LT(stats.dropRate, 5.0f);  // <5% drop rate under stress
}

TEST_F(RealtimeMidiBufferTest, EmptyBufferReadReturnsZero) {
    RealtimeMidiBuffer::MidiPacket packets[10];
    int count = buffer.readBatch(packets, 10);
    EXPECT_EQ(count, 0);
}

TEST_F(RealtimeMidiBufferTest, PartialBatchRead) {
    // Write 5 messages
    for (int i = 0; i < 5; ++i) {
        buffer.write(createTestPacket(60 + i));
    }

    // Try to read 10 (only 5 available)
    RealtimeMidiBuffer::MidiPacket packets[10];
    int count = buffer.readBatch(packets, 10);
    EXPECT_EQ(count, 5);
}
