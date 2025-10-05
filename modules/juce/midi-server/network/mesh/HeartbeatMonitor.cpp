/**
 * HeartbeatMonitor.cpp
 *
 * Implementation of HeartbeatMonitor for connection health monitoring.
 */

#include "HeartbeatMonitor.h"

namespace NetworkMidi {

//==============================================================================
HeartbeatMonitor::HeartbeatMonitor(ConnectionPool& pool)
    : connectionPool(pool)
{
}

//==============================================================================
HeartbeatMonitor::~HeartbeatMonitor()
{
    stop();
}

//==============================================================================
void HeartbeatMonitor::start()
{
    if (running) {
        juce::Logger::writeToLog("HeartbeatMonitor already running");
        return;
    }

    running = true;
    resetStatistics();

    // Start timer with HEARTBEAT_INTERVAL_MS interval
    startTimer(HEARTBEAT_INTERVAL_MS);

    juce::Logger::writeToLog("HeartbeatMonitor started (interval: " +
                            juce::String(HEARTBEAT_INTERVAL_MS) + "ms, timeout: " +
                            juce::String(HEARTBEAT_TIMEOUT_MS) + "ms)");
}

//==============================================================================
void HeartbeatMonitor::stop()
{
    if (!running) {
        return;
    }

    running = false;
    stopTimer();

    juce::Logger::writeToLog("HeartbeatMonitor stopped (sent: " +
                            juce::String(heartbeatsSent.load()) +
                            ", timeouts: " +
                            juce::String(timeoutsDetected.load()) + ")");
}

//==============================================================================
bool HeartbeatMonitor::isRunning() const
{
    return running;
}

//==============================================================================
int64_t HeartbeatMonitor::getHeartbeatsSent() const
{
    return heartbeatsSent;
}

//==============================================================================
int64_t HeartbeatMonitor::getTimeoutsDetected() const
{
    return timeoutsDetected;
}

//==============================================================================
void HeartbeatMonitor::resetStatistics()
{
    heartbeatsSent = 0;
    timeoutsDetected = 0;
}

//==============================================================================
void HeartbeatMonitor::timerCallback()
{
    if (!running) {
        return;
    }

    // Send heartbeats to all connected nodes
    sendHeartbeats();

    // Check for timeouts
    checkTimeouts();

    // Clean up dead connections
    connectionPool.removeDeadConnections();
}

//==============================================================================
void HeartbeatMonitor::sendHeartbeats()
{
    auto connections = connectionPool.getConnectionsByState(
        NetworkConnection::State::Connected);

    for (auto* connection : connections) {
        if (!connection) {
            continue;
        }

        try {
            // Send heartbeat via connection
            // Note: This will be properly implemented when UDP transport is ready
            // For now, we rely on the connection's internal heartbeat mechanism

            ++heartbeatsSent;

        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Failed to send heartbeat to " +
                                    connection->getRemoteNode().name +
                                    ": " + e.what());
        }
    }
}

//==============================================================================
void HeartbeatMonitor::checkTimeouts()
{
    auto connections = connectionPool.getConnectionsByState(
        NetworkConnection::State::Connected);

    for (auto* connection : connections) {
        if (!connection) {
            continue;
        }

        // Check if connection has timed out
        int64_t timeSinceLastHeartbeat = connection->getTimeSinceLastHeartbeat();

        if (timeSinceLastHeartbeat > HEARTBEAT_TIMEOUT_MS) {
            handleTimeout(connection);
        }
    }
}

//==============================================================================
void HeartbeatMonitor::handleTimeout(NetworkConnection* connection)
{
    if (!connection) {
        return;
    }

    NodeInfo nodeInfo = connection->getRemoteNode();
    int64_t timeSinceLastHeartbeat = connection->getTimeSinceLastHeartbeat();

    juce::Logger::writeToLog("HeartbeatMonitor: Connection timeout detected for " +
                            nodeInfo.name + " (" +
                            juce::String(timeSinceLastHeartbeat) + "ms since last heartbeat)");

    ++timeoutsDetected;

    // Notify callback
    if (onConnectionLost) {
        juce::String reason = "Heartbeat timeout (" +
                             juce::String(timeSinceLastHeartbeat) + "ms)";
        onConnectionLost(nodeInfo.uuid, reason);
    }

    // Trigger connection's own heartbeat check (will mark as failed)
    connection->checkHeartbeat();
}

} // namespace NetworkMidi
