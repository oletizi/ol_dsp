#include "../../network/transport/MidiMessageRouter.h"
#include "../../network/transport/UdpMidiTransport.h"
#include "../../network/transport/ReliableTransport.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <thread>
#include <atomic>

using namespace NetworkMidi;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

/**
 * Unit tests for MidiMessageRouter.
 *
 * Tests verify that:
 * - Real-time messages are routed to UDP transport
 * - Non-real-time messages are routed to TCP transport
 * - Statistics are updated correctly
 * - Thread-safe routing works under load
 */
class MidiMessageRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create transports
        udpTransport = std::make_unique<UdpMidiTransport>(0);  // Auto-assign port
        udpTransport->setNodeId(juce::Uuid());

        reliableTransport = std::make_unique<ReliableTransport>(*udpTransport);

        // Create router
        router = std::make_unique<MidiMessageRouter>(*udpTransport, *reliableTransport);

        // Set up callbacks to track routing
        udpSendCount = 0;
        tcpSendCount = 0;

        udpTransport->onPacketReceived = [this](const MidiPacket& packet,
                                                 const juce::String& sourceAddress,
                                                 int sourcePort) {
            udpSendCount++;
        };
    }

    void TearDown() override {
        router.reset();
        reliableTransport.reset();
        udpTransport.reset();
    }

    std::unique_ptr<UdpMidiTransport> udpTransport;
    std::unique_ptr<ReliableTransport> reliableTransport;
    std::unique_ptr<MidiMessageRouter> router;

    std::atomic<int> udpSendCount;
    std::atomic<int> tcpSendCount;

    // Test destination
    const juce::String testAddress = "127.0.0.1";
    const int testPort = 5004;
    const juce::Uuid testDestNode = juce::Uuid();
    const uint16_t testDeviceId = 0;
};

// ============================================================================
// Basic Routing Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, RouteNoteOnToRealtime)
{
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);

    bool success = router->routeMessage(
        noteOn,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort
    );

    EXPECT_TRUE(success);

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 1);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 0);
}

TEST_F(MidiMessageRouterTest, RouteSysExToNonRealtime)
{
    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);

    bool success = router->routeMessage(
        sysex,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort
    );

    EXPECT_TRUE(success);

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 0);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 1);
}

TEST_F(MidiMessageRouterTest, RouteControlChangeToRealtime)
{
    juce::MidiMessage cc = juce::MidiMessage::controllerEvent(1, 7, 127);

    bool success = router->routeMessage(
        cc,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort
    );

    EXPECT_TRUE(success);

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 1);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 0);
}

TEST_F(MidiMessageRouterTest, RouteMidiClockToRealtime)
{
    juce::MidiMessage clock = juce::MidiMessage::midiClock();

    bool success = router->routeMessage(
        clock,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort
    );

    EXPECT_TRUE(success);

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 1);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 0);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, StatisticsCountCorrectly)
{
    // Send mixed messages
    router->routeMessage(
        juce::MidiMessage::noteOn(1, 60, 0.8f),
        testDeviceId, testDestNode, testAddress, testPort
    );

    router->routeMessage(
        juce::MidiMessage::noteOff(1, 60, 0.5f),
        testDeviceId, testDestNode, testAddress, testPort
    );

    router->routeMessage(
        juce::MidiMessage::controllerEvent(1, 7, 127),
        testDeviceId, testDestNode, testAddress, testPort
    );

    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);
    router->routeMessage(
        sysex,
        testDeviceId, testDestNode, testAddress, testPort
    );

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 3);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 1);
    EXPECT_GT(stats.totalBytesSent, 0);
}

TEST_F(MidiMessageRouterTest, ResetStatistics)
{
    // Send some messages
    router->routeMessage(
        juce::MidiMessage::noteOn(1, 60, 0.8f),
        testDeviceId, testDestNode, testAddress, testPort
    );

    router->routeMessage(
        juce::MidiMessage::noteOff(1, 60, 0.5f),
        testDeviceId, testDestNode, testAddress, testPort
    );

    // Verify stats are non-zero
    auto stats = router->getStatistics();
    EXPECT_GT(stats.realtimeMessagesSent, 0);

    // Reset
    router->resetStatistics();

    // Verify stats are zero
    stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, 0);
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 0);
    EXPECT_EQ(stats.totalBytesSent, 0);
}

TEST_F(MidiMessageRouterTest, DetailedStatisticsTracking)
{
    // Enable detailed tracking
    router->setDetailedTracking(true);
    EXPECT_TRUE(router->isDetailedTrackingEnabled());

    // Send various message types
    router->routeMessage(
        juce::MidiMessage::noteOn(1, 60, 0.8f),
        testDeviceId, testDestNode, testAddress, testPort
    );

    router->routeMessage(
        juce::MidiMessage::controllerEvent(1, 7, 127),
        testDeviceId, testDestNode, testAddress, testPort
    );

    router->routeMessage(
        juce::MidiMessage::midiClock(),
        testDeviceId, testDestNode, testAddress, testPort
    );

    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);
    router->routeMessage(
        sysex,
        testDeviceId, testDestNode, testAddress, testPort
    );

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.noteMessages, 1);
    EXPECT_EQ(stats.controlChangeMessages, 1);
    EXPECT_EQ(stats.clockMessages, 1);
    EXPECT_EQ(stats.sysexMessages, 1);
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, RouteWithCallbackRealtime)
{
    std::atomic<bool> callbackInvoked{false};

    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);

    bool success = router->routeMessageWithCallback(
        noteOn,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort,
        [&callbackInvoked]() {
            callbackInvoked = true;
        },
        nullptr
    );

    EXPECT_TRUE(success);

    // For real-time messages, callback is invoked immediately (async)
    // Wait a bit for async callback
    for (int i = 0; i < 10 && !callbackInvoked; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(callbackInvoked);
}

TEST_F(MidiMessageRouterTest, RouteWithCallbackNonRealtime)
{
    std::atomic<bool> deliveredCallbackInvoked{false};
    std::atomic<bool> failedCallbackInvoked{false};

    uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
    juce::MidiMessage sysex(sysexData, 5);

    bool success = router->routeMessageWithCallback(
        sysex,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort,
        [&deliveredCallbackInvoked]() {
            deliveredCallbackInvoked = true;
        },
        [&failedCallbackInvoked](const juce::String& reason) {
            failedCallbackInvoked = true;
        }
    );

    EXPECT_TRUE(success);

    // Non-real-time messages use reliable transport
    // Callbacks depend on ACK/timeout (tested in ReliableTransport tests)
    auto stats = router->getStatistics();
    EXPECT_EQ(stats.nonRealtimeMessagesSent, 1);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, ConcurrentRoutingFromMultipleThreads)
{
    const int numThreads = 10;
    const int messagesPerThread = 100;
    std::atomic<int> successCount{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, &successCount, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60 + (i % 12), 0.8f);
                bool success = router->routeMessage(
                    noteOn,
                    testDeviceId,
                    testDestNode,
                    testAddress,
                    testPort
                );
                if (success) {
                    successCount++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All messages should be routed successfully
    EXPECT_EQ(successCount.load(), numThreads * messagesPerThread);

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, numThreads * messagesPerThread);
}

TEST_F(MidiMessageRouterTest, MixedMessageTypesUnderLoad)
{
    const int numMessages = 1000;
    std::atomic<int> realtimeCount{0};
    std::atomic<int> nonRealtimeCount{0};

    std::thread sender([this, &realtimeCount, &nonRealtimeCount, numMessages]() {
        for (int i = 0; i < numMessages; ++i) {
            if (i % 10 == 0) {
                // Send SysEx (non-real-time)
                uint8_t sysexData[] = {0xF0, 0x43, 0x12, 0x00, 0xF7};
                juce::MidiMessage sysex(sysexData, 5);
                router->routeMessage(
                    sysex,
                    testDeviceId,
                    testDestNode,
                    testAddress,
                    testPort
                );
                nonRealtimeCount++;
            }
            else {
                // Send Note On (real-time)
                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60 + (i % 12), 0.8f);
                router->routeMessage(
                    noteOn,
                    testDeviceId,
                    testDestNode,
                    testAddress,
                    testPort
                );
                realtimeCount++;
            }
        }
    });

    sender.join();

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, realtimeCount.load());
    EXPECT_EQ(stats.nonRealtimeMessagesSent, nonRealtimeCount.load());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, RoutingPerformance)
{
    const int iterations = 10000;
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);

    auto startTime = juce::Time::getHighResolutionTicks();

    for (int i = 0; i < iterations; ++i) {
        router->routeMessage(
            noteOn,
            testDeviceId,
            testDestNode,
            testAddress,
            testPort
        );
    }

    auto endTime = juce::Time::getHighResolutionTicks();
    auto elapsedSeconds = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    auto avgTimeMicros = (elapsedSeconds / iterations) * 1e6;

    // Routing should take < 10μs per message
    EXPECT_LT(avgTimeMicros, 10.0) << "Routing too slow: " << avgTimeMicros << "μs";

    std::cout << "Average routing time: " << avgTimeMicros << "μs" << std::endl;

    auto stats = router->getStatistics();
    EXPECT_EQ(stats.realtimeMessagesSent, iterations);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(MidiMessageRouterTest, ErrorCallbackOnFailure)
{
    std::atomic<bool> errorCallbackInvoked{false};
    juce::String lastError;

    router->onRoutingError = [&errorCallbackInvoked, &lastError](
        const juce::String& error,
        const juce::MidiMessage& msg
    ) {
        errorCallbackInvoked = true;
        lastError = error;
    };

    // Stop UDP transport to force send failure
    udpTransport->stop();

    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    bool success = router->routeMessage(
        noteOn,
        testDeviceId,
        testDestNode,
        testAddress,
        testPort
    );

    // Note: sendPacket will fail, but routeMessage returns success if queued
    // Error handling depends on transport implementation

    auto stats = router->getStatistics();
    EXPECT_GT(stats.routingErrors, 0);
}
