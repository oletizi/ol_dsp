/**
 * MidiRouterCommands.h
 *
 * SEDA command classes for MidiRouter operations.
 * Provides type-safe commands for the router worker thread.
 */

#pragma once

#include "../core/MidiPacket.h"  // For ForwardingContext
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <set>

namespace NetworkMidi {

// Forward declarations
class MidiRouter;
class MidiPortInterface;
class RouteManager;
class NetworkTransport;
class UuidRegistry;

namespace MidiRouterCommands {

//==============================================================================
// Statistics structure (must match MidiRouter::Statistics)
struct Statistics {
    uint64_t localMessagesSent = 0;
    uint64_t localMessagesReceived = 0;
    uint64_t networkMessagesSent = 0;
    uint64_t networkMessagesReceived = 0;
    uint64_t routingErrors = 0;
    uint64_t messagesForwarded = 0;
    uint64_t messagesDropped = 0;
    uint64_t loopsDetected = 0;
};

//==============================================================================
/**
 * Base command class for all MidiRouter commands.
 * Uses polymorphism for type-safe command dispatch.
 */
struct Command {
    enum Type {
        ForwardMessage,
        DirectSend,
        RegisterPort,
        UnregisterPort,
        QueueMessage,
        GetStatistics,
        ResetStatistics,
        SetRouteManager,
        SetNetworkTransport,
        SetUuidRegistry,
        SetNodeId
    };

    Type type;
    virtual ~Command() = default;

protected:
    explicit Command(Type t) : type(t) {}
};

//==============================================================================
// HOT PATH COMMAND - Message forwarding (async, no response)

/**
 * Forward MIDI message according to routing rules.
 * This is the hot path - called for every MIDI message.
 *
 * Phase 4.3: Now includes optional incoming context for multi-hop routing.
 */
struct ForwardMessageCommand : Command {
    juce::Uuid sourceNode;
    uint16_t sourceDevice;
    std::vector<uint8_t> midiData;
    std::optional<ForwardingContext> incomingContext;

    ForwardMessageCommand(const juce::Uuid& node, uint16_t dev,
                          const std::vector<uint8_t>& data,
                          std::optional<ForwardingContext> ctx = std::nullopt)
        : Command(ForwardMessage)
        , sourceNode(node)
        , sourceDevice(dev)
        , midiData(data)
        , incomingContext(std::move(ctx))
    {}
};

/**
 * Send MIDI message directly to a destination node/device.
 * Bypasses rule lookup and routes directly to the destination.
 */
struct DirectSendCommand : Command {
    juce::Uuid destNode;
    uint16_t destDevice;
    std::vector<uint8_t> midiData;

    DirectSendCommand(const juce::Uuid& node, uint16_t dev,
                      const std::vector<uint8_t>& data)
        : Command(DirectSend)
        , destNode(node)
        , destDevice(dev)
        , midiData(data)
    {}
};

//==============================================================================
// PORT MANAGEMENT COMMANDS (async, no response)

/**
 * Register local MIDI port for a device.
 * NOTE: Takes ownership of the port (raw pointer ownership transfer).
 */
struct RegisterPortCommand : Command {
    uint16_t deviceId;
    MidiPortInterface* port;  // Ownership transferred

    RegisterPortCommand(uint16_t id, MidiPortInterface* p)
        : Command(RegisterPort)
        , deviceId(id)
        , port(p)
    {}
};

/**
 * Unregister local MIDI port for a device.
 */
struct UnregisterPortCommand : Command {
    uint16_t deviceId;

    explicit UnregisterPortCommand(uint16_t id)
        : Command(UnregisterPort)
        , deviceId(id)
    {}
};

//==============================================================================
// MESSAGE QUEUEING COMMAND (async, no response)

/**
 * Queue received message for local device consumption.
 */
struct QueueMessageCommand : Command {
    uint16_t deviceId;
    std::vector<uint8_t> midiData;

    QueueMessageCommand(uint16_t id, const std::vector<uint8_t>& data)
        : Command(QueueMessage)
        , deviceId(id)
        , midiData(data)
    {}
};

//==============================================================================
// STATISTICS COMMANDS

/**
 * Query command for retrieving statistics (sync with response).
 * Uses atomic flag for synchronization with worker thread.
 */
struct GetStatisticsQuery : Command {
    std::atomic<bool> ready{false};
    Statistics result;

    GetStatisticsQuery() : Command(GetStatistics) {}

    void signal() {
        ready.store(true);
    }

    bool wait(int timeoutMs) {
        // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() - startTime < timeout) {
            if (ready.load()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
};

/**
 * Reset statistics counters (async, no response).
 */
struct ResetStatisticsCommand : Command {
    ResetStatisticsCommand() : Command(ResetStatistics) {}
};

//==============================================================================
// CONFIGURATION COMMANDS (async, no response)

/**
 * Set RouteManager pointer for forwarding rules.
 */
struct SetRouteManagerCommand : Command {
    RouteManager* manager;

    explicit SetRouteManagerCommand(RouteManager* mgr)
        : Command(SetRouteManager)
        , manager(mgr)
    {}
};

/**
 * Set NetworkTransport pointer for network message transmission.
 */
struct SetNetworkTransportCommand : Command {
    NetworkTransport* transport;

    explicit SetNetworkTransportCommand(NetworkTransport* t)
        : Command(SetNetworkTransport)
        , transport(t)
    {}
};

/**
 * Set UuidRegistry pointer for context deserialization.
 * Phase 4.3: Required for multi-hop routing with context preservation.
 */
struct SetUuidRegistryCommand : Command {
    UuidRegistry* registry;

    explicit SetUuidRegistryCommand(UuidRegistry* reg)
        : Command(SetUuidRegistry)
        , registry(reg)
    {}
};

/**
 * Set node ID for packet creation.
 * Phase 4.5: Required for MidiRouter to create packets with source node ID.
 */
struct SetNodeIdCommand : Command {
    juce::Uuid nodeId;

    explicit SetNodeIdCommand(const juce::Uuid& id)
        : Command(SetNodeId)
        , nodeId(id)
    {}
};

} // namespace MidiRouterCommands
} // namespace NetworkMidi
