/**
 * NetworkConnection.h
 *
 * Represents a connection to a remote MIDI network node.
 * Manages HTTP handshake, UDP communication, and connection lifecycle.
 *
 * Connection States:
 * Disconnected -> Connecting -> Connected -> Disconnected
 *                      |             |
 *                   Failed <---------+
 *
 * SEDA Architecture (Phase B.2 Complete):
 * - All mutable state owned by ConnectionWorker thread
 * - Commands sent via lock-free queue
 * - No mutexes needed (single-threaded worker)
 * - Query commands use blocking WaitableEvent for synchronous results
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace NetworkMidi {

// Forward declarations for SEDA infrastructure
class NetworkConnectionQueue;
class ConnectionWorker;
class MidiRouter;

//==============================================================================
// Node information structure
struct NodeInfo {
    juce::Uuid uuid;
    juce::String name;
    juce::String hostname;
    juce::String ipAddress;
    int httpPort = 0;
    int udpPort = 0;
    juce::String version;
    int deviceCount = 0;

    NodeInfo() = default;

    NodeInfo(const juce::Uuid& id, const juce::String& nodeName,
             const juce::String& host, const juce::String& ip,
             int http, int udp)
        : uuid(id), name(nodeName), hostname(host), ipAddress(ip),
          httpPort(http), udpPort(udp) {}

    bool isValid() const {
        return !uuid.isNull() && httpPort > 0 && udpPort > 0;
    }
};

//==============================================================================
// Device information structure
struct DeviceInfo {
    uint16_t id = 0;
    juce::String name;
    juce::String type;  // "input" or "output"

    DeviceInfo() = default;
    DeviceInfo(uint16_t deviceId, const juce::String& deviceName,
               const juce::String& deviceType)
        : id(deviceId), name(deviceName), type(deviceType) {}
};

//==============================================================================
// MIDI message structure for network transport
struct MidiMessage {
    uint16_t deviceId = 0;
    std::vector<uint8_t> data;
    uint32_t timestampMicros = 0;

    MidiMessage() = default;
    MidiMessage(uint16_t devId, const std::vector<uint8_t>& msgData)
        : deviceId(devId), data(msgData) {}
};

//==============================================================================
/**
 * NetworkConnection manages a connection to a single remote node.
 *
 * Responsibilities:
 * - HTTP handshake to exchange UDP endpoints and device lists
 * - UDP-based MIDI message transport
 * - Connection health monitoring via heartbeat
 * - Thread-safe command queuing
 *
 * SEDA Architecture:
 * - All state owned by ConnectionWorker thread (no mutexes)
 * - Commands sent via lock-free queue
 * - Queries use blocking WaitableEvent pattern
 * - Callbacks invoked from worker thread
 *
 * Thread Safety:
 * All public methods are thread-safe (use command queue internally).
 */
class NetworkConnection {
public:
    //==============================================================================
    // Connection states
    enum class State {
        Disconnected,   // Not connected
        Connecting,     // Handshake in progress
        Connected,      // Fully connected and operational
        Failed          // Connection failed
    };

    //==============================================================================
    // Callbacks
    std::function<void(State oldState, State newState)> onStateChanged;
    std::function<void(const std::vector<DeviceInfo>&)> onDevicesReceived;
    std::function<void(const MidiMessage&)> onMidiMessageReceived;
    std::function<void(const juce::String&)> onError;

    //==============================================================================
    // Constructor/Destructor
    explicit NetworkConnection(const NodeInfo& remoteNode);
    ~NetworkConnection();

    //==============================================================================
    // Connection management

    /**
     * Initiates connection to remote node.
     * Performs HTTP handshake and establishes UDP communication.
     * Non-blocking - state changes reported via onStateChanged callback.
     */
    void connect();

    /**
     * Gracefully disconnects from remote node.
     * Stops heartbeat monitoring and closes sockets.
     */
    void disconnect();

    /**
     * Returns current connection state.
     * Thread-safe via query command.
     */
    State getState() const;

    /**
     * Returns information about the remote node.
     * Thread-safe via query command.
     */
    NodeInfo getRemoteNode() const;

    /**
     * Returns list of devices advertised by remote node.
     * Thread-safe via query command.
     */
    std::vector<DeviceInfo> getRemoteDevices() const;

    //==============================================================================
    // MIDI message transport

    /**
     * Sends MIDI message to specific device on remote node.
     * Non-blocking - message queued for UDP transmission.
     *
     * @param deviceId Remote device identifier
     * @param data MIDI message bytes (1-N bytes)
     * @throws std::invalid_argument if data is empty
     */
    void sendMidiMessage(uint16_t deviceId, const std::vector<uint8_t>& data);

    //==============================================================================
    // Connection health

    /**
     * Returns time since last heartbeat received (milliseconds).
     * Used to monitor connection health.
     */
    int64_t getTimeSinceLastHeartbeat() const;

    /**
     * Returns true if connection is alive (heartbeat within timeout).
     */
    bool isAlive() const;

    /**
     * Manually trigger heartbeat check.
     * Normally called by HeartbeatMonitor, but can be used for testing.
     */
    void checkHeartbeat();

    //==============================================================================
    // Routing integration (Phase 3.2)

    /**
     * Sets the MidiRouter for forwarding received MIDI messages.
     * Optional - if not set, messages are only delivered via onMidiMessageReceived callback.
     *
     * @param router Pointer to MidiRouter instance (not owned, must outlive NetworkConnection)
     */
    void setMidiRouter(MidiRouter* router);

private:
    //==============================================================================
    // Member variables

    // Read-only node info (initialized in constructor)
    NodeInfo remoteNodeInfo;

    //==============================================================================
    // SEDA Infrastructure
    // All mutable state is owned by ConnectionWorker thread

    std::unique_ptr<NetworkConnectionQueue> commandQueue;
    std::unique_ptr<ConnectionWorker> worker;

    // Routing integration (Phase 3.2)
    MidiRouter* midiRouter = nullptr;  // Not owned, optional

    static constexpr int64_t HEARTBEAT_TIMEOUT_MS = 3000;  // 3 seconds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkConnection)
};

//==============================================================================
// Utility function to convert State enum to string
inline juce::String toString(NetworkConnection::State state) {
    switch (state) {
        case NetworkConnection::State::Disconnected: return "Disconnected";
        case NetworkConnection::State::Connecting:   return "Connecting";
        case NetworkConnection::State::Connected:    return "Connected";
        case NetworkConnection::State::Failed:       return "Failed";
        default:                                     return "Unknown";
    }
}

} // namespace NetworkMidi
