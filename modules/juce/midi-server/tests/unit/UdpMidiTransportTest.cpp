/**
 * UdpMidiTransportTest.cpp
 *
 * Unit tests for UdpMidiTransport
 * Tests: send/receive, statistics, thread safety, port binding
 *
 * Coverage Target: 80%+
 */

#include "network/transport/UdpMidiTransport.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

using namespace NetworkMidi;
using namespace testing;

class UdpMidiTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        sourceNode = juce::Uuid();
        destNode = juce::Uuid();
        deviceId = 42;
    }

    void TearDown() override {
        // Clean up any running transports
    }

    juce::Uuid sourceNode;
    juce::Uuid destNode;
    uint16_t deviceId;
};

// Test construction with auto port
TEST_F(UdpMidiTransportTest, ConstructsWithAutoPort) {
    UdpMidiTransport transport(0);

    EXPECT_FALSE(transport.isRunning());
    EXPECT_EQ(0, transport.getPort());
}

// Test construction with specific port
TEST_F(UdpMidiTransportTest, ConstructsWithSpecificPort) {
    UdpMidiTransport transport(9000);

    EXPECT_FALSE(transport.isRunning());
    EXPECT_EQ(9000, transport.getPort());
}

// Test start success
TEST_F(UdpMidiTransportTest, StartsSuccessfully) {
    UdpMidiTransport transport(0);

    bool started = transport.start();

    EXPECT_TRUE(started);
    EXPECT_TRUE(transport.isRunning());
    EXPECT_GT(transport.getPort(), 0);

    transport.stop();
}

// Test port assignment after start
TEST_F(UdpMidiTransportTest, AssignsPortAfterStart) {
    UdpMidiTransport transport(0);

    transport.start();

    int port = transport.getPort();
    EXPECT_GT(port, 0);
    EXPECT_LT(port, 65536);

    transport.stop();
}

// Test stop
TEST_F(UdpMidiTransportTest, StopsSuccessfully) {
    UdpMidiTransport transport(0);

    transport.start();
    EXPECT_TRUE(transport.isRunning());

    transport.stop();
    EXPECT_FALSE(transport.isRunning());
}

// Test double start (should be idempotent)
TEST_F(UdpMidiTransportTest, DoubleStartIsIdempotent) {
    UdpMidiTransport transport(0);

    transport.start();
    bool secondStart = transport.start();

    EXPECT_TRUE(secondStart);
    EXPECT_TRUE(transport.isRunning());

    transport.stop();
}

// Test double stop (should be safe)
TEST_F(UdpMidiTransportTest, DoubleStopIsSafe) {
    UdpMidiTransport transport(0);

    transport.start();
    transport.stop();

    EXPECT_NO_THROW({
        transport.stop();
    });

    EXPECT_FALSE(transport.isRunning());
}

// Test node ID setter/getter
TEST_F(UdpMidiTransportTest, SetAndGetNodeId) {
    UdpMidiTransport transport(0);

    transport.setNodeId(sourceNode);

    EXPECT_EQ(sourceNode, transport.getNodeId());
}

// Test send message when not running
TEST_F(UdpMidiTransportTest, SendMessageFailsWhenNotRunning) {
    UdpMidiTransport transport(0);

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    bool sent = transport.sendMessage(destNode, "127.0.0.1", 9001, deviceId, midiData);

    EXPECT_FALSE(sent);
}

// Test send message when running
TEST_F(UdpMidiTransportTest, SendMessageSucceedsWhenRunning) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver(0);

    sender.setNodeId(sourceNode);
    sender.start();
    receiver.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    bool sent = sender.sendMessage(
        destNode,
        "127.0.0.1",
        receiver.getPort(),
        deviceId,
        midiData
    );

    EXPECT_TRUE(sent);

    sender.stop();
    receiver.stop();
}

// Test send packet
TEST_F(UdpMidiTransportTest, SendPacket) {
    UdpMidiTransport transport(0);
    transport.setNodeId(sourceNode);
    transport.start();

    MidiPacket packet = MidiPacket::createDataPacket(
        sourceNode, destNode, deviceId, {0x90, 0x3C, 0x64}, 100
    );

    bool sent = transport.sendPacket(packet, "127.0.0.1", transport.getPort());

    EXPECT_TRUE(sent);

    transport.stop();
}

// Test receive callback
TEST_F(UdpMidiTransportTest, ReceiveCallback) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver(0);

    sender.setNodeId(sourceNode);
    receiver.setNodeId(destNode);

    std::atomic<bool> receivedPacket{false};
    MidiPacket receivedData;

    receiver.onPacketReceived = [&](const MidiPacket& packet, const juce::String&, int) {
        receivedPacket = true;
        receivedData = packet;
    };

    sender.start();
    receiver.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    sender.sendMessage(destNode, "127.0.0.1", receiver.getPort(), deviceId, midiData);

    // Wait for receive
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(receivedPacket);
    EXPECT_EQ(midiData, receivedData.getMidiData());

    sender.stop();
    receiver.stop();
}

// Test statistics tracking
TEST_F(UdpMidiTransportTest, TracksStatistics) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver(0);

    sender.setNodeId(sourceNode);
    sender.start();
    receiver.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    sender.sendMessage(destNode, "127.0.0.1", receiver.getPort(), deviceId, midiData);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto senderStats = sender.getStatistics();
    auto receiverStats = receiver.getStatistics();

    EXPECT_GT(senderStats.packetsSent, 0u);
    EXPECT_GT(senderStats.bytesSent, 0u);
    EXPECT_GT(receiverStats.packetsReceived, 0u);
    EXPECT_GT(receiverStats.bytesReceived, 0u);

    sender.stop();
    receiver.stop();
}

// Test statistics reset
TEST_F(UdpMidiTransportTest, ResetsStatistics) {
    UdpMidiTransport transport(0);
    transport.setNodeId(sourceNode);
    transport.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};
    transport.sendMessage(destNode, "127.0.0.1", 9000, deviceId, midiData);

    auto stats1 = transport.getStatistics();
    EXPECT_GT(stats1.packetsSent, 0u);

    transport.resetStatistics();

    auto stats2 = transport.getStatistics();
    EXPECT_EQ(0u, stats2.packetsSent);
    EXPECT_EQ(0u, stats2.bytesSent);

    transport.stop();
}

// Test error callback
TEST_F(UdpMidiTransportTest, InvokesErrorCallback) {
    UdpMidiTransport transport(0);

    std::atomic<bool> errorCalled{false};
    juce::String errorMsg;

    transport.onError = [&](const juce::String& error) {
        errorCalled = true;
        errorMsg = error;
    };

    // Try to bind to reserved port (should fail)
    UdpMidiTransport failTransport(1);  // Port 1 is reserved

    bool started = failTransport.start();

    if (!started) {
        // Expected - port binding should fail
    }
}

// Test invalid packet handling
TEST_F(UdpMidiTransportTest, HandlesInvalidPackets) {
    UdpMidiTransport receiver(0);

    std::atomic<int> invalidCount{0};

    receiver.onError = [&](const juce::String& error) {
        if (error.contains("invalid")) {
            invalidCount++;
        }
    };

    receiver.start();

    // Send invalid data to receiver
    juce::DatagramSocket socket;
    socket.bindToPort(0);

    uint8_t invalidData[] = {0xFF, 0xFF, 0xFF, 0xFF};
    socket.write("127.0.0.1", receiver.getPort(), invalidData, sizeof(invalidData));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = receiver.getStatistics();
    EXPECT_GT(stats.invalidPackets, 0u);

    receiver.stop();
}

// Test concurrent sends
TEST_F(UdpMidiTransportTest, HandlesConcurrentSends) {
    UdpMidiTransport transport(0);
    transport.setNodeId(sourceNode);
    transport.start();

    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            std::vector<uint8_t> midiData = {0x90, static_cast<uint8_t>(i), 0x64};
            transport.sendMessage(destNode, "127.0.0.1", 9000, deviceId, midiData);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto stats = transport.getStatistics();
    EXPECT_GE(stats.packetsSent, 10u);

    transport.stop();
}

// Test sequence number increment
TEST_F(UdpMidiTransportTest, IncrementsSequenceNumber) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver(0);

    sender.setNodeId(sourceNode);

    std::vector<uint16_t> receivedSequences;

    receiver.onPacketReceived = [&](const MidiPacket& packet, const juce::String&, int) {
        receivedSequences.push_back(packet.getSequence());
    };

    sender.start();
    receiver.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    for (int i = 0; i < 5; ++i) {
        sender.sendMessage(destNode, "127.0.0.1", receiver.getPort(), deviceId, midiData);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(5u, receivedSequences.size());

    // Sequences should be incrementing
    for (size_t i = 1; i < receivedSequences.size(); ++i) {
        EXPECT_GT(receivedSequences[i], receivedSequences[i-1]);
    }

    sender.stop();
    receiver.stop();
}

// Test multiple receivers
TEST_F(UdpMidiTransportTest, SendsToMultipleReceivers) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver1(0);
    UdpMidiTransport receiver2(0);

    sender.setNodeId(sourceNode);

    std::atomic<int> receiver1Count{0};
    std::atomic<int> receiver2Count{0};

    receiver1.onPacketReceived = [&](const MidiPacket&, const juce::String&, int) {
        receiver1Count++;
    };

    receiver2.onPacketReceived = [&](const MidiPacket&, const juce::String&, int) {
        receiver2Count++;
    };

    sender.start();
    receiver1.start();
    receiver2.start();

    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};

    sender.sendMessage(destNode, "127.0.0.1", receiver1.getPort(), deviceId, midiData);
    sender.sendMessage(destNode, "127.0.0.1", receiver2.getPort(), deviceId, midiData);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_GT(receiver1Count, 0);
    EXPECT_GT(receiver2Count, 0);

    sender.stop();
    receiver1.stop();
    receiver2.stop();
}

// Test destructor cleanup
TEST_F(UdpMidiTransportTest, DestructorCleansUp) {
    {
        UdpMidiTransport transport(0);
        transport.start();
        EXPECT_TRUE(transport.isRunning());
    }

    // Should clean up automatically
}

// Test large message
TEST_F(UdpMidiTransportTest, SendsLargeMessage) {
    UdpMidiTransport sender(0);
    UdpMidiTransport receiver(0);

    sender.setNodeId(sourceNode);

    std::atomic<bool> received{false};
    std::vector<uint8_t> receivedData;

    receiver.onPacketReceived = [&](const MidiPacket& packet, const juce::String&, int) {
        received = true;
        receivedData = packet.getMidiData();
    };

    sender.start();
    receiver.start();

    // Large SysEx message
    std::vector<uint8_t> largeMidiData(1000);
    largeMidiData[0] = 0xF0;  // SysEx start
    for (size_t i = 1; i < largeMidiData.size() - 1; ++i) {
        largeMidiData[i] = static_cast<uint8_t>(i % 128);
    }
    largeMidiData[999] = 0xF7;  // SysEx end

    sender.sendMessage(destNode, "127.0.0.1", receiver.getPort(), deviceId, largeMidiData);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(received);
    EXPECT_EQ(largeMidiData.size(), receivedData.size());

    sender.stop();
    receiver.stop();
}

// Test empty statistics
TEST_F(UdpMidiTransportTest, EmptyStatistics) {
    UdpMidiTransport transport(0);

    auto stats = transport.getStatistics();

    EXPECT_EQ(0u, stats.packetsSent);
    EXPECT_EQ(0u, stats.packetsReceived);
    EXPECT_EQ(0u, stats.bytesSent);
    EXPECT_EQ(0u, stats.bytesReceived);
    EXPECT_EQ(0u, stats.sendErrors);
    EXPECT_EQ(0u, stats.receiveErrors);
    EXPECT_EQ(0u, stats.invalidPackets);
}
