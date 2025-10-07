/**
 * MeshManager.h
 *
 * Central coordinator for auto-mesh formation and management.
 * Integrates service discovery, connection pool, and heartbeat monitoring.
 *
 * Responsibilities:
 * - React to node discovery events
 * - Automatically establish connections to new nodes
 * - Manage connection lifecycle
 * - Coordinate heartbeat monitoring
 * - Provide mesh status and statistics
 */

#pragma once

#include "ConnectionPool.h"
#include "HeartbeatMonitor.h"
#include "NetworkConnection.h"
#include "../routing/UuidRegistry.h"

#include <juce_core/juce_core.h>

#include <functional>
#include <memory>
#include <mutex>

namespace NetworkMidi {

//==============================================================================
/**
 * MeshManager orchestrates the auto-mesh formation process.
 *
 * High-level workflow:
 * 1. Discovery system finds new node -> onNodeDiscovered()
 * 2. MeshManager creates NetworkConnection
 * 3. Connection performs handshake
 * 4. HeartbeatMonitor starts monitoring
 * 5. Node disappears -> onNodeRemoved()
 * 6. MeshManager removes connection
 *
 * Integration:
 * - Works with ServiceDiscovery (Phase 2) for node discovery
 * - Uses ConnectionPool to manage active connections
 * - Uses HeartbeatMonitor for health monitoring
 * - Maintains UuidRegistry for multi-hop routing (Phase 4)
 *
 * Thread Safety:
 * All public methods are thread-safe.
 */
class MeshManager {
public:
    //==============================================================================
    // Callbacks for mesh events
    std::function<void(const NodeInfo& node)> onNodeConnected;
    std::function<void(const juce::Uuid& nodeId, const juce::String& reason)>
        onNodeDisconnected;
    std::function<void(const NodeInfo& node, const juce::String& error)>
        onConnectionFailed;
    std::function<void(const juce::Uuid& nodeId, const std::vector<DeviceInfo>& devices)>
        onRemoteDevicesDiscovered;

    //==============================================================================
    // Constructor/Destructor

    /**
     * Creates MeshManager with local node identity.
     *
     * @param localNodeId UUID of this node (to avoid self-connection)
     * @param httpPort Local HTTP server port
     * @param udpPort Local UDP port
     */
    MeshManager(const juce::Uuid& localNodeId, int httpPort, int udpPort);

    ~MeshManager();

    //==============================================================================
    // Lifecycle management

    /**
     * Starts mesh manager.
     * Begins heartbeat monitoring and prepares for connections.
     */
    void start();

    /**
     * Stops mesh manager.
     * Disconnects all connections and stops monitoring.
     */
    void stop();

    /**
     * Returns true if mesh manager is running.
     */
    bool isRunning() const;

    //==============================================================================
    // Node discovery integration

    /**
     * Called when a new node is discovered via mDNS or other discovery.
     * Automatically initiates connection if not already connected.
     *
     * Thread-safe - can be called from discovery callback.
     *
     * @param node Information about discovered node
     */
    void onNodeDiscovered(const NodeInfo& node);

    /**
     * Called when a node is removed from discovery (disappeared).
     * Removes associated connection if exists.
     *
     * Thread-safe - can be called from discovery callback.
     *
     * @param nodeId UUID of removed node
     */
    void onNodeRemoved(const juce::Uuid& nodeId);

    //==============================================================================
    // Mesh status and queries

    /**
     * Returns list of all connected nodes.
     * Thread-safe.
     */
    std::vector<NodeInfo> getConnectedNodes() const;

    /**
     * Returns total number of MIDI devices across all nodes.
     * Includes local and all remote devices.
     */
    int getTotalDeviceCount() const;

    /**
     * Returns information about specific node.
     * Returns invalid NodeInfo if not found.
     */
    NodeInfo getNodeInfo(const juce::Uuid& nodeId) const;

    /**
     * Returns connection to specific node.
     * Returns nullptr if not found.
     */
    NetworkConnection* getConnection(const juce::Uuid& nodeId) const;

    /**
     * Returns mesh statistics.
     */
    struct MeshStatistics {
        size_t totalNodes = 0;
        size_t connectedNodes = 0;
        size_t connectingNodes = 0;
        size_t failedNodes = 0;
        int64_t heartbeatsSent = 0;
        int64_t timeoutsDetected = 0;
        int totalDevices = 0;
    };

    MeshStatistics getStatistics() const;

    //==============================================================================
    // Phase 4.5: UUID Registry Access

    /**
     * Returns reference to the UUID registry.
     * Used by MidiRouter for context deserialization during multi-hop routing.
     *
     * The registry maps 32-bit UUID hashes to full juce::Uuid objects,
     * enabling efficient network transmission while preserving full
     * UUID resolution capability.
     *
     * @return Reference to the UUID registry
     */
    UuidRegistry& getUuidRegistry() { return uuidRegistry; }

private:
    //==============================================================================
    // Connection lifecycle handlers

    /**
     * Creates and initiates connection to remote node.
     */
    void createConnection(const NodeInfo& node);

    /**
     * Handles connection state changes.
     */
    void handleConnectionStateChange(const juce::Uuid& nodeId,
                                     NetworkConnection::State oldState,
                                     NetworkConnection::State newState);

    /**
     * Handles connection loss detected by heartbeat monitor.
     */
    void handleConnectionLost(const juce::Uuid& nodeId, const juce::String& reason);

    //==============================================================================
    // Member variables

    juce::Uuid myNodeId;
    int myHttpPort;
    int myUdpPort;

    ConnectionPool connectionPool;
    std::unique_ptr<HeartbeatMonitor> heartbeatMonitor;

    // Phase 4.5: UUID Registry for multi-hop routing
    UuidRegistry uuidRegistry;

    std::atomic<bool> running{false};

    mutable std::mutex managerMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeshManager)
};

} // namespace NetworkMidi
