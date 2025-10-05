/**
 * HeartbeatMonitor.h
 *
 * Monitors connection health by sending periodic heartbeat packets
 * and detecting timeout conditions.
 *
 * Protocol:
 * - Send UDP heartbeat every 1 second
 * - Detect timeout after 3 missed heartbeats (3 seconds)
 * - Callback on connection loss
 */

#pragma once

#include "ConnectionPool.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <functional>

namespace NetworkMidi {

//==============================================================================
/**
 * HeartbeatMonitor manages periodic health checks for all connections.
 *
 * Responsibilities:
 * - Send heartbeat packets at regular interval (1 second)
 * - Check for heartbeat timeout on each connection (3 seconds)
 * - Notify when connections are lost
 * - Automatic cleanup of dead connections
 *
 * Threading:
 * Uses JUCE Timer on message thread for periodic checks.
 * Thread-safe for all public methods.
 */
class HeartbeatMonitor : private juce::Timer {
public:
    //==============================================================================
    // Configuration constants
    static constexpr int HEARTBEAT_INTERVAL_MS = 1000;     // Send every 1 second
    static constexpr int HEARTBEAT_TIMEOUT_MS = 3000;      // Timeout after 3 seconds

    //==============================================================================
    // Callbacks
    std::function<void(const juce::Uuid& nodeId, const juce::String& reason)>
        onConnectionLost;

    //==============================================================================
    // Constructor/Destructor
    explicit HeartbeatMonitor(ConnectionPool& pool);
    ~HeartbeatMonitor() override;

    // Non-copyable
    HeartbeatMonitor(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor& operator=(const HeartbeatMonitor&) = delete;

    //==============================================================================
    // Monitoring control

    /**
     * Starts heartbeat monitoring.
     * Begins periodic timer for sending heartbeats and checking timeouts.
     */
    void start();

    /**
     * Stops heartbeat monitoring.
     * Stops timer and clears state.
     */
    void stop();

    /**
     * Returns true if monitoring is active.
     */
    bool isRunning() const;

    //==============================================================================
    // Statistics

    /**
     * Returns total number of heartbeats sent since start.
     */
    int64_t getHeartbeatsSent() const;

    /**
     * Returns total number of timeouts detected since start.
     */
    int64_t getTimeoutsDetected() const;

    /**
     * Resets statistics counters.
     */
    void resetStatistics();

private:
    //==============================================================================
    // Timer callback (called every HEARTBEAT_INTERVAL_MS)
    void timerCallback() override;

    /**
     * Sends heartbeat to all connected nodes.
     * Called periodically by timer.
     */
    void sendHeartbeats();

    /**
     * Checks all connections for timeout.
     * Called periodically by timer.
     */
    void checkTimeouts();

    /**
     * Handles a detected connection timeout.
     */
    void handleTimeout(NetworkConnection* connection);

    //==============================================================================
    // Member variables

    ConnectionPool& connectionPool;

    std::atomic<bool> running{false};

    std::atomic<int64_t> heartbeatsSent{0};
    std::atomic<int64_t> timeoutsDetected{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeartbeatMonitor)
};

} // namespace NetworkMidi
