/**
 * TestHelpers.h
 *
 * Shared utilities for integration testing of NetworkConnection and MeshManager.
 * Provides test node creation, state waiting, and mock HTTP server.
 */

#pragma once

#include "../NetworkConnection.h"
#include "../../../httplib.h"
#include <juce_core/juce_core.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>

namespace NetworkMidi {
namespace TestHelpers {

//==============================================================================
/**
 * Creates a test NodeInfo with specified parameters.
 */
inline NodeInfo createTestNode(const juce::String& name,
                               const juce::String& ipAddress,
                               int httpPort,
                               int udpPort) {
    NodeInfo node;
    node.uuid = juce::Uuid();
    node.name = name;
    node.hostname = name + ".local";
    node.ipAddress = ipAddress;
    node.httpPort = httpPort;
    node.udpPort = udpPort;
    node.version = "1.0.0";
    node.deviceCount = 2;
    return node;
}

/**
 * Creates a localhost test node with specified ports.
 */
inline NodeInfo createLocalTestNode(const juce::String& name,
                                   int httpPort,
                                   int udpPort) {
    return createTestNode(name, "127.0.0.1", httpPort, udpPort);
}

//==============================================================================
/**
 * Waits for connection to reach specified state with timeout.
 *
 * @param conn Connection to monitor
 * @param expectedState State to wait for
 * @param timeoutMs Maximum time to wait in milliseconds
 * @return true if state reached within timeout, false otherwise
 */
inline bool waitForState(NetworkConnection* conn,
                        NetworkConnection::State expectedState,
                        int timeoutMs = 5000) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (elapsed >= timeoutMs) {
            return false;
        }

        if (conn->getState() == expectedState) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/**
 * Waits for condition to become true with timeout.
 *
 * @param condition Function that returns true when condition is met
 * @param timeoutMs Maximum time to wait in milliseconds
 * @return true if condition met within timeout, false otherwise
 */
inline bool waitFor(std::function<bool()> condition, int timeoutMs = 5000) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (elapsed >= timeoutMs) {
            return false;
        }

        if (condition()) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//==============================================================================
/**
 * Simple mock HTTP server for testing NetworkConnection handshake.
 * Responds to /network/handshake requests with configured JSON.
 */
class MockHttpServer {
public:
    explicit MockHttpServer(int port) : port_(port), server_(std::make_unique<httplib::Server>()) {
        setupHandlers();
    }

    ~MockHttpServer() {
        stop();
    }

    /**
     * Start HTTP server in background thread.
     */
    void start() {
        if (running_.load()) {
            return;
        }

        running_.store(true);
        serverThread_ = std::thread([this]() {
            server_->listen("127.0.0.1", port_);
        });

        // Wait for server to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * Stop HTTP server and wait for thread to exit.
     */
    void stop() {
        if (!running_.load()) {
            return;
        }

        running_.store(false);
        server_->stop();

        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }

    /**
     * Set JSON response for handshake requests.
     * Default provides valid handshake with 2 devices.
     */
    void setHandshakeResponse(const juce::String& jsonResponse) {
        handshakeResponse_ = jsonResponse.toStdString();
    }

    /**
     * Returns default successful handshake response.
     */
    static juce::String getDefaultHandshakeResponse(const juce::String& nodeName,
                                                    const juce::Uuid& nodeId,
                                                    int udpPort) {
        return juce::String::formatted(R"({
            "status": "ok",
            "node": {
                "uuid": "%s",
                "name": "%s",
                "version": "1.0.0",
                "udp_endpoint": "127.0.0.1:%d"
            },
            "devices": [
                {"id": 1, "name": "Test Input", "type": "input"},
                {"id": 2, "name": "Test Output", "type": "output"}
            ]
        })",
        nodeId.toString().toRawUTF8(),
        nodeName.toRawUTF8(),
        udpPort);
    }

    /**
     * Get number of handshake requests received.
     */
    int getHandshakeRequestCount() const {
        return handshakeCount_.load();
    }

private:
    void setupHandlers() {
        // Default handshake response
        setHandshakeResponse(getDefaultHandshakeResponse("TestNode", juce::Uuid(), 8889));

        // Handshake endpoint
        server_->Post("/network/handshake", [this](const httplib::Request& req, httplib::Response& res) {
            handshakeCount_.fetch_add(1);
            res.set_content(handshakeResponse_, "application/json");
            res.status = 200;
        });

        // Health endpoint
        server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
            res.set_content(R"({"status": "ok"})", "application/json");
            res.status = 200;
        });
    }

    int port_;
    std::unique_ptr<httplib::Server> server_;
    std::thread serverThread_;
    std::atomic<bool> running_{false};
    std::atomic<int> handshakeCount_{0};
    std::string handshakeResponse_;
};

//==============================================================================
/**
 * RAII helper to track callback invocations in tests.
 */
class CallbackTracker {
public:
    CallbackTracker() = default;

    void recordCall() {
        callCount_.fetch_add(1);
    }

    int getCallCount() const {
        return callCount_.load();
    }

    void reset() {
        callCount_.store(0);
    }

    bool waitForCalls(int expectedCount, int timeoutMs = 5000) {
        return waitFor([this, expectedCount]() {
            return callCount_.load() >= expectedCount;
        }, timeoutMs);
    }

private:
    std::atomic<int> callCount_{0};
};

//==============================================================================
/**
 * Creates sample MIDI message data for testing.
 */
inline std::vector<uint8_t> createNoteOn(uint8_t note = 60, uint8_t velocity = 100) {
    return {0x90, note, velocity};  // Note On, channel 0
}

inline std::vector<uint8_t> createNoteOff(uint8_t note = 60) {
    return {0x80, note, 0x00};  // Note Off, channel 0
}

inline std::vector<uint8_t> createControlChange(uint8_t controller = 1, uint8_t value = 64) {
    return {0xB0, controller, value};  // CC, channel 0
}

inline std::vector<uint8_t> createSysEx(size_t length = 100) {
    std::vector<uint8_t> sysex;
    sysex.push_back(0xF0);  // SysEx start
    for (size_t i = 0; i < length - 2; ++i) {
        sysex.push_back(static_cast<uint8_t>(i & 0x7F));
    }
    sysex.push_back(0xF7);  // SysEx end
    return sysex;
}

} // namespace TestHelpers
} // namespace NetworkMidi
