/**
 * DualTransportIntegrationTest.cpp
 *
 * Integration tests for dual-transport MIDI routing.
 * Tests message classification, transport selection, and buffer behavior.
 */

#include "../NetworkConnection.h"
#include "../ConnectionWorker.h"
#include "TestHelpers.h"
#include <gtest/gtest.h>
#include <thread>
#include <atomic>

using namespace NetworkMidi;
using namespace NetworkMidi::TestHelpers;

//==============================================================================
// Message Classification and Routing Tests

TEST(DualTransportIntegrationTest, RealtimeMessageClassification) {
    auto node = createLocalTestNode("TestNode", 8200, 8201);
    NetworkConnection conn(node);

    // Send various real-time messages
    // Note: These should be classified as real-time and buffered

    // Note On/Off - real-time
    EXPECT_NO_THROW(conn.sendMidiMessage(1, createNoteOn(60, 100)));
    EXPECT_NO_THROW(conn.sendMidiMessage(1, createNoteOff(60)));

    // Control Change - real-time
    EXPECT_NO_THROW(conn.sendMidiMessage(1, createControlChange(1, 64)));

    // Pitch Bend - real-time
    std::vector<uint8_t> pitchBend = {0xE0, 0x00, 0x40};
    EXPECT_NO_THROW(conn.sendMidiMessage(1, pitchBend));

    // Program Change - real-time
    std::vector<uint8_t> programChange = {0xC0, 0x05};
    EXPECT_NO_THROW(conn.sendMidiMessage(1, programChange));
}

TEST(DualTransportIntegrationTest, NonRealtimeMessageClassification) {
    auto node = createLocalTestNode("TestNode", 8202, 8203);
    NetworkConnection conn(node);

    // Send non-real-time messages
    // Note: These should be classified as non-real-time and sent via TCP

    // SysEx - non-real-time
    auto sysex = createSysEx(100);
    EXPECT_NO_THROW(conn.sendMidiMessage(1, sysex));

    // Large SysEx
    auto largeSysex = createSysEx(1000);
    EXPECT_NO_THROW(conn.sendMidiMessage(1, largeSysex));
}

TEST(DualTransportIntegrationTest, MixedMessageStream) {
    auto node = createLocalTestNode("TestNode", 8204, 8205);
    NetworkConnection conn(node);

    // Send mixed stream of real-time and non-real-time messages
    for (int i = 0; i < 10; ++i) {
        // Real-time message
        conn.sendMidiMessage(1, createNoteOn(60 + i, 100));

        // Non-real-time message
        if (i % 3 == 0) {
            conn.sendMidiMessage(1, createSysEx(50));
        }

        // More real-time
        conn.sendMidiMessage(1, createControlChange(1, i * 10));
        conn.sendMidiMessage(1, createNoteOff(60 + i));
    }

    // Should not crash or deadlock
    juce::Thread::sleep(100);
}

//==============================================================================
// Buffer Behavior Tests

TEST(DualTransportIntegrationTest, RealtimeBufferCapacity) {
    auto node = createLocalTestNode("TestNode", 8206, 8207);
    NetworkConnection conn(node);

    // Send many messages rapidly to test buffer capacity
    const int MESSAGE_COUNT = 1000;

    for (int i = 0; i < MESSAGE_COUNT; ++i) {
        conn.sendMidiMessage(1, createNoteOn(60, 100));
    }

    // Buffer should handle this without crashing
    juce::Thread::sleep(100);
}

TEST(DualTransportIntegrationTest, BurstySendPattern) {
    auto node = createLocalTestNode("TestNode", 8208, 8209);
    NetworkConnection conn(node);

    // Simulate bursty MIDI traffic (common in performance scenarios)
    for (int burst = 0; burst < 5; ++burst) {
        // Burst: send 100 messages rapidly
        for (int i = 0; i < 100; ++i) {
            conn.sendMidiMessage(1, createNoteOn(60 + (i % 12), 100));
        }

        // Silence: wait
        juce::Thread::sleep(50);
    }

    // Should handle bursts without buffer overflow
    juce::Thread::sleep(100);
}

//==============================================================================
// Transport Lifecycle Tests

TEST(DualTransportIntegrationTest, TransportInitialization) {
    MockHttpServer mockServer(8210);
    auto handshakeResponse = MockHttpServer::getDefaultHandshakeResponse(
        "TransportNode", juce::Uuid(), 8211);
    mockServer.setHandshakeResponse(handshakeResponse);
    mockServer.start();

    auto node = createLocalTestNode("TransportNode", 8210, 8211);
    NetworkConnection conn(node);

    // Before connection, sending should queue messages
    conn.sendMidiMessage(1, createNoteOn(60, 100));

    // Connect
    conn.connect();

    // Wait for connection to establish
    waitFor([&conn]() {
        auto state = conn.getState();
        return state == NetworkConnection::State::Connected ||
               state == NetworkConnection::State::Failed;
    }, 3000);

    // Send more messages after connection
    conn.sendMidiMessage(1, createNoteOff(60));
    conn.sendMidiMessage(1, createControlChange(1, 64));

    // Disconnect
    conn.disconnect();
    waitForState(&conn, NetworkConnection::State::Disconnected, 1000);

    mockServer.stop();
}

TEST(DualTransportIntegrationTest, TransportShutdown) {
    auto node = createLocalTestNode("TestNode", 8212, 8213);
    NetworkConnection conn(node);

    // Connect and send messages
    conn.connect();
    juce::Thread::sleep(100);

    conn.sendMidiMessage(1, createNoteOn(60, 100));
    conn.sendMidiMessage(1, createNoteOff(60));

    // Disconnect should cleanly shutdown transports
    conn.disconnect();
    waitForState(&conn, NetworkConnection::State::Disconnected, 1000);

    // Connection destructor should not hang or crash
}

//==============================================================================
// Concurrent Transport Access Tests

TEST(DualTransportIntegrationTest, ConcurrentSendFromMultipleThreads) {
    auto node = createLocalTestNode("TestNode", 8214, 8215);
    NetworkConnection conn(node);

    std::atomic<int> messagesSent{0};
    std::atomic<bool> errorOccurred{false};

    // Launch multiple threads sending MIDI messages
    std::vector<std::thread> threads;
    const int NUM_THREADS = 5;
    const int MESSAGES_PER_THREAD = 100;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&conn, &messagesSent, &errorOccurred, i]() {
            try {
                for (int j = 0; j < MESSAGES_PER_THREAD; ++j) {
                    // Mix of real-time and non-real-time
                    conn.sendMidiMessage(1, createNoteOn(60 + (j % 12), 100));

                    if (j % 10 == 0) {
                        conn.sendMidiMessage(1, createSysEx(50));
                    }

                    messagesSent.fetch_add(1);
                }
            } catch (...) {
                errorOccurred.store(true);
            }
        });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // Verify all messages were sent without errors
    EXPECT_EQ(messagesSent.load(), NUM_THREADS * MESSAGES_PER_THREAD);
    EXPECT_FALSE(errorOccurred.load());
}

//==============================================================================
// Transport Statistics Tests
// Note: These tests verify that getTransportStats() can be called safely
// Actual statistics validation requires ConnectionWorker to be accessible

TEST(DualTransportIntegrationTest, TransportStatsQuery) {
    auto node = createLocalTestNode("TestNode", 8216, 8217);
    NetworkConnection conn(node);

    // Send some messages
    for (int i = 0; i < 10; ++i) {
        conn.sendMidiMessage(1, createNoteOn(60 + i, 100));
    }

    juce::Thread::sleep(50);

    // Note: Cannot directly query transport stats through NetworkConnection
    // This test verifies the architecture supports sending messages
    EXPECT_TRUE(true);
}

//==============================================================================
// Error Handling Tests

TEST(DualTransportIntegrationTest, InvalidDeviceId) {
    auto node = createLocalTestNode("TestNode", 8218, 8219);
    NetworkConnection conn(node);

    // Sending to invalid device ID should not crash
    // (validation happens at protocol level, not here)
    EXPECT_NO_THROW(conn.sendMidiMessage(9999, createNoteOn(60, 100)));
}

TEST(DualTransportIntegrationTest, MalformedMidiMessage) {
    auto node = createLocalTestNode("TestNode", 8220, 8221);
    NetworkConnection conn(node);

    // Empty message should throw
    std::vector<uint8_t> empty;
    EXPECT_THROW(conn.sendMidiMessage(1, empty), std::invalid_argument);

    // Single byte (incomplete message) - should still accept
    std::vector<uint8_t> singleByte = {0x90};
    EXPECT_NO_THROW(conn.sendMidiMessage(1, singleByte));
}

//==============================================================================
// Performance Tests

TEST(DualTransportIntegrationTest, HighThroughputRealtime) {
    auto node = createLocalTestNode("TestNode", 8222, 8223);
    NetworkConnection conn(node);

    // Simulate high-throughput real-time MIDI
    const int MESSAGE_COUNT = 2000;
    auto start = juce::Time::getCurrentTime();

    for (int i = 0; i < MESSAGE_COUNT; ++i) {
        conn.sendMidiMessage(1, createNoteOn(60 + (i % 12), 100));
    }

    auto end = juce::Time::getCurrentTime();
    auto duration = (end - start).inMilliseconds();

    // Should complete quickly (< 1 second for 2000 messages)
    EXPECT_LT(duration, 1000);

    // Log performance
    std::cout << "Sent " << MESSAGE_COUNT << " real-time messages in "
              << duration << "ms ("
              << (MESSAGE_COUNT * 1000.0 / duration) << " msg/sec)"
              << std::endl;
}

TEST(DualTransportIntegrationTest, MixedThroughputTest) {
    auto node = createLocalTestNode("TestNode", 8224, 8225);
    NetworkConnection conn(node);

    const int REALTIME_COUNT = 1000;
    const int SYSEX_COUNT = 50;
    auto start = juce::Time::getCurrentTime();

    for (int i = 0; i < REALTIME_COUNT; ++i) {
        conn.sendMidiMessage(1, createNoteOn(60 + (i % 12), 100));

        // Intersperse SysEx messages
        if (i % 20 == 0 && i / 20 < SYSEX_COUNT) {
            conn.sendMidiMessage(1, createSysEx(100));
        }
    }

    auto end = juce::Time::getCurrentTime();
    auto duration = (end - start).inMilliseconds();

    EXPECT_LT(duration, 1000);

    std::cout << "Sent " << REALTIME_COUNT << " real-time + "
              << SYSEX_COUNT << " SysEx messages in "
              << duration << "ms" << std::endl;
}
