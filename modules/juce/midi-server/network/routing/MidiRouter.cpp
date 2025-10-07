/**
 * MidiRouter.cpp
 *
 * Implementation of transparent MIDI message router
 */

#include "MidiRouter.h"
#include <algorithm>

namespace NetworkMidi {

MidiRouter::MidiRouter(DeviceRegistry& registry, RoutingTable& routes)
    : deviceRegistry(registry)
    , routingTable(routes)
    , networkTransport(nullptr)
{
}

MidiRouter::~MidiRouter()
{
    clearLocalPorts();
}

//==============================================================================
// Network transport integration

void MidiRouter::setNetworkTransport(NetworkTransport* transport)
{
    networkTransport = transport;
}

//==============================================================================
// Local port management

void MidiRouter::registerLocalPort(uint16_t deviceId,
                                   std::unique_ptr<MidiPortInterface> port)
{
    std::lock_guard<std::mutex> lock(portMutex);

    if (!port) {
        reportError("Cannot register null MIDI port for device " +
                    juce::String(deviceId));
        return;
    }

    localPorts[deviceId] = std::move(port);
}

void MidiRouter::unregisterLocalPort(uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(portMutex);
    localPorts.erase(deviceId);
}

void MidiRouter::clearLocalPorts()
{
    std::lock_guard<std::mutex> lock(portMutex);
    localPorts.clear();
}

//==============================================================================
// Message transmission

void MidiRouter::sendMessage(uint16_t deviceId,
                             const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Cannot send empty MIDI message to device " +
                    juce::String(deviceId));
        return;
    }

    // Look up local route (backward compatibility - assumes local device)
    auto route = routingTable.getLocalRoute(deviceId);
    if (!route.has_value()) {
        reportError("No local route found for device " + juce::String(deviceId));
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.routingErrors++;
        return;
    }

    // Route to local device
    routeLocalMessage(deviceId, midiData);
}

void MidiRouter::sendMessageToNode(const juce::Uuid& nodeId,
                                   uint16_t deviceId,
                                   const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Cannot send empty MIDI message");
        return;
    }

    routeNetworkMessage(nodeId, deviceId, midiData);
}

//==============================================================================
// Message reception

std::vector<std::vector<uint8_t>> MidiRouter::getMessages(uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(messageMutex);

    std::vector<std::vector<uint8_t>> result;

    auto it = messageQueues.find(deviceId);
    if (it != messageQueues.end()) {
        auto& queue = it->second;

        result.reserve(queue.size());

        while (!queue.empty()) {
            result.push_back(std::move(queue.front()));
            queue.pop();
        }
    }

    return result;
}

int MidiRouter::getMessageCount(uint16_t deviceId) const
{
    std::lock_guard<std::mutex> lock(messageMutex);

    auto it = messageQueues.find(deviceId);
    if (it != messageQueues.end()) {
        return static_cast<int>(it->second.size());
    }

    return 0;
}

void MidiRouter::clearMessages(uint16_t deviceId)
{
    std::lock_guard<std::mutex> lock(messageMutex);

    auto it = messageQueues.find(deviceId);
    if (it != messageQueues.end()) {
        std::queue<std::vector<uint8_t>> empty;
        std::swap(it->second, empty);
    }
}

//==============================================================================
// Network packet handling

void MidiRouter::onNetworkPacketReceived(const juce::Uuid& sourceNode,
                                         uint16_t deviceId,
                                         const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Received empty network MIDI packet from " +
                    sourceNode.toString());
        return;
    }

    // Verify device exists in routing table (use composite key with source node)
    auto route = routingTable.getRoute(sourceNode, deviceId);
    if (!route.has_value()) {
        reportError("Received network message for unknown device " +
                    juce::String(deviceId) + " from node " + sourceNode.toString());
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.routingErrors++;
        return;
    }

    // Queue for consumption by local applications
    queueReceivedMessage(deviceId, midiData);

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.networkMessagesReceived++;
    }
}

//==============================================================================
// Statistics

MidiRouter::Statistics MidiRouter::getStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void MidiRouter::resetStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = Statistics();
}

//==============================================================================
// Error callback

void MidiRouter::setErrorCallback(ErrorCallback callback)
{
    errorCallback = std::move(callback);
}

//==============================================================================
// Private helper methods

void MidiRouter::routeLocalMessage(uint16_t deviceId,
                                   const std::vector<uint8_t>& midiData)
{
    std::lock_guard<std::mutex> lock(portMutex);

    auto it = localPorts.find(deviceId);
    if (it == localPorts.end()) {
        reportError("Local port not found for device " +
                    juce::String(deviceId));
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.routingErrors++;
        return;
    }

    try {
        it->second->sendMessage(midiData);

        // Update statistics
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.localMessagesSent++;
    }
    catch (const std::exception& e) {
        reportError("Error sending local MIDI message: " +
                    juce::String(e.what()));
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.routingErrors++;
    }
}

void MidiRouter::routeNetworkMessage(const juce::Uuid& destNode,
                                     uint16_t deviceId,
                                     const std::vector<uint8_t>& midiData)
{
    if (!networkTransport) {
        reportError("Network transport not configured - cannot route message");
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.routingErrors++;
        return;
    }

    try {
        networkTransport->sendMidiMessage(destNode, deviceId, midiData);

        // Update statistics
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.networkMessagesSent++;
    }
    catch (const std::exception& e) {
        reportError("Error sending network MIDI message: " +
                    juce::String(e.what()));
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.routingErrors++;
    }
}

void MidiRouter::queueReceivedMessage(uint16_t deviceId,
                                      const std::vector<uint8_t>& midiData)
{
    std::lock_guard<std::mutex> lock(messageMutex);

    // Create queue if it doesn't exist
    auto& queue = messageQueues[deviceId];

    // Limit queue size to prevent memory exhaustion
    const size_t maxQueueSize = 1000;
    if (queue.size() >= maxQueueSize) {
        // Drop oldest message
        queue.pop();
        reportError("Message queue overflow for device " +
                    juce::String(deviceId) + " - dropping oldest message");
    }

    queue.push(midiData);
}

void MidiRouter::reportError(const juce::String& error)
{
    if (errorCallback) {
        errorCallback(error);
    }
    // Also log to stderr for debugging
    std::cerr << "MidiRouter Error: " << error << std::endl;
}

} // namespace NetworkMidi
