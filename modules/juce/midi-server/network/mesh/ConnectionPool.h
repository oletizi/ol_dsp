/**
 * ConnectionPool.h
 *
 * Manages multiple NetworkConnection instances for mesh networking.
 * Provides thread-safe access to active connections and automatic lifecycle management.
 */

#pragma once

#include "NetworkConnection.h"

#include <juce_core/juce_core.h>

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace NetworkMidi {

//==============================================================================
/**
 * ConnectionPool manages all active network connections in the mesh.
 *
 * Responsibilities:
 * - Maintains map of UUID -> NetworkConnection
 * - Prevents duplicate connections to same node
 * - Thread-safe connection lookup and iteration
 * - Automatic cleanup of failed connections
 *
 * Thread Safety:
 * All public methods are thread-safe.
 */
class ConnectionPool {
public:
    //==============================================================================
    // Constructor/Destructor
    ConnectionPool() = default;
    ~ConnectionPool() = default;

    //==============================================================================
    // Connection management

    /**
     * Adds a new connection to the pool.
     * If a connection to this node already exists, returns false.
     *
     * @param connection Unique pointer to NetworkConnection (takes ownership)
     * @return true if added, false if connection already exists
     */
    bool addConnection(std::unique_ptr<NetworkConnection> connection);

    /**
     * Removes connection for specified node.
     * Gracefully disconnects before removal.
     *
     * @param nodeId UUID of remote node
     * @return true if connection was removed, false if not found
     */
    bool removeConnection(const juce::Uuid& nodeId);

    /**
     * Gets connection for specified node.
     * Returns nullptr if no connection exists.
     * Thread-safe.
     *
     * @param nodeId UUID of remote node
     * @return Pointer to connection (not owned - don't delete!)
     */
    NetworkConnection* getConnection(const juce::Uuid& nodeId) const;

    /**
     * Checks if connection exists for specified node.
     * Thread-safe.
     *
     * @param nodeId UUID of remote node
     * @return true if connection exists
     */
    bool hasConnection(const juce::Uuid& nodeId) const;

    /**
     * Returns all active connections.
     * Thread-safe (returns copy of internal map).
     *
     * @return Vector of pointers to all connections
     */
    std::vector<NetworkConnection*> getAllConnections() const;

    /**
     * Returns all connections in specified state.
     * Thread-safe.
     *
     * @param state Desired connection state
     * @return Vector of connections in that state
     */
    std::vector<NetworkConnection*>
        getConnectionsByState(NetworkConnection::State state) const;

    /**
     * Returns number of active connections.
     * Thread-safe.
     */
    size_t getConnectionCount() const;

    /**
     * Removes all connections.
     * Gracefully disconnects all before clearing.
     */
    void clear();

    //==============================================================================
    // Connection health

    /**
     * Removes all failed or timed-out connections.
     * Called periodically by MeshManager.
     *
     * @return Number of connections removed
     */
    int removeDeadConnections();

    /**
     * Gets statistics about connection pool.
     */
    struct Statistics {
        size_t totalConnections = 0;
        size_t connectedCount = 0;
        size_t connectingCount = 0;
        size_t failedCount = 0;
        size_t disconnectedCount = 0;
    };

    Statistics getStatistics() const;

private:
    //==============================================================================
    // Member variables

    mutable std::mutex connectionsMutex;
    std::map<juce::Uuid, std::unique_ptr<NetworkConnection>> connections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionPool)
};

} // namespace NetworkMidi
