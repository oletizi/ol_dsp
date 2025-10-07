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
#include <set>
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
 * MidiRouter - Core MIDI message forwarding engine
 *
 * Routes MIDI messages between local ports and network nodes based on
 * forwarding rules from RouteManager. Provides loop prevention, statistics
 * tracking, and filter-based message forwarding.
 *
 * Thread-safe: All public methods can be called from any thread.
 *
 * DESIGN OVERVIEW:
 * ────────────────
 * - Transparent routing: Client code doesn't care if device is local or remote
 * - Local devices: Direct JUCE MIDI port access (zero overhead)
 * - Remote devices: Network transport via UDP (Phase 4 integration)
 * - Message queuing: Buffered receive for pull-based consumption
 * - Loop prevention: Hop count limiting and visited device tracking
 *
 * USAGE EXAMPLE 1: Basic Setup
 * ────────────────────────────
 *
 *   DeviceRegistry registry;
 *   RoutingTable routingTable;
 *   RouteManager routeManager(registry);
 *   MidiRouter router(registry, routingTable);
 *
 *   // Configure router
 *   router.setRouteManager(&routeManager);
 *   router.setNetworkTransport(&networkTransport);
 *   router.setErrorCallback([](const juce::String& error) {
 *       std::cerr << "Router error: " << error << std::endl;
 *   });
 *
 *   // Register local MIDI output port
 *   auto port = std::make_unique<VirtualMidiPort>("My Synth", false);
 *   router.registerLocalPort(2, std::move(port));
 *
 * USAGE EXAMPLE 2: Forwarding Messages
 * ─────────────────────────────────────
 *
 *   // Receive MIDI from network and forward according to rules
 *   void onNetworkMidiReceived(const juce::Uuid& sourceNode,
 *                              uint16_t sourceDevice,
 *                              const std::vector<uint8_t>& midiData)
 *   {
 *       router.forwardMessage(sourceNode, sourceDevice, midiData);
 *       // MidiRouter will:
 *       //   1. Query RouteManager for destination rules
 *       //   2. Apply channel and message type filters
 *       //   3. Route to local ports or network nodes
 *       //   4. Update statistics
 *       //   5. Prevent loops (hop count & visited devices)
 *   }
 *
 * USAGE EXAMPLE 3: Monitoring Statistics
 * ───────────────────────────────────────
 *
 *   auto stats = router.getStatistics();
 *   std::cout << "Local messages: " << stats.localMessagesSent << std::endl;
 *   std::cout << "Network messages: " << stats.networkMessagesSent
 *             << std::endl;
 *   std::cout << "Loops detected: " << stats.loopsDetected << std::endl;
 *
 *   // Reset statistics (e.g., every minute)
 *   router.resetStatistics();
 *
 * USAGE EXAMPLE 4: Local MIDI Input Handling
 * ───────────────────────────────────────────
 *
 *   // When local MIDI device sends a message
 *   void handleMidiInput(const juce::MidiMessage& msg) {
 *       std::vector<uint8_t> data(msg.getRawData(),
 *                                 msg.getRawData() + msg.getRawDataSize());
 *
 *       // Send via router (triggers forwarding rules)
 *       router.sendMessage(1, data);  // deviceId = 1
 *   }
 *
 * USAGE EXAMPLE 5: Retrieving Messages for Local Devices
 * ────────────────────────────────────────────────────────
 *
 *   // Poll for messages routed to a local device
 *   auto messages = router.getMessages(2);  // deviceId = 2
 *   for (const auto& midiData : messages) {
 *       processMidiData(midiData);
 *   }
 *
 *   // Check queue depth before retrieving
 *   int pending = router.getMessageCount(2);
 *   if (pending > 100) {
 *       router.clearMessages(2);  // Prevent buffer overflow
 *   }
 *
 * PERFORMANCE CHARACTERISTICS:
 * ────────────────────────────
 * - Destination lookup: O(log N) where N = number of rules (indexed)
 * - Filter matching: O(1) per rule
 * - Loop prevention: O(log H) where H = hop count (std::set lookup)
 * - Thread contention: Minimal (short critical sections)
 * - Memory: ~50 bytes overhead per forwarded message (transient)
 *
 * LOOP PREVENTION:
 * ────────────────
 * - Max hops: 8 (prevents infinite chains)
 * - Visited devices: Tracked per message (prevents cycles)
 * - Context: Currently single-node (Phase 4 will add network-wide context)
 *
 * ERROR HANDLING:
 * ───────────────
 * - Empty messages: Logged and ignored
 * - Unknown devices: Logged and ignored
 * - Null transport: Logged and ignored
 * - All errors reported via errorCallback if set
 *
 * INTEGRATION POINTS:
 * ───────────────────
 * - DeviceRegistry: Device enumeration and lookup
 * - RoutingTable: Device-to-node mapping
 * - NetworkTransport: Network message transmission (Phase 4)
 * - VirtualMidiPort: Remote device wrapping (Phase 5)
 *
 * @see RouteManager for rule management
 * @see ForwardingRule for rule structure and filters
 * @see DeviceRegistry for device management
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
        uint64_t loopsDetected = 0;          // Loop prevention
    };

    Statistics getStatistics() const;
    void resetStatistics();

    // Error callback
    using ErrorCallback = std::function<void(const juce::String&)>;
    void setErrorCallback(ErrorCallback callback);

private:
    //==========================================================================
    // Thread Safety & Mutex Ordering Convention
    //
    // This class uses three mutexes for thread safety. To prevent deadlocks,
    // always acquire mutexes in this order:
    //
    //   1. portMutex (highest priority - protects local MIDI ports)
    //   2. messageMutex (medium priority - protects message queues)
    //   3. statsMutex (lowest priority - protects statistics)
    //
    // RULE: Never acquire a higher-numbered mutex while holding a
    //       lower-numbered one.
    //
    // Example (CORRECT):
    //   std::lock_guard<std::mutex> portLock(portMutex);      // Lock 1
    //   doSomething();
    //   std::lock_guard<std::mutex> statsLock(statsMutex);    // Lock 3
    //                                                          // (OK: 1→3)
    //
    // Example (INCORRECT - DEADLOCK RISK):
    //   std::lock_guard<std::mutex> statsLock(statsMutex);    // Lock 3
    //   doSomething();
    //   std::lock_guard<std::mutex> portLock(portMutex);      // Lock 1
    //                                                          // (BAD: 3→1)
    //
    // If you need multiple locks simultaneously, use std::scoped_lock:
    //   std::scoped_lock lock(portMutex, messageMutex, statsMutex);
    //
    //==========================================================================

    /**
     * Context for tracking message forwarding to prevent loops
     *
     * Prevents infinite forwarding loops via:
     * - Hop count limiting (max 8 hops)
     * - Visited device tracking (prevents A → B → A cycles)
     */
    struct ForwardingContext {
        std::set<DeviceKey> visitedDevices;
        uint8_t hopCount = 0;
        static constexpr uint8_t MAX_HOPS = 8;

        /**
         * Check if we should forward from this device
         * Returns false if:
         * - Hop count exceeds MAX_HOPS
         * - Device has already been visited (loop detected)
         */
        bool shouldForward(const DeviceKey& device) const {
            if (hopCount >= MAX_HOPS) return false;
            if (visitedDevices.count(device) > 0) return false;
            return true;
        }

        /**
         * Mark device as visited in the forwarding path
         */
        void recordVisit(const DeviceKey& device) {
            visitedDevices.insert(device);
        }
    };

    // References to routing infrastructure
    DeviceRegistry& deviceRegistry;
    RoutingTable& routingTable;

    // Network transport (Phase 4)
    NetworkTransport* networkTransport;

    // Route manager (Phase 3.1)
    RouteManager* routeManager;

    // Thread synchronization (acquire in this order to prevent deadlocks)
    mutable std::mutex portMutex;        // 1. Protects localPorts map
    mutable std::mutex messageMutex;     // 2. Protects messageQueues map
    mutable std::mutex statsMutex;       // 3. Protects stats structure

    // Local MIDI ports
    std::map<uint16_t, std::unique_ptr<MidiPortInterface>> localPorts;

    // Received message queues (per device)
    std::map<uint16_t, std::queue<std::vector<uint8_t>>> messageQueues;

    // Statistics
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
    void forwardMessageInternal(const juce::Uuid& sourceNode,
                                uint16_t sourceDevice,
                                const std::vector<uint8_t>& midiData,
                                ForwardingContext& context);

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
