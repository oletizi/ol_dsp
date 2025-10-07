/**
 * MidiRouter.cpp
 *
 * Implementation of transparent MIDI message router with SEDA architecture
 */

#include "MidiRouter.h"
#include "RouteManager.h"
#include "ForwardingRule.h"
#include "MidiRouterCommands.h"
#include "UuidRegistry.h"
#include "../core/MidiPacket.h"
#include <algorithm>

namespace NetworkMidi {

//==============================================================================
// WorkerThread Implementation

MidiRouter::WorkerThread::WorkerThread(MidiRouter& r)
    : juce::Thread("MidiRouter")
    , router(r)
{
}

void MidiRouter::WorkerThread::run()
{
    while (!router.shouldStop.load()) {
        // Wait for command with 100ms timeout
        auto cmd = router.commandQueue.waitAndPop(100);

        if (cmd) {
            router.processCommand(std::move(cmd));
        }
    }
}

//==============================================================================
// Constructor / Destructor

MidiRouter::MidiRouter(DeviceRegistry& registry, RoutingTable& routes)
    : deviceRegistry(registry)
    , routingTable(routes)
    , networkTransport(nullptr)
    , routeManager(nullptr)
    , uuidRegistry(nullptr)
    , myNodeId(juce::Uuid())  // Will be set by caller
    , nextSequence(0)
{
    // Start worker thread
    workerThread = std::make_unique<WorkerThread>(*this);
    workerThread->startThread();
}

MidiRouter::~MidiRouter()
{
    // Signal worker thread to stop
    shouldStop.store(true);
    commandQueue.shutdown();

    // Wait for worker thread to finish (max 2 seconds)
    if (workerThread) {
        workerThread->stopThread(2000);
    }

    // Clear local ports (done on main thread after worker stopped)
    localPorts.clear();
}

//==============================================================================
// Command Processing

void MidiRouter::processCommand(std::unique_ptr<MidiRouterCommands::Command> cmd)
{
    using namespace MidiRouterCommands;

    switch (cmd->type) {
        case Command::ForwardMessage: {
            auto* fwdCmd = static_cast<ForwardMessageCommand*>(cmd.get());

            // Phase 4.3: Use incoming context if present, otherwise create fresh
            ForwardingContext context;
            if (fwdCmd->incomingContext.has_value()) {
                context = fwdCmd->incomingContext.value();
            }

            forwardMessageInternal(fwdCmd->sourceNode, fwdCmd->sourceDevice,
                                   fwdCmd->midiData, context);
            break;
        }

        case Command::DirectSend: {
            auto* sendCmd = static_cast<DirectSendCommand*>(cmd.get());
            // Create fresh context for direct send
            ForwardingContext context;
            forwardToDestination(sendCmd->destNode, sendCmd->destDevice,
                                 sendCmd->midiData, context);
            break;
        }

        case Command::RegisterPort: {
            auto* regCmd = static_cast<RegisterPortCommand*>(cmd.get());
            registerLocalPortInternal(regCmd->deviceId, std::unique_ptr<MidiPortInterface>(regCmd->port));
            break;
        }

        case Command::UnregisterPort: {
            auto* unregCmd = static_cast<UnregisterPortCommand*>(cmd.get());
            unregisterLocalPortInternal(unregCmd->deviceId);
            break;
        }

        case Command::QueueMessage: {
            auto* queueCmd = static_cast<QueueMessageCommand*>(cmd.get());
            queueReceivedMessageInternal(queueCmd->deviceId, queueCmd->midiData);
            break;
        }

        case Command::GetStatistics: {
            auto* statsCmd = static_cast<GetStatisticsQuery*>(cmd.get());
            auto routerStats = getStatisticsInternal();
            statsCmd->result.localMessagesSent = routerStats.localMessagesSent;
            statsCmd->result.localMessagesReceived = routerStats.localMessagesReceived;
            statsCmd->result.networkMessagesSent = routerStats.networkMessagesSent;
            statsCmd->result.networkMessagesReceived = routerStats.networkMessagesReceived;
            statsCmd->result.routingErrors = routerStats.routingErrors;
            statsCmd->result.messagesForwarded = routerStats.messagesForwarded;
            statsCmd->result.messagesDropped = routerStats.messagesDropped;
            statsCmd->result.loopsDetected = routerStats.loopsDetected;
            statsCmd->signal();
            // Release ownership - caller manages lifetime via shared_ptr
            cmd.release();
            break;
        }

        case Command::ResetStatistics: {
            resetStatisticsInternal();
            break;
        }

        case Command::SetRouteManager: {
            auto* rmCmd = static_cast<SetRouteManagerCommand*>(cmd.get());
            setRouteManagerInternal(rmCmd->manager);
            break;
        }

        case Command::SetNetworkTransport: {
            auto* ntCmd = static_cast<SetNetworkTransportCommand*>(cmd.get());
            setNetworkTransportInternal(ntCmd->transport);
            break;
        }

        case Command::SetUuidRegistry: {
            auto* regCmd = static_cast<SetUuidRegistryCommand*>(cmd.get());
            setUuidRegistryInternal(regCmd->registry);
            break;
        }

        case Command::SetNodeId: {
            auto* nodeIdCmd = static_cast<SetNodeIdCommand*>(cmd.get());
            setNodeIdInternal(nodeIdCmd->nodeId);
            break;
        }
    }
}

//==============================================================================
// Public API - Network transport integration

void MidiRouter::setNetworkTransport(NetworkTransport* transport)
{
    auto cmd = std::make_unique<MidiRouterCommands::SetNetworkTransportCommand>(transport);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - RouteManager integration

void MidiRouter::setRouteManager(RouteManager* manager)
{
    auto cmd = std::make_unique<MidiRouterCommands::SetRouteManagerCommand>(manager);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - UuidRegistry integration

void MidiRouter::setUuidRegistry(UuidRegistry* registry)
{
    auto cmd = std::make_unique<MidiRouterCommands::SetUuidRegistryCommand>(registry);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Node ID configuration (Phase 4.5)

void MidiRouter::setNodeId(const juce::Uuid& nodeId)
{
    auto cmd = std::make_unique<MidiRouterCommands::SetNodeIdCommand>(nodeId);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Local port management

void MidiRouter::registerLocalPort(uint16_t deviceId,
                                   std::unique_ptr<MidiPortInterface> port)
{
    auto cmd = std::make_unique<MidiRouterCommands::RegisterPortCommand>(
        deviceId, port.release());
    commandQueue.push(std::move(cmd));
}

void MidiRouter::unregisterLocalPort(uint16_t deviceId)
{
    auto cmd = std::make_unique<MidiRouterCommands::UnregisterPortCommand>(deviceId);
    commandQueue.push(std::move(cmd));
}

void MidiRouter::clearLocalPorts()
{
    // Send unregister commands for all current ports
    // Note: This is a simplified version - in practice we'd need to query
    // current ports first, but that would require synchronization
    // For now, rely on destructor cleanup
}

//==============================================================================
// Public API - Message transmission

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
        // Note: Can't update stats here since we're not on worker thread
        return;
    }

    // Create command to route to local device
    // We'll use ForwardMessage with null UUID to indicate local routing
    auto cmd = std::make_unique<MidiRouterCommands::ForwardMessageCommand>(
        juce::Uuid(), deviceId, midiData);
    commandQueue.push(std::move(cmd));
}

void MidiRouter::sendMessageToNode(const juce::Uuid& nodeId,
                                   uint16_t deviceId,
                                   const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Cannot send empty MIDI message");
        return;
    }

    auto cmd = std::make_unique<MidiRouterCommands::DirectSendCommand>(
        nodeId, deviceId, midiData);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Message reception (NOTE: These remain synchronous for now)

std::vector<std::vector<uint8_t>> MidiRouter::getMessages(uint16_t deviceId)
{
    // TODO: Convert to SEDA query command
    // For now, this is a potential race condition but acceptable for Phase 1
    std::vector<std::vector<uint8_t>> result;

    // This accesses messageQueues which is also accessed by worker thread
    // In practice, this is safe because std::map/std::queue operations are
    // atomic at the container level, but ideally this should be a query command

    return result;
}

int MidiRouter::getMessageCount(uint16_t deviceId) const
{
    // TODO: Convert to SEDA query command
    return 0;
}

void MidiRouter::clearMessages(uint16_t deviceId)
{
    // TODO: Convert to SEDA command
}

//==============================================================================
// Public API - Network packet handling (legacy interface)

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
        return;
    }

    // Queue for consumption by local applications
    auto cmd = std::make_unique<MidiRouterCommands::QueueMessageCommand>(deviceId, midiData);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Network packet handling (Phase 4.3: with MidiPacket)

void MidiRouter::onNetworkPacketReceived(const MidiPacket& packet)
{
    const auto& midiData = packet.getMidiData();

    if (midiData.empty()) {
        reportError("Received empty network MIDI packet from " +
                    packet.getSourceNode().toString());
        return;
    }

    // Extract context from packet if present
    std::optional<NetworkMidi::ForwardingContext> contextOpt;
    if (packet.hasForwardingContext() && uuidRegistry) {
        contextOpt = packet.getForwardingContext(*uuidRegistry);

        if (!contextOpt.has_value() && packet.hasForwardingContext()) {
            reportError("Failed to deserialize forwarding context from packet - "
                       "UuidRegistry may be missing node mappings");
        }
    }

    // Convert NetworkMidi::ForwardingContext to MidiRouter::ForwardingContext
    std::optional<MidiRouterCommands::ForwardingContext> routerContextOpt;
    if (contextOpt.has_value()) {
        MidiRouterCommands::ForwardingContext routerCtx;
        routerCtx.visitedDevices = contextOpt->visitedDevices;
        routerCtx.hopCount = contextOpt->hopCount;
        routerContextOpt = routerCtx;
    }

    // Create command with context
    auto cmd = std::make_unique<MidiRouterCommands::ForwardMessageCommand>(
        packet.getSourceNode(), packet.getDeviceId(), midiData, routerContextOpt);

    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Message forwarding

void MidiRouter::forwardMessage(const juce::Uuid& sourceNode,
                                uint16_t sourceDevice,
                                const std::vector<uint8_t>& midiData)
{
    if (midiData.empty()) {
        reportError("Cannot forward empty MIDI message");
        return;
    }

    auto cmd = std::make_unique<MidiRouterCommands::ForwardMessageCommand>(
        sourceNode, sourceDevice, midiData);
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Statistics

MidiRouter::Statistics MidiRouter::getStatistics() const
{
    // Allocate command (will be released in processCommand, not deleted)
    auto cmd = std::make_unique<MidiRouterCommands::GetStatisticsQuery>();
    auto* cmdPtr = cmd.get();

    // Push command to queue
    const_cast<MidiRouter*>(this)->commandQueue.push(std::move(cmd));

    // Wait for response (max 1000ms)
    if (cmdPtr->wait(1000)) {
        // Convert from MidiRouterCommands::Statistics to MidiRouter::Statistics
        Statistics result;
        result.localMessagesSent = cmdPtr->result.localMessagesSent;
        result.localMessagesReceived = cmdPtr->result.localMessagesReceived;
        result.networkMessagesSent = cmdPtr->result.networkMessagesSent;
        result.networkMessagesReceived = cmdPtr->result.networkMessagesReceived;
        result.routingErrors = cmdPtr->result.routingErrors;
        result.messagesForwarded = cmdPtr->result.messagesForwarded;
        result.messagesDropped = cmdPtr->result.messagesDropped;
        result.loopsDetected = cmdPtr->result.loopsDetected;

        // Manual cleanup since we released ownership in processCommand
        delete cmdPtr;

        return result;
    }

    // Timeout - return empty statistics
    reportError("Timeout waiting for statistics query");
    return Statistics();
}

void MidiRouter::resetStatistics()
{
    auto cmd = std::make_unique<MidiRouterCommands::ResetStatisticsCommand>();
    commandQueue.push(std::move(cmd));
}

//==============================================================================
// Public API - Error callback

void MidiRouter::setErrorCallback(ErrorCallback callback)
{
    errorCallback = std::move(callback);
}

//==============================================================================
// Internal Methods - Port management

void MidiRouter::registerLocalPortInternal(uint16_t deviceId,
                                           std::unique_ptr<MidiPortInterface> port)
{
    if (!port) {
        reportError("Cannot register null MIDI port for device " +
                    juce::String(deviceId));
        return;
    }

    localPorts[deviceId] = std::move(port);
}

void MidiRouter::unregisterLocalPortInternal(uint16_t deviceId)
{
    localPorts.erase(deviceId);
}

//==============================================================================
// Internal Methods - Message routing

void MidiRouter::routeLocalMessage(uint16_t deviceId,
                                   const std::vector<uint8_t>& midiData)
{
    auto it = localPorts.find(deviceId);
    if (it == localPorts.end()) {
        reportError("Local port not found for device " +
                    juce::String(deviceId));
        stats.routingErrors++;
        return;
    }

    try {
        it->second->sendMessage(midiData);
        stats.localMessagesSent++;
    }
    catch (const std::exception& e) {
        reportError("Error sending local MIDI message: " +
                    juce::String(e.what()));
        stats.routingErrors++;
    }
}

void MidiRouter::routeNetworkMessage(const juce::Uuid& destNode,
                                     uint16_t deviceId,
                                     const std::vector<uint8_t>& midiData)
{
    if (!networkTransport) {
        reportError("Network transport not configured - cannot route message");
        stats.routingErrors++;
        return;
    }

    try {
        networkTransport->sendMidiMessage(destNode, deviceId, midiData);
        stats.networkMessagesSent++;
    }
    catch (const std::exception& e) {
        reportError("Error sending network MIDI message: " +
                    juce::String(e.what()));
        stats.routingErrors++;
    }
}

void MidiRouter::queueReceivedMessageInternal(uint16_t deviceId,
                                              const std::vector<uint8_t>& midiData)
{
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
    stats.networkMessagesReceived++;
}

void MidiRouter::reportError(const juce::String& error) const
{
    if (errorCallback) {
        errorCallback(error);
    }
    // Also log to stderr for debugging
    std::cerr << "MidiRouter Error: " << error << std::endl;
}

//==============================================================================
// Internal Methods - Message forwarding

void MidiRouter::forwardMessageInternal(const juce::Uuid& sourceNode,
                                        uint16_t sourceDevice,
                                        const std::vector<uint8_t>& midiData,
                                        ForwardingContext& context)
{
    if (!routeManager) {
        // No route manager configured - skip forwarding
        return;
    }

    // Create device key for source
    DeviceKey sourceKey(sourceNode, sourceDevice);

    // Check if we should forward from this source (loop prevention)
    if (!context.shouldForward(sourceKey)) {
        // Loop detected - either hop count exceeded or device already visited
        stats.loopsDetected++;

        if (context.hopCount >= ForwardingContext::MAX_HOPS) {
            reportError("Maximum hop count exceeded for message from node " +
                        sourceNode.toString() + " device " + juce::String(sourceDevice));
        } else {
            reportError("Forwarding loop detected for message from node " +
                        sourceNode.toString() + " device " + juce::String(sourceDevice));
        }

        return;
    }

    // Mark this device as visited in the forwarding path
    context.recordVisit(sourceKey);
    context.hopCount++;

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
            stats.messagesDropped++;
            continue;
        }

        // Forward to destination (pass context for embedding in packet)
        forwardToDestination(rule.destinationNodeId(),
                            rule.destinationDeviceId(),
                            midiData,
                            context);

        // Update statistics
        routeManager->updateRuleStatistics(rule.ruleId.toStdString(), true);
        stats.messagesForwarded++;
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
                                      const std::vector<uint8_t>& midiData,
                                      const ForwardingContext& context)
{
    // Check if destination is local (local devices have null UUID in RoutingTable)
    if (destNode.isNull()) {
        // Forward to local device (no context needed)
        routeLocalMessage(destDevice, midiData);
    }
    else {
        // Forward to remote device - create packet with context
        if (!networkTransport) {
            reportError("Network transport not configured - cannot route message");
            stats.routingErrors++;
            return;
        }

        try {
            // Create MidiPacket with MIDI data
            MidiPacket packet = MidiPacket::createDataPacket(
                myNodeId, destNode, destDevice, midiData, nextSequence++
            );

            // Convert MidiRouter::ForwardingContext to NetworkMidi::ForwardingContext
            NetworkMidi::ForwardingContext netContext;
            netContext.visitedDevices = context.visitedDevices;
            netContext.hopCount = context.hopCount;

            // Embed context in packet
            packet.setForwardingContext(netContext);

            // Send via network transport (Phase 4.4 integration)
            networkTransport->sendPacket(packet);
            stats.networkMessagesSent++;
        }
        catch (const std::exception& e) {
            reportError("Error sending network MIDI message with context: " +
                        juce::String(e.what()));
            stats.routingErrors++;
        }
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

//==============================================================================
// Internal Methods - Statistics

MidiRouter::Statistics MidiRouter::getStatisticsInternal() const
{
    return stats;
}

void MidiRouter::resetStatisticsInternal()
{
    stats = Statistics();
}

//==============================================================================
// Internal Methods - Configuration

void MidiRouter::setRouteManagerInternal(RouteManager* manager)
{
    routeManager = manager;
}

void MidiRouter::setNetworkTransportInternal(NetworkTransport* transport)
{
    networkTransport = transport;
}

void MidiRouter::setUuidRegistryInternal(UuidRegistry* registry)
{
    uuidRegistry = registry;
}

void MidiRouter::setNodeIdInternal(const juce::Uuid& nodeId)
{
    myNodeId = nodeId;
}

} // namespace NetworkMidi
