/**
 * MidiRouter.h
 *
 * Core MIDI routing engine that transparently routes messages between
 * local and remote devices. Handles both direct (local) and network
 * (remote) message delivery.
 */

#pragma once

#include "DeviceRegistry.h"
#include "RoutingTable.h"
#include "ForwardingRule.h"
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <functional>
#include <memory>

namespace NetworkMidi {

// Forward declarations
struct MidiPacket;
class RouteManager;

/**
 * Callback interface for network message transmission
 * (Will be implemented by NetworkConnection/UdpMidiTransport)
 */
class NetworkTransport {
public:
    virtual ~NetworkTransport() = default;

    virtual void sendMidiMessage(const juce::Uuid& destNode,
                                 uint16_t deviceId,
                                 const std::vector<uint8_t>& midiData) = 0;
};

/**
 * Interface for local MIDI port abstraction
 */
class MidiPortInterface {
public:
    virtual ~MidiPortInterface() = default;

    virtual void sendMessage(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<std::vector<uint8_t>> getMessages() = 0;
    virtual juce::String getName() const = 0;
    virtual bool isInput() const = 0;
    virtual bool isOutput() const = 0;
};

/**
 * Thread-safe MIDI message router
 *
 * Design:
 * - Transparent routing: Client code doesn't care if device is local or remote
 * - Local devices: Direct JUCE MIDI port access (zero overhead)
 * - Remote devices: Network transport via UDP (Phase 4 integration)
 * - Message queuing: Buffered receive for pull-based consumption
 * - Thread-safe: All operations protected by mutex
 *
 * Integration points:
 * - DeviceRegistry: Device enumeration and lookup
 * - RoutingTable: Device-to-node mapping
 * - NetworkTransport: Network message transmission (Phase 4)
 * - VirtualMidiPort: Remote device wrapping (Phase 5)
 */
class MidiRouter {
public:
    MidiRouter(DeviceRegistry& registry, RoutingTable& routes);
    ~MidiRouter();

    // Network transport integration (Phase 4)
    void setNetworkTransport(NetworkTransport* transport);

    // RouteManager integration (Phase 3.1)
    void setRouteManager(RouteManager* manager);

    // Local port management
    void registerLocalPort(uint16_t deviceId,
                           std::unique_ptr<MidiPortInterface> port);
    void unregisterLocalPort(uint16_t deviceId);
    void clearLocalPorts();

    // Message transmission
    void sendMessage(uint16_t deviceId,
                     const std::vector<uint8_t>& midiData);

    void sendMessageToNode(const juce::Uuid& nodeId,
                           uint16_t deviceId,
                           const std::vector<uint8_t>& midiData);

    // Message reception
    std::vector<std::vector<uint8_t>> getMessages(uint16_t deviceId);
    int getMessageCount(uint16_t deviceId) const;
    void clearMessages(uint16_t deviceId);

    // Network packet handling (Phase 4 integration)
    void onNetworkPacketReceived(const juce::Uuid& sourceNode,
                                 uint16_t deviceId,
                                 const std::vector<uint8_t>& midiData);

    // Message forwarding (Phase 3.1)
    void forwardMessage(const juce::Uuid& sourceNode,
                        uint16_t sourceDevice,
                        const std::vector<uint8_t>& midiData);

    // Statistics
    struct Statistics {
        uint64_t localMessagesSent = 0;
        uint64_t localMessagesReceived = 0;
        uint64_t networkMessagesSent = 0;
        uint64_t networkMessagesReceived = 0;
        uint64_t routingErrors = 0;
        uint64_t messagesForwarded = 0;      // Phase 3.1
        uint64_t messagesDropped = 0;        // Phase 3.1
    };

    Statistics getStatistics() const;
    void resetStatistics();

    // Error callback
    using ErrorCallback = std::function<void(const juce::String&)>;
    void setErrorCallback(ErrorCallback callback);

private:
    // References to routing infrastructure
    DeviceRegistry& deviceRegistry;
    RoutingTable& routingTable;

    // Network transport (Phase 4)
    NetworkTransport* networkTransport;

    // Route manager (Phase 3.1)
    RouteManager* routeManager;

    // Local MIDI ports
    mutable std::mutex portMutex;
    std::map<uint16_t, std::unique_ptr<MidiPortInterface>> localPorts;

    // Received message queues (per device)
    mutable std::mutex messageMutex;
    std::map<uint16_t, std::queue<std::vector<uint8_t>>> messageQueues;

    // Statistics
    mutable std::mutex statsMutex;
    Statistics stats;

    // Error handling
    ErrorCallback errorCallback;

    // Helper methods
    void routeLocalMessage(uint16_t deviceId,
                           const std::vector<uint8_t>& midiData);

    void routeNetworkMessage(const juce::Uuid& destNode,
                             uint16_t deviceId,
                             const std::vector<uint8_t>& midiData);

    void queueReceivedMessage(uint16_t deviceId,
                              const std::vector<uint8_t>& midiData);

    void reportError(const juce::String& error);

    // Phase 3.1 forwarding helpers
    bool matchesFilters(const ForwardingRule& rule,
                        const std::vector<uint8_t>& midiData) const;

    void forwardToDestination(const juce::Uuid& destNode,
                              uint16_t destDevice,
                              const std::vector<uint8_t>& midiData);

    uint8_t extractMidiChannel(const std::vector<uint8_t>& midiData) const;
    MidiMessageType getMidiMessageType(const std::vector<uint8_t>& midiData) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRouter)
};

} // namespace NetworkMidi
