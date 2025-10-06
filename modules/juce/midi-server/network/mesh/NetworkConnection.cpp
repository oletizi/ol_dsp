/**
 * NetworkConnection.cpp
 *
 * Implementation of NetworkConnection class for peer-to-peer MIDI node connections.
 * Uses SEDA architecture with ConnectionWorker thread for all state management.
 */

#include "NetworkConnection.h"
#include "NetworkConnectionQueue.h"
#include "ConnectionWorker.h"
#include "Commands.h"

namespace NetworkMidi {

//==============================================================================
NetworkConnection::NetworkConnection(const NodeInfo& remoteNode)
    : remoteNodeInfo(remoteNode)
{
    if (!remoteNode.isValid()) {
        throw std::invalid_argument("Invalid NodeInfo provided to NetworkConnection");
    }

    // Initialize SEDA infrastructure
    commandQueue = std::make_unique<NetworkConnectionQueue>();
    worker = std::make_unique<ConnectionWorker>(
        *commandQueue,
        remoteNode,
        onStateChanged,
        onDevicesReceived,
        onMidiMessageReceived,
        onError
    );

    // Start worker thread
    worker->startThread();

    juce::Logger::writeToLog("NetworkConnection: SEDA infrastructure initialized for " +
                            remoteNode.name);
}

//==============================================================================
NetworkConnection::~NetworkConnection()
{
    // Shutdown worker thread gracefully
    if (worker && commandQueue) {
        juce::Logger::writeToLog("NetworkConnection: Shutting down worker thread for " +
                                remoteNodeInfo.name);

        // Push shutdown command
        commandQueue->pushCommand(std::make_unique<Commands::ShutdownCommand>());

        // Wait for worker to exit (with timeout)
        worker->stopThread(2000);  // 2 second timeout
    }
}

//==============================================================================
void NetworkConnection::connect()
{
    juce::Logger::writeToLog("NetworkConnection::connect() - Queuing connect command");
    commandQueue->pushCommand(std::make_unique<Commands::ConnectCommand>());
}

//==============================================================================
void NetworkConnection::disconnect()
{
    juce::Logger::writeToLog("NetworkConnection::disconnect() - Queuing disconnect command");
    commandQueue->pushCommand(std::make_unique<Commands::DisconnectCommand>());
}

//==============================================================================
NetworkConnection::State NetworkConnection::getState() const
{
    // Use query command for thread-safe state access
    auto query = std::make_unique<Commands::GetStateQuery>();
    auto* queryPtr = query.get();

    commandQueue->pushCommand(std::move(query));

    // Wait for response (with timeout)
    if (queryPtr->responseReady.wait(1000)) {
        return queryPtr->result;
    }

    // Timeout - return Disconnected as safe default
    juce::Logger::writeToLog("NetworkConnection::getState() - Query timeout, returning Disconnected");
    return State::Disconnected;
}

//==============================================================================
NodeInfo NetworkConnection::getRemoteNode() const
{
    auto query = std::make_unique<Commands::GetRemoteNodeQuery>();
    auto* queryPtr = query.get();

    commandQueue->pushCommand(std::move(query));

    if (queryPtr->responseReady.wait(1000)) {
        return queryPtr->result;
    }

    // Timeout - return initial node info
    juce::Logger::writeToLog("NetworkConnection::getRemoteNode() - Query timeout, returning initial info");
    return remoteNodeInfo;
}

//==============================================================================
std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    auto query = std::make_unique<Commands::GetDevicesQuery>();
    auto* queryPtr = query.get();

    commandQueue->pushCommand(std::move(query));

    if (queryPtr->responseReady.wait(1000)) {
        return queryPtr->result;
    }

    // Timeout - return empty vector
    juce::Logger::writeToLog("NetworkConnection::getRemoteDevices() - Query timeout, returning empty list");
    return {};
}

//==============================================================================
void NetworkConnection::sendMidiMessage(uint16_t deviceId,
                                       const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        throw std::invalid_argument("Cannot send empty MIDI message");
    }

    commandQueue->pushCommand(
        std::make_unique<Commands::SendMidiCommand>(deviceId, data)
    );
}

//==============================================================================
int64_t NetworkConnection::getTimeSinceLastHeartbeat() const
{
    // For now, we need to add a query command for this
    // or use atomic snapshot from worker
    // TODO: Add GetHeartbeatQuery command or expose atomic snapshot

    // As a temporary solution, we'll check if we're connected
    State state = getState();
    if (state != State::Connected) {
        return HEARTBEAT_TIMEOUT_MS + 1;  // Return value > timeout if not connected
    }

    // Return 0 for now (heartbeat OK)
    // This will be properly implemented with atomic snapshot or query command
    return 0;
}

//==============================================================================
bool NetworkConnection::isAlive() const
{
    return getTimeSinceLastHeartbeat() < HEARTBEAT_TIMEOUT_MS;
}

//==============================================================================
void NetworkConnection::checkHeartbeat()
{
    commandQueue->pushCommand(std::make_unique<Commands::CheckHeartbeatCommand>());
}

} // namespace NetworkMidi
