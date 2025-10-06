/**
 * MeshManager.cpp
 *
 * Implementation of MeshManager for auto-mesh formation.
 */

#include "MeshManager.h"

namespace NetworkMidi {

//==============================================================================
MeshManager::MeshManager(const juce::Uuid& localNodeId, int httpPort, int udpPort)
    : myNodeId(localNodeId)
    , myHttpPort(httpPort)
    , myUdpPort(udpPort)
{
    if (localNodeId.isNull()) {
        throw std::invalid_argument("MeshManager: localNodeId cannot be null");
    }

    if (httpPort <= 0 || udpPort <= 0) {
        throw std::invalid_argument("MeshManager: invalid port numbers");
    }

    // Create heartbeat monitor
    heartbeatMonitor = std::make_unique<HeartbeatMonitor>(connectionPool);

    // Set up heartbeat monitor callback
    heartbeatMonitor->onConnectionLost = [this](const juce::Uuid& nodeId,
                                                const juce::String& reason) {
        handleConnectionLost(nodeId, reason);
    };

    juce::Logger::writeToLog("MeshManager created for node " + localNodeId.toString());
}

//==============================================================================
MeshManager::~MeshManager()
{
    stop();
}

//==============================================================================
void MeshManager::start()
{
    std::lock_guard<std::mutex> lock(managerMutex);

    if (running) {
        juce::Logger::writeToLog("MeshManager already running");
        return;
    }

    running = true;

    // Start heartbeat monitoring
    heartbeatMonitor->start();

    juce::Logger::writeToLog("MeshManager started");
}

//==============================================================================
void MeshManager::stop()
{
    std::lock_guard<std::mutex> lock(managerMutex);

    if (!running) {
        return;
    }

    running = false;

    // Stop heartbeat monitoring
    heartbeatMonitor->stop();

    // Disconnect all connections
    connectionPool.clear();

    juce::Logger::writeToLog("MeshManager stopped");
}

//==============================================================================
bool MeshManager::isRunning() const
{
    return running;
}

//==============================================================================
void MeshManager::onNodeDiscovered(const NodeInfo& node)
{
    // Skip self-connection
    if (node.uuid == myNodeId) {
        juce::Logger::writeToLog("MeshManager: Ignoring self-discovery");
        return;
    }

    // Check if already connected
    if (connectionPool.hasConnection(node.uuid)) {
        juce::Logger::writeToLog("MeshManager: Already connected to " + node.name);
        return;
    }

    juce::Logger::writeToLog("MeshManager: Discovered new node: " + node.name +
                            " (" + node.ipAddress + ":" + juce::String(node.httpPort) + ")");

    // Create and initiate connection
    createConnection(node);
}

//==============================================================================
void MeshManager::onNodeRemoved(const juce::Uuid& nodeId)
{
    juce::Logger::writeToLog("MeshManager: Node removed: " + nodeId.toString());

    // Remove connection if exists
    bool removed = connectionPool.removeConnection(nodeId);

    if (removed) {
        juce::Logger::writeToLog("MeshManager: Disconnected from removed node");

        if (onNodeDisconnected) {
            onNodeDisconnected(nodeId, "Node removed from discovery");
        }
    }
}

//==============================================================================
std::vector<NodeInfo> MeshManager::getConnectedNodes() const
{
    auto connections = connectionPool.getConnectionsByState(
        NetworkConnection::State::Connected);

    std::vector<NodeInfo> nodes;
    nodes.reserve(connections.size());

    for (const auto* connection : connections) {
        if (connection) {
            nodes.push_back(connection->getRemoteNode());
        }
    }

    return nodes;
}

//==============================================================================
int MeshManager::getTotalDeviceCount() const
{
    int total = 0;

    auto connections = connectionPool.getAllConnections();

    for (auto* connection : connections) {
        if (connection && connection->getState() == NetworkConnection::State::Connected) {
            total += static_cast<int>(connection->getRemoteDevices().size());
        }
    }

    return total;
}

//==============================================================================
NodeInfo MeshManager::getNodeInfo(const juce::Uuid& nodeId) const
{
    auto* connection = connectionPool.getConnection(nodeId);

    if (connection) {
        return connection->getRemoteNode();
    }

    return NodeInfo();  // Invalid NodeInfo
}

//==============================================================================
NetworkConnection* MeshManager::getConnection(const juce::Uuid& nodeId) const
{
    return connectionPool.getConnection(nodeId);
}

//==============================================================================
MeshManager::MeshStatistics MeshManager::getStatistics() const
{
    MeshStatistics stats;

    auto poolStats = connectionPool.getStatistics();
    stats.totalNodes = poolStats.totalConnections;
    stats.connectedNodes = poolStats.connectedCount;
    stats.connectingNodes = poolStats.connectingCount;
    stats.failedNodes = poolStats.failedCount;

    stats.heartbeatsSent = heartbeatMonitor->getHeartbeatsSent();
    stats.timeoutsDetected = heartbeatMonitor->getTimeoutsDetected();

    stats.totalDevices = getTotalDeviceCount();

    return stats;
}

//==============================================================================
// Private methods
//==============================================================================

void MeshManager::createConnection(const NodeInfo& node)
{
    try {
        // Create new connection
        auto connection = std::make_unique<NetworkConnection>(node);

        // Set up callbacks
        connection->onStateChanged = [this, nodeId = node.uuid]
            (NetworkConnection::State oldState, NetworkConnection::State newState) {
            handleConnectionStateChange(nodeId, oldState, newState);
        };

        connection->onError = [this, nodeInfo = node](const juce::String& error) {
            juce::Logger::writeToLog("Connection error for " + nodeInfo.name +
                                    ": " + error);
            if (onConnectionFailed) {
                onConnectionFailed(nodeInfo, error);
            }
        };

        connection->onDevicesReceived = [nodeInfo = node]
            (const std::vector<DeviceInfo>& devices) {
            juce::Logger::writeToLog("Received " + juce::String(static_cast<int>(devices.size())) +
                                    " devices from " + nodeInfo.name);
        };

        connection->onMidiMessageReceived = [](const MidiMessage& msg) {
            // TODO: Route to local MIDI system
            juce::Logger::writeToLog("Received MIDI message: device=" +
                                    juce::String(msg.deviceId) +
                                    ", bytes=" + juce::String(static_cast<int>(msg.data.size())));
        };

        // Add to pool
        if (!connectionPool.addConnection(std::move(connection))) {
            juce::Logger::writeToLog("MeshManager: Failed to add connection to pool");
            return;
        }

        // Initiate connection
        auto* conn = connectionPool.getConnection(node.uuid);
        if (conn) {
            conn->connect();
        }

    } catch (const std::exception& e) {
        juce::Logger::writeToLog("MeshManager: Failed to create connection: " +
                                juce::String(e.what()));

        if (onConnectionFailed) {
            onConnectionFailed(node, e.what());
        }
    }
}

//==============================================================================
void MeshManager::handleConnectionStateChange(const juce::Uuid& nodeId,
                                              NetworkConnection::State oldState,
                                              NetworkConnection::State newState)
{
    juce::Logger::writeToLog("MeshManager: Connection " + nodeId.toString() +
                            " state changed: " + toString(oldState) +
                            " -> " + toString(newState));

    auto* connection = connectionPool.getConnection(nodeId);

    if (!connection) {
        return;
    }

    if (newState == NetworkConnection::State::Connected) {
        // Connection established successfully
        if (onNodeConnected) {
            onNodeConnected(connection->getRemoteNode());
        }

    } else if (newState == NetworkConnection::State::Failed) {
        // Connection failed - will be cleaned up by heartbeat monitor
        if (onConnectionFailed) {
            onConnectionFailed(connection->getRemoteNode(), "Connection failed");
        }
    }
}

//==============================================================================
void MeshManager::handleConnectionLost(const juce::Uuid& nodeId,
                                      const juce::String& reason)
{
    juce::Logger::writeToLog("MeshManager: Connection lost to " + nodeId.toString() +
                            " - " + reason);

    if (onNodeDisconnected) {
        onNodeDisconnected(nodeId, reason);
    }

    // Connection will be removed by heartbeat monitor's cleanup
}

} // namespace NetworkMidi
