/**
 * ConnectionPool.cpp
 *
 * Implementation of ConnectionPool for managing multiple peer connections.
 */

#include "ConnectionPool.h"

namespace NetworkMidi {

//==============================================================================
bool ConnectionPool::addConnection(std::unique_ptr<NetworkConnection> connection)
{
    if (!connection) {
        juce::Logger::writeToLog("ConnectionPool::addConnection() - null connection");
        return false;
    }

    juce::Uuid nodeId = connection->getRemoteNode().uuid;

    std::lock_guard<std::mutex> lock(connectionsMutex);

    // Check if connection already exists
    if (connections.find(nodeId) != connections.end()) {
        juce::Logger::writeToLog("ConnectionPool::addConnection() - "
                                "connection already exists for " + nodeId.toString());
        return false;
    }

    // Add connection
    connections[nodeId] = std::move(connection);

    juce::Logger::writeToLog("ConnectionPool: Added connection to " + nodeId.toString() +
                            " (total: " + juce::String((int)connections.size()) + ")");

    return true;
}

//==============================================================================
bool ConnectionPool::removeConnection(const juce::Uuid& nodeId)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    auto it = connections.find(nodeId);
    if (it == connections.end()) {
        return false;
    }

    // Gracefully disconnect before removing
    it->second->disconnect();

    connections.erase(it);

    juce::Logger::writeToLog("ConnectionPool: Removed connection to " + nodeId.toString() +
                            " (remaining: " + juce::String((int)connections.size()) + ")");

    return true;
}

//==============================================================================
NetworkConnection* ConnectionPool::getConnection(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    auto it = connections.find(nodeId);
    if (it == connections.end()) {
        return nullptr;
    }

    return it->second.get();
}

//==============================================================================
bool ConnectionPool::hasConnection(const juce::Uuid& nodeId) const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    return connections.find(nodeId) != connections.end();
}

//==============================================================================
std::vector<NetworkConnection*> ConnectionPool::getAllConnections() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    std::vector<NetworkConnection*> result;
    result.reserve(connections.size());

    for (const auto& pair : connections) {
        result.push_back(pair.second.get());
    }

    return result;
}

//==============================================================================
std::vector<NetworkConnection*>
ConnectionPool::getConnectionsByState(NetworkConnection::State state) const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    std::vector<NetworkConnection*> result;

    for (const auto& pair : connections) {
        if (pair.second->getState() == state) {
            result.push_back(pair.second.get());
        }
    }

    return result;
}

//==============================================================================
size_t ConnectionPool::getConnectionCount() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    return connections.size();
}

//==============================================================================
void ConnectionPool::clear()
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    juce::Logger::writeToLog("ConnectionPool: Clearing all connections (" +
                            juce::String((int)connections.size()) + ")");

    // Disconnect all connections before clearing
    for (auto& pair : connections) {
        pair.second->disconnect();
    }

    connections.clear();
}

//==============================================================================
int ConnectionPool::removeDeadConnections()
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    int removedCount = 0;
    auto it = connections.begin();

    while (it != connections.end()) {
        NetworkConnection* conn = it->second.get();
        NetworkConnection::State state = conn->getState();

        // Remove if failed or timed out
        bool shouldRemove = (state == NetworkConnection::State::Failed) ||
                           (state == NetworkConnection::State::Connected && !conn->isAlive());

        if (shouldRemove) {
            juce::Logger::writeToLog("ConnectionPool: Removing dead connection to " +
                                    it->first.toString() + " (state: " +
                                    toString(state) + ")");

            conn->disconnect();
            it = connections.erase(it);
            ++removedCount;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        juce::Logger::writeToLog("ConnectionPool: Removed " + juce::String(removedCount) +
                                " dead connections");
    }

    return removedCount;
}

//==============================================================================
ConnectionPool::Statistics ConnectionPool::getStatistics() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    Statistics stats;
    stats.totalConnections = connections.size();

    for (const auto& pair : connections) {
        switch (pair.second->getState()) {
            case NetworkConnection::State::Connected:
                ++stats.connectedCount;
                break;
            case NetworkConnection::State::Connecting:
                ++stats.connectingCount;
                break;
            case NetworkConnection::State::Failed:
                ++stats.failedCount;
                break;
            case NetworkConnection::State::Disconnected:
                ++stats.disconnectedCount;
                break;
        }
    }

    return stats;
}

} // namespace NetworkMidi
