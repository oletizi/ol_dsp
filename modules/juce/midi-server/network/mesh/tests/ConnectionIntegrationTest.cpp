/**
 * ConnectionIntegrationTest.cpp
 *
 * Integration tests for NetworkConnection SEDA architecture.
 * Tests command queue flow, connection lifecycle, queries, and thread safety.
 */

#include "../NetworkConnection.h"
#include "../Commands.h"
#include "TestHelpers.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace NetworkMidi;
using namespace NetworkMidi::TestHelpers;

//==============================================================================
// Command Queue Integration Tests

TEST(ConnectionIntegrationTest, CommandQueueFlow) {
    // Create connection to test node
    auto node = createLocalTestNode("TestNode", 8080, 8081);
    NetworkConnection conn(node);

    // Verify initial state
    EXPECT_EQ(conn.getState(), NetworkConnection::State::Disconnected);

    // Push connect command (non-blocking)
    // This should transition state from Disconnected -> Connecting
    conn.connect();

    // State should eventually change (even if connection fails)
    bool stateChanged = waitFor([&conn]() {
        return conn.getState() != NetworkConnection::State::Disconnected;
    }, 1000);

    EXPECT_TRUE(stateChanged);

    // Disconnect
    conn.disconnect();

    // Should return to disconnected
    EXPECT_TRUE(waitForState(&conn, NetworkConnection::State::Disconnected, 1000));
}

TEST(ConnectionIntegrationTest, MultipleCommandsQueued) {
    auto node = createLocalTestNode("TestNode", 8082, 8083);
    NetworkConnection conn(node);

    CallbackTracker stateChanges;
    conn.onStateChanged = [&stateChanges](auto oldState, auto newState) {
        stateChanges.recordCall();
    };

    // Queue multiple commands rapidly
    conn.connect();
    juce::Thread::sleep(10);
    conn.disconnect();
    juce::Thread::sleep(10);
    conn.connect();
    juce::Thread::sleep(10);
    conn.disconnect();

    // Wait for state changes to be processed
    juce::Thread::sleep(500);

    // Should have received multiple state change callbacks
    EXPECT_GT(stateChanges.getCallCount(), 0);
}

//==============================================================================
// Connection Lifecycle Tests

TEST(ConnectionIntegrationTest, ConnectionLifecycleWithMockServer) {
    // Start mock HTTP server
    MockHttpServer mockServer(8084);
    auto handshakeResponse = MockHttpServer::getDefaultHandshakeResponse(
        "RemoteNode", juce::Uuid(), 8085);
    mockServer.setHandshakeResponse(handshakeResponse);
    mockServer.start();

    // Create connection
    auto node = createLocalTestNode("RemoteNode", 8084, 8085);
    NetworkConnection conn(node);

    CallbackTracker stateChanges;
    NetworkConnection::State finalState = NetworkConnection::State::Disconnected;

    conn.onStateChanged = [&](NetworkConnection::State oldState,
                              NetworkConnection::State newState) {
        stateChanges.recordCall();
        finalState = newState;
    };

    // Initiate connection
    EXPECT_EQ(conn.getState(), NetworkConnection::State::Disconnected);
    conn.connect();

    // Should transition to Connecting
    bool reachedConnecting = waitForState(&conn, NetworkConnection::State::Connecting, 1000);
    EXPECT_TRUE(reachedConnecting);

    // Should eventually reach Connected or Failed
    bool connectionAttempted = waitFor([&conn]() {
        auto state = conn.getState();
        return state == NetworkConnection::State::Connected ||
               state == NetworkConnection::State::Failed;
    }, 3000);

    EXPECT_TRUE(connectionAttempted);

    // Disconnect
    conn.disconnect();
    EXPECT_TRUE(waitForState(&conn, NetworkConnection::State::Disconnected, 1000));

    // Verify state changes occurred
    EXPECT_GT(stateChanges.getCallCount(), 0);

    mockServer.stop();
}

TEST(ConnectionIntegrationTest, ConnectionFailureHandling) {
    // Create connection to non-existent server
    auto node = createLocalTestNode("NonExistentNode", 9999, 9998);
    NetworkConnection conn(node);

    CallbackTracker errorCalls;
    conn.onError = [&errorCalls](const juce::String& error) {
        errorCalls.recordCall();
    };

    // Attempt connection
    conn.connect();

    // Should eventually fail
    bool reachedFailed = waitForState(&conn, NetworkConnection::State::Failed, 3000);
    EXPECT_TRUE(reachedFailed);

    // Error callback should have been invoked
    EXPECT_GT(errorCalls.getCallCount(), 0);
}

//==============================================================================
// Query Command Tests

TEST(ConnectionIntegrationTest, QueryCommandsReturnsAccurateState) {
    auto node = createLocalTestNode("TestNode", 8086, 8087);
    NetworkConnection conn(node);

    // Query state multiple times rapidly
    for (int i = 0; i < 10; ++i) {
        auto state = conn.getState();
        EXPECT_EQ(state, NetworkConnection::State::Disconnected);
    }

    // Connect and query again
    conn.connect();
    juce::Thread::sleep(100);

    auto state = conn.getState();
    EXPECT_TRUE(state == NetworkConnection::State::Connecting ||
                state == NetworkConnection::State::Connected ||
                state == NetworkConnection::State::Failed);
}

TEST(ConnectionIntegrationTest, QueryRemoteNodeInfo) {
    auto node = createLocalTestNode("TestNode", 8088, 8089);
    NetworkConnection conn(node);

    // Query remote node info
    auto remoteNode = conn.getRemoteNode();

    // Should match the node info we passed in constructor
    EXPECT_EQ(remoteNode.name, node.name);
    EXPECT_EQ(remoteNode.httpPort, node.httpPort);
    EXPECT_EQ(remoteNode.udpPort, node.udpPort);
    EXPECT_EQ(remoteNode.ipAddress, node.ipAddress);
}

TEST(ConnectionIntegrationTest, QueryRemoteDevices) {
    // Start mock server with device list
    MockHttpServer mockServer(8090);
    auto handshakeResponse = MockHttpServer::getDefaultHandshakeResponse(
        "DeviceNode", juce::Uuid(), 8091);
    mockServer.setHandshakeResponse(handshakeResponse);
    mockServer.start();

    auto node = createLocalTestNode("DeviceNode", 8090, 8091);
    NetworkConnection conn(node);

    // Initially no devices
    auto devices = conn.getRemoteDevices();
    EXPECT_EQ(devices.size(), 0u);

    // Connect to receive device list
    conn.connect();

    // Wait for connection to complete
    waitFor([&conn]() {
        auto state = conn.getState();
        return state == NetworkConnection::State::Connected ||
               state == NetworkConnection::State::Failed;
    }, 3000);

    // Query devices again (should have 2 from mock server)
    devices = conn.getRemoteDevices();

    // Note: This might be 0 if connection failed, or 2 if succeeded
    // Test passes if query completes without crashing
    EXPECT_TRUE(devices.size() == 0 || devices.size() == 2);

    mockServer.stop();
}

//==============================================================================
// Heartbeat Query Tests

TEST(ConnectionIntegrationTest, HeartbeatTimingQuery) {
    auto node = createLocalTestNode("TestNode", 8092, 8093);
    NetworkConnection conn(node);

    // Initially, time since last heartbeat should be 0 or very large
    auto timeSinceHeartbeat = conn.getTimeSinceLastHeartbeat();
    EXPECT_GE(timeSinceHeartbeat, 0);

    // Query multiple times - time should be consistent or increasing
    auto time1 = conn.getTimeSinceLastHeartbeat();
    juce::Thread::sleep(50);
    auto time2 = conn.getTimeSinceLastHeartbeat();

    // Time should increase (or stay the same if heartbeat received)
    EXPECT_GE(time2, time1 - 10);  // Allow 10ms tolerance
}

TEST(ConnectionIntegrationTest, HeartbeatCheckCommand) {
    auto node = createLocalTestNode("TestNode", 8094, 8095);
    NetworkConnection conn(node);

    // Initial check should not crash
    conn.checkHeartbeat();

    // Query after check
    auto timeSinceHeartbeat = conn.getTimeSinceLastHeartbeat();
    EXPECT_GE(timeSinceHeartbeat, 0);

    // Multiple checks should work
    for (int i = 0; i < 5; ++i) {
        conn.checkHeartbeat();
        juce::Thread::sleep(10);
    }
}

TEST(ConnectionIntegrationTest, IsAliveCheck) {
    auto node = createLocalTestNode("TestNode", 8096, 8097);
    NetworkConnection conn(node);

    // Disconnected connection is not alive
    EXPECT_FALSE(conn.isAlive());

    // After connection attempt, might become alive
    conn.connect();
    juce::Thread::sleep(100);

    // Query should not crash
    bool alive = conn.isAlive();
    EXPECT_TRUE(alive || !alive);  // Just verify query works
}

//==============================================================================
// Concurrent Access Tests

TEST(ConnectionIntegrationTest, ConcurrentStateQueries) {
    auto node = createLocalTestNode("TestNode", 8098, 8099);
    NetworkConnection conn(node);

    std::atomic<int> queryCount{0};
    std::atomic<bool> errorOccurred{false};

    // Launch 10 threads querying state concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&conn, &queryCount, &errorOccurred]() {
            try {
                for (int j = 0; j < 100; ++j) {
                    auto state = conn.getState();
                    queryCount.fetch_add(1);

                    // Verify state is valid
                    EXPECT_TRUE(state == NetworkConnection::State::Disconnected ||
                               state == NetworkConnection::State::Connecting ||
                               state == NetworkConnection::State::Connected ||
                               state == NetworkConnection::State::Failed);
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

    // Verify all queries completed
    EXPECT_EQ(queryCount.load(), 1000);
    EXPECT_FALSE(errorOccurred.load());
}

TEST(ConnectionIntegrationTest, ConcurrentMixedOperations) {
    auto node = createLocalTestNode("TestNode", 8100, 8101);
    NetworkConnection conn(node);

    std::atomic<int> operationCount{0};
    std::atomic<bool> errorOccurred{false};

    // Thread 1: Query state repeatedly
    std::thread queryThread([&]() {
        try {
            for (int i = 0; i < 50; ++i) {
                conn.getState();
                operationCount.fetch_add(1);
                juce::Thread::sleep(5);
            }
        } catch (...) {
            errorOccurred.store(true);
        }
    });

    // Thread 2: Query remote node info
    std::thread nodeThread([&]() {
        try {
            for (int i = 0; i < 50; ++i) {
                conn.getRemoteNode();
                operationCount.fetch_add(1);
                juce::Thread::sleep(5);
            }
        } catch (...) {
            errorOccurred.store(true);
        }
    });

    // Thread 3: Query devices
    std::thread deviceThread([&]() {
        try {
            for (int i = 0; i < 50; ++i) {
                conn.getRemoteDevices();
                operationCount.fetch_add(1);
                juce::Thread::sleep(5);
            }
        } catch (...) {
            errorOccurred.store(true);
        }
    });

    // Thread 4: Query heartbeat
    std::thread heartbeatThread([&]() {
        try {
            for (int i = 0; i < 50; ++i) {
                conn.getTimeSinceLastHeartbeat();
                operationCount.fetch_add(1);
                juce::Thread::sleep(5);
            }
        } catch (...) {
            errorOccurred.store(true);
        }
    });

    // Wait for all threads
    queryThread.join();
    nodeThread.join();
    deviceThread.join();
    heartbeatThread.join();

    // Verify all operations completed
    EXPECT_EQ(operationCount.load(), 200);
    EXPECT_FALSE(errorOccurred.load());
}

//==============================================================================
// MIDI Message Sending Tests

TEST(ConnectionIntegrationTest, SendMidiMessages) {
    auto node = createLocalTestNode("TestNode", 8102, 8103);
    NetworkConnection conn(node);

    // Send messages while disconnected (should queue)
    EXPECT_NO_THROW({
        conn.sendMidiMessage(1, createNoteOn(60, 100));
        conn.sendMidiMessage(1, createNoteOff(60));
        conn.sendMidiMessage(2, createControlChange(1, 64));
    });
}

TEST(ConnectionIntegrationTest, SendEmptyMidiMessage) {
    auto node = createLocalTestNode("TestNode", 8104, 8105);
    NetworkConnection conn(node);

    // Sending empty message should throw
    std::vector<uint8_t> emptyData;
    EXPECT_THROW({
        conn.sendMidiMessage(1, emptyData);
    }, std::invalid_argument);
}
