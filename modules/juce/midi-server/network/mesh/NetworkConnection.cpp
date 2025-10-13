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
#include "../routing/MidiRouter.h"
#include "../core/MidiPacket.h"
#include <iostream>

namespace NetworkMidi {

//==============================================================================
NetworkConnection::NetworkConnection(const NodeInfo& remoteNode)
    : remoteNodeInfo(remoteNode)
{
    if (!remoteNode.isValid()) {
        throw std::invalid_argument("Invalid NodeInfo provided to NetworkConnection");
    }

    // Initialize command queue (worker created lazily in connect())
    commandQueue = std::make_unique<NetworkConnectionQueue>();

    juce::Logger::writeToLog("NetworkConnection: Infrastructure initialized for " +
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
    // Create worker thread if not already created (lazy initialization)
    // This allows callbacks to be set before worker is created
    if (!worker) {
        std::cout << "===== Creating ConnectionWorker with callbacks =====" << std::endl << std::flush;

        // Wrap onMidiMessageReceived to integrate MidiRouter forwarding (Phase 3.2)
        auto wrappedMidiCallback = [this](const MidiMessage& msg) {
            // Forward to MidiRouter if configured (Phase 3.2 integration)
            if (midiRouter) {
                midiRouter->forwardMessage(remoteNodeInfo.uuid, msg.deviceId, msg.data);
            }

            // Also invoke user callback if set (backward compatibility)
            if (onMidiMessageReceived) {
                onMidiMessageReceived(msg);
            }
        };

        worker = std::make_unique<ConnectionWorker>(
            *commandQueue,
            remoteNodeInfo,
            onStateChanged,
            onDevicesReceived,
            wrappedMidiCallback,
            onError
        );
        worker->startThread();
        std::cout << "===== ConnectionWorker created and started =====" << std::endl << std::flush;
        juce::Logger::writeToLog("NetworkConnection: SEDA worker thread started for " +
                                remoteNodeInfo.name);
    }

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
    // Use cached atomic state for fast, non-blocking reads
    // This avoids the timeout issues that occur when querying the worker thread
    if (!worker || !worker->isThreadRunning()) {
        return State::Disconnected;
    }

    return worker->getCachedState();
}

//==============================================================================
NodeInfo NetworkConnection::getRemoteNode() const
{
    // SAFETY CHECK 1: Verify worker thread is running
    if (!worker || !worker->isThreadRunning()) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteNode() - Worker thread not running");
        return remoteNodeInfo;
    }

    auto query = std::make_unique<Commands::GetRemoteNodeQuery>();
    auto* queryPtr = query.get();

    commandQueue->pushCommand(std::move(query));

    // SAFETY CHECK 2: Validate timeout value
    int timeoutMs = 1000;
    if (timeoutMs <= 0 || timeoutMs > 60000) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteNode() - Invalid timeout value");
        return remoteNodeInfo;
    }

    // SAFETY CHECK 3: Wrap wait() in try-catch
    try {
        if (queryPtr->wait(timeoutMs)) {
            return queryPtr->result;
        }
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteNode() - WaitableEvent exception: " +
                                juce::String(e.what()));
        return remoteNodeInfo;
    }

    // Timeout - return initial node info
    juce::Logger::writeToLog("NetworkConnection::getRemoteNode() - Query timeout, returning initial info");
    return remoteNodeInfo;
}

//==============================================================================
std::vector<DeviceInfo> NetworkConnection::getRemoteDevices() const
{
    // SAFETY CHECK 1: Verify worker thread is running
    if (!worker || !worker->isThreadRunning()) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteDevices() - Worker thread not running");
        return {};
    }

    auto query = std::make_unique<Commands::GetDevicesQuery>();
    auto* queryPtr = query.get();

    commandQueue->pushCommand(std::move(query));

    // SAFETY CHECK 2: Validate timeout value
    int timeoutMs = 1000;
    if (timeoutMs <= 0 || timeoutMs > 60000) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteDevices() - Invalid timeout value");
        return {};
    }

    // SAFETY CHECK 3: Wrap wait() in try-catch
    try {
        if (queryPtr->wait(timeoutMs)) {
            return queryPtr->result;
        }
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("NetworkConnection::getRemoteDevices() - WaitableEvent exception: " +
                                juce::String(e.what()));
        return {};
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
void NetworkConnection::sendPacket(const MidiPacket& packet)
{
    // Phase 4: Send full packet with context via SEDA command queue
    commandQueue->pushCommand(
        std::make_unique<Commands::SendPacketCommand>(packet)
    );
}

//==============================================================================
int64_t NetworkConnection::getTimeSinceLastHeartbeat() const
{
    // Use cached heartbeat time for fast, non-blocking reads
    if (!worker || !worker->isThreadRunning()) {
        return HEARTBEAT_TIMEOUT_MS + 1;  // Return value indicating disconnected
    }

    // Get cached heartbeat time
    int64_t lastHeartbeat = worker->getCachedHeartbeatTime();
    int64_t currentTime = juce::Time::getMillisecondCounterHiRes();

    return currentTime - lastHeartbeat;
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

//==============================================================================
void NetworkConnection::setMidiRouter(MidiRouter* router)
{
    midiRouter = router;
    juce::Logger::writeToLog("NetworkConnection: MidiRouter " +
                            juce::String(router ? "enabled" : "disabled") +
                            " for " + remoteNodeInfo.name);
}

} // namespace NetworkMidi
