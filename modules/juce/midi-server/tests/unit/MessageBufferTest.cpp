/**
 * MessageBufferTest.cpp
 *
 * Unit tests for MessageBuffer
 * Tests: packet reordering, duplicates, wraparound, gap handling, timeouts
 *
 * Coverage Target: 80%+
 */

#include "network/transport/MessageBuffer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

using namespace NetworkMidi;
using namespace testing;

class MessageBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = MessageBuffer::Config();
        config.maxBufferSize = 10;
        config.maxSequenceGap = 5;
        config.allowDuplicates = false;
        config.deliveryTimeoutMs = 100;

        buffer = std::make_unique<MessageBuffer>(config);

        sourceNode = juce::Uuid();
        destNode = juce::Uuid();
        deviceId = 42;
    }

    MidiPacket createPacket(uint16_t sequence, const std::vector<uint8_t>& data = {0x90, 0x3C, 0x64}) {
        return MidiPacket::createDataPacket(sourceNode, destNode, deviceId, data, sequence);
    }

    MessageBuffer::Config config;
    std::unique_ptr<MessageBuffer> buffer;
    juce::Uuid sourceNode;
    juce::Uuid destNode;
    uint16_t deviceId;
};

// Test in-order packet delivery
TEST_F(MessageBufferTest, DeliversInOrderPackets) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));
    buffer->addPacket(createPacket(2));

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 1, 2));
}

// Test out-of-order packet reordering
TEST_F(MessageBufferTest, ReordersOutOfOrderPackets) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    // Receive out of order: 0, 2, 1
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(2));  // Buffered
    buffer->addPacket(createPacket(1));  // Triggers delivery of 1 and 2

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 1, 2));
}

// Test duplicate detection
TEST_F(MessageBufferTest, DetectsDuplicates) {
    std::vector<uint16_t> deliveredSequences;
    std::vector<uint16_t> duplicateSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->onDuplicateDetected = [&](uint16_t sequence) {
        duplicateSequences.push_back(sequence);
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));
    buffer->addPacket(createPacket(1));  // Duplicate

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 1));
    EXPECT_THAT(duplicateSequences, ElementsAre(1));
}

// Test duplicate delivery when allowed
TEST_F(MessageBufferTest, DeliversDuplicatesWhenAllowed) {
    config.allowDuplicates = true;
    buffer = std::make_unique<MessageBuffer>(config);

    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));
    buffer->addPacket(createPacket(1));  // Duplicate

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 1, 1));
}

// Test gap detection
TEST_F(MessageBufferTest, DetectsGaps) {
    std::vector<uint16_t> gapSequences;

    buffer->onGapDetected = [&](uint16_t sequence) {
        gapSequences.push_back(sequence);
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(3));  // Gap: 1 and 2 missing

    EXPECT_THAT(gapSequences, ElementsAre(1, 2));
}

// Test large gap handling (skip forward)
TEST_F(MessageBufferTest, SkipsForwardOnLargeGap) {
    std::vector<uint16_t> deliveredSequences;
    std::vector<uint16_t> gapSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->onGapDetected = [&](uint16_t sequence) {
        gapSequences.push_back(sequence);
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(10));  // Gap too large (maxSequenceGap = 5)

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 10));
    EXPECT_EQ(10u, gapSequences.size());  // Gaps: 1-10
}

// Test buffer overflow
TEST_F(MessageBufferTest, HandlesBufferOverflow) {
    config.maxBufferSize = 3;
    buffer = std::make_unique<MessageBuffer>(config);

    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    // Receive: 0, then 2, 3, 4, 5 (buffer size = 3)
    buffer->addPacket(createPacket(0));  // Delivered immediately
    buffer->addPacket(createPacket(2));  // Buffered
    buffer->addPacket(createPacket(3));  // Buffered
    buffer->addPacket(createPacket(4));  // Buffered
    buffer->addPacket(createPacket(5));  // Causes overflow, drops oldest

    // Buffer should drop packet 2 to make room
    buffer->addPacket(createPacket(1));  // Triggers delivery

    EXPECT_THAT(deliveredSequences, Contains(0));
    EXPECT_THAT(deliveredSequences, Contains(1));
}

// Test sequence wraparound
TEST_F(MessageBufferTest, HandlesSequenceWraparound) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->setNextExpectedSequence(65534);

    buffer->addPacket(createPacket(65534));
    buffer->addPacket(createPacket(65535));
    buffer->addPacket(createPacket(0));  // Wraparound
    buffer->addPacket(createPacket(1));

    EXPECT_THAT(deliveredSequences, ElementsAre(65534, 65535, 0, 1));
}

// Test reset functionality
TEST_F(MessageBufferTest, ResetClearsState) {
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(2));  // Buffered

    buffer->reset();

    EXPECT_EQ(0, buffer->getNextExpectedSequence());

    auto stats = buffer->getStatistics();
    EXPECT_EQ(0u, stats.currentBufferSize);
}

// Test statistics tracking
TEST_F(MessageBufferTest, TracksStatistics) {
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(2));
    buffer->addPacket(createPacket(1));

    auto stats = buffer->getStatistics();

    EXPECT_EQ(3u, stats.packetsReceived);
    EXPECT_EQ(3u, stats.packetsDelivered);
    EXPECT_EQ(1u, stats.packetsReordered);
}

// Test duplicate statistics
TEST_F(MessageBufferTest, TracksDuplicateStatistics) {
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));
    buffer->addPacket(createPacket(1));

    auto stats = buffer->getStatistics();

    EXPECT_EQ(1u, stats.duplicates);
}

// Test gap detection statistics
TEST_F(MessageBufferTest, TracksGapStatistics) {
    buffer->onGapDetected = [](uint16_t) {};

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(3));

    auto stats = buffer->getStatistics();

    EXPECT_GE(stats.gapsDetected, 1u);
}

// Test statistics reset
TEST_F(MessageBufferTest, ResetsStatistics) {
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));

    auto stats1 = buffer->getStatistics();
    EXPECT_GT(stats1.packetsReceived, 0u);

    buffer->resetStatistics();

    auto stats2 = buffer->getStatistics();
    EXPECT_EQ(0u, stats2.packetsReceived);
}

// Test max buffer size tracking
TEST_F(MessageBufferTest, TracksMaxBufferSize) {
    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(5));
    buffer->addPacket(createPacket(6));
    buffer->addPacket(createPacket(7));

    auto stats = buffer->getStatistics();

    EXPECT_GT(stats.maxBufferSizeReached, 0u);
}

// Test old packet dropping
TEST_F(MessageBufferTest, DropsOldPackets) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->addPacket(createPacket(5));
    buffer->addPacket(createPacket(6));
    buffer->addPacket(createPacket(3));  // Old packet (before expected)

    auto stats = buffer->getStatistics();
    EXPECT_GT(stats.packetsDropped, 0u);
}

// Test sequence difference calculation
TEST_F(MessageBufferTest, CalculatesSequenceDifference) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->setNextExpectedSequence(100);

    buffer->addPacket(createPacket(100));
    buffer->addPacket(createPacket(102));
    buffer->addPacket(createPacket(101));

    EXPECT_THAT(deliveredSequences, ElementsAre(100, 101, 102));
}

// Test next expected sequence update
TEST_F(MessageBufferTest, UpdatesNextExpectedSequence) {
    buffer->setNextExpectedSequence(50);
    EXPECT_EQ(50, buffer->getNextExpectedSequence());

    buffer->addPacket(createPacket(50));
    EXPECT_EQ(51, buffer->getNextExpectedSequence());
}

// Test sequential packet delivery from buffer
TEST_F(MessageBufferTest, DeliversSequentialPacketsFromBuffer) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(1));
    buffer->addPacket(createPacket(3));
    buffer->addPacket(createPacket(4));
    buffer->addPacket(createPacket(2));  // Fills gap, triggers 2, 3, 4

    EXPECT_THAT(deliveredSequences, ElementsAre(0, 1, 2, 3, 4));
}

// Test timeout handling
TEST_F(MessageBufferTest, HandlesTimeouts) {
    std::vector<uint16_t> deliveredSequences;
    std::vector<uint16_t> gapSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->onGapDetected = [&](uint16_t sequence) {
        gapSequences.push_back(sequence);
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(2));  // Gap at 1

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Timeout checker should trigger gap detection and skip forward
    EXPECT_GT(gapSequences.size(), 0u);
}

// Test callback invocation
TEST_F(MessageBufferTest, InvokesCallbacks) {
    bool packetReadyCalled = false;
    bool duplicateCalled = false;
    bool gapCalled = false;

    buffer->onPacketReady = [&](const MidiPacket&) {
        packetReadyCalled = true;
    };

    buffer->onDuplicateDetected = [&](uint16_t) {
        duplicateCalled = true;
    };

    buffer->onGapDetected = [&](uint16_t) {
        gapCalled = true;
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(0));  // Duplicate
    buffer->addPacket(createPacket(3));  // Gap

    EXPECT_TRUE(packetReadyCalled);
    EXPECT_TRUE(duplicateCalled);
    EXPECT_TRUE(gapCalled);
}

// Test empty buffer state
TEST_F(MessageBufferTest, EmptyBufferState) {
    auto stats = buffer->getStatistics();

    EXPECT_EQ(0u, stats.packetsReceived);
    EXPECT_EQ(0u, stats.packetsDelivered);
    EXPECT_EQ(0u, stats.currentBufferSize);
}

// Test wraparound edge case
TEST_F(MessageBufferTest, HandlesWraparoundEdgeCase) {
    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->setNextExpectedSequence(65535);

    buffer->addPacket(createPacket(65535));
    buffer->addPacket(createPacket(0));

    EXPECT_THAT(deliveredSequences, ElementsAre(65535, 0));
}

// Test maximum sequence gap
TEST_F(MessageBufferTest, RespectsMaxSequenceGap) {
    config.maxSequenceGap = 3;
    buffer = std::make_unique<MessageBuffer>(config);

    std::vector<uint16_t> deliveredSequences;

    buffer->onPacketReady = [&](const MidiPacket& packet) {
        deliveredSequences.push_back(packet.getSequence());
    };

    buffer->addPacket(createPacket(0));
    buffer->addPacket(createPacket(5));  // Gap of 5 exceeds maxSequenceGap

    // Should skip forward
    EXPECT_THAT(deliveredSequences, Contains(5));
}
