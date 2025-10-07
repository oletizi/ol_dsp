/**
 * MidiRouter.cpp
 *
 * Implementation of transparent MIDI message router
 */

#include "MidiRouter.h"
#include "RouteManager.h"
#include "ForwardingRule.h"
#include <algorithm>

namespace NetworkMidi {

MidiRouter::MidiRouter(DeviceRegistry& registry, RoutingTable& routes)
    : deviceRegistry(registry)
    , routingTable(routes)
    , networkTransport(nullptr)
    , routeManager(nullptr)
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
// RouteManager integration (Phase 3.1)

void MidiRouter::setRouteManager(RouteManager* manager)
{
    routeManager = manager;
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

//==============================================================================
// Message forwarding (Phase 3.1)

void MidiRouter::forwardMessage(const juce::Uuid& sourceNode,
                                uint16_t sourceDevice,
                                const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Cannot forward empty MIDI message");
        return;
    }

    if (!routeManager) {
        // No route manager configured - skip forwarding
        return;
    }

    // Query RouteManager for destination rules (already sorted by priority, filtered to enabled only)
    auto rules = routeManager->getDestinations(sourceNode, sourceDevice);

    if (rules.empty()) {
        // No forwarding rules configured for this source device
        return;
    }

    // For each rule (already sorted by priority, highest first)
    for (const auto& rule : rules) {
        // Apply filters (channel, message type)
        if (!matchesFilters(rule, midiData)) {
            // Message doesn't match filters - update statistics
            routeManager->updateRuleStatistics(rule.ruleId.toStdString(), false);

            std::lock_guard<std::mutex> lock(statsMutex);
            stats.messagesDropped++;
            continue;
        }

        // Forward to destination
        forwardToDestination(rule.destinationNodeId(),
                            rule.destinationDeviceId(),
                            midiData);

        // Update statistics
        routeManager->updateRuleStatistics(rule.ruleId.toStdString(), true);

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            stats.messagesForwarded++;
        }
    }
}

bool MidiRouter::matchesFilters(const ForwardingRule& rule,
                                const std::vector<uint8_t>& midiData) const
{
    if (midiData.empty()) {
        return false;
    }

    // Extract MIDI channel from message
    uint8_t midiChannel = extractMidiChannel(midiData);

    // Check channel filter
    if (!rule.matchesChannel(midiChannel)) {
        return false;
    }

    // Extract message type
    MidiMessageType msgType = getMidiMessageType(midiData);

    // Check message type filter
    if (!rule.matchesMessageType(msgType)) {
        return false;
    }

    return true;
}

void MidiRouter::forwardToDestination(const juce::Uuid& destNode,
                                      uint16_t destDevice,
                                      const std::vector<uint8_t>& midiData)
{
    // Check if destination is local (local devices have null UUID in RoutingTable)
    if (destNode.isNull()) {
        // Forward to local device
        routeLocalMessage(destDevice, midiData);
    }
    else {
        // Forward to remote device
        routeNetworkMessage(destNode, destDevice, midiData);
    }
}

uint8_t MidiRouter::extractMidiChannel(const std::vector<uint8_t>& midiData) const
{
    if (midiData.empty()) {
        return 0;  // Invalid
    }

    const uint8_t statusByte = midiData[0];

    // Channel voice messages (0x80-0xEF) encode channel in lower nibble
    if (statusByte >= 0x80 && statusByte < 0xF0) {
        // MIDI channels are 1-16, status byte lower nibble is 0-15
        return (statusByte & 0x0F) + 1;
    }

    // System messages don't have a channel
    return 0;
}

NetworkMidi::MidiMessageType MidiRouter::getMidiMessageType(const std::vector<uint8_t>& midiData) const
{
    if (midiData.empty()) {
        return MidiMessageType::None;
    }

    const uint8_t statusByte = midiData[0];

    // System Real-Time messages (0xF8-0xFF)
    if (statusByte >= 0xF8) {
        return MidiMessageType::SystemMessage;
    }

    // System Exclusive (0xF0)
    if (statusByte == 0xF0) {
        return MidiMessageType::SystemMessage;
    }

    // System Common messages (0xF1-0xF7)
    if (statusByte >= 0xF0 && statusByte < 0xF8) {
        return MidiMessageType::SystemMessage;
    }

    // Channel voice messages - extract message type from upper nibble
    uint8_t messageType = statusByte & 0xF0;

    switch (messageType) {
        case 0x80: return MidiMessageType::NoteOff;
        case 0x90: return MidiMessageType::NoteOn;
        case 0xA0: return MidiMessageType::PolyAftertouch;
        case 0xB0: return MidiMessageType::ControlChange;
        case 0xC0: return MidiMessageType::ProgramChange;
        case 0xD0: return MidiMessageType::ChannelAftertouch;
        case 0xE0: return MidiMessageType::PitchBend;
        default:   return MidiMessageType::None;
    }
}

} // namespace NetworkMidi
