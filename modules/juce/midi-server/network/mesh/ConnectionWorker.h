/**
 * ConnectionWorker.h
 *
 * Worker thread for NetworkConnection SEDA architecture.
 * Processes commands from queue in dedicated thread.
 */

#pragma once

#include "NetworkConnectionQueue.h"
#include "NetworkConnection.h"
#include "Commands.h"
#include "../../httplib.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

// Forward declarations for dual-transport MIDI infrastructure
namespace NetworkMidi {
    class RealtimeMidiBuffer;
    class RealtimeMidiTransport;
    class NonRealtimeMidiTransport;
}

namespace NetworkMidi {

//==============================================================================
/**
 * ConnectionWorker is the single-threaded event loop for NetworkConnection.
 *
 * Design Principles:
 * - Single thread owns all mutable state (no mutexes needed)
 * - Commands received via lock-free queue
 * - State queries use atomic snapshots for fast reads
 * - Complex queries use command pattern with blocking response
 *
 * Thread Safety:
 * - run() executes on worker thread
 * - All state access happens on worker thread
 * - Snapshot methods use atomics (callable from any thread)
 */
class ConnectionWorker : public juce::Thread {
public:
    /**
     * Creates worker thread for processing connection commands.
     *
     * @param queue Command queue to process
     * @param remoteNode Information about the remote node
     * @param stateCallback Called when connection state changes
     * @param devicesCallback Called when device list is received
     * @param midiCallback Called when MIDI message is received
     * @param errorCallback Called when an error occurs
     */
    explicit ConnectionWorker(NetworkConnectionQueue& queue,
                             const NodeInfo& remoteNode,
                             std::function<void(NetworkConnection::State, NetworkConnection::State)> stateCallback,
                             std::function<void(const std::vector<DeviceInfo>&)> devicesCallback,
                             std::function<void(const MidiMessage&)> midiCallback,
                             std::function<void(const juce::String&)> errorCallback);

    ~ConnectionWorker() override;

    /**
     * Main worker thread loop.
     * Processes commands until shutdown requested.
     */
    void run() override;

    /**
     * Transport statistics for monitoring performance.
     */
    struct TransportStats {
        struct {
            int numReady{0};
            int freeSpace{0};
            uint64_t dropped{0};
            uint64_t written{0};
            uint64_t read{0};
            float dropRate{0.0f};
        } realtimeBuffer;

        struct {
            uint64_t packetsSent{0};
            uint64_t packetsReceived{0};
            uint64_t sendFailures{0};
            uint64_t receiveErrors{0};
        } realtimeTransport;

        struct {
            uint64_t messagesSent{0};
            uint64_t messagesReceived{0};
            uint64_t fragmentsSent{0};
            uint64_t fragmentsReceived{0};
            uint64_t retries{0};
            uint64_t failures{0};
        } nonRealtimeTransport;
    };

    /**
     * Get transport statistics (thread-safe).
     */
    TransportStats getTransportStats() const;

private:
    //==============================================================================
    // Command processing

    /**
     * Process a single command from the queue.
     * Dispatches to appropriate handler based on command type.
     *
     * @param cmd Command to process (ownership transferred)
     */
    void processCommand(std::unique_ptr<Commands::Command> cmd);

    //==============================================================================
    // Command handlers

    void handleConnectCommand();
    void handleDisconnectCommand();
    void handleCheckHeartbeatCommand();
    void handleNotifyHeartbeatCommand();
    void handleSendMidiCommand(Commands::SendMidiCommand* cmd);

    // Query handlers
    void handleGetStateQuery(Commands::GetStateQuery* query);
    void handleGetRemoteNodeQuery(Commands::GetRemoteNodeQuery* query);
    void handleGetDevicesQuery(Commands::GetDevicesQuery* query);
    void handleGetHeartbeatQuery(Commands::GetHeartbeatQuery* query);

    //==============================================================================
    // Helper methods

    /**
     * Updates connection state and invokes callback.
     * @param newState New state to transition to
     */
    void setState(NetworkConnection::State newState);

    /**
     * Updates atomic snapshots for lock-free queries.
     */
    void updateSnapshots();

    /**
     * Handles received UDP packet.
     * @param data Packet data
     * @param size Packet size in bytes
     * @param sender Sender IP address
     * @param port Sender port number
     */
    void handleUdpPacket(const void* data, int size,
                        const juce::String& sender, int port);

    //==============================================================================
    // Member variables

    NetworkConnectionQueue& commandQueue;

    // State owned by worker thread (no mutex needed - single-threaded)
    NodeInfo remoteNodeInfo;
    std::vector<DeviceInfo> remoteDevices;
    NetworkConnection::State currentState{NetworkConnection::State::Disconnected};
    juce::int64 lastHeartbeatTime{0};
    bool running{false};

    // Network resources
    std::unique_ptr<httplib::Client> httpClient;
    std::unique_ptr<juce::DatagramSocket> udpSocket;
    juce::String localUdpEndpoint;
    juce::String remoteUdpEndpoint;
    std::vector<MidiMessage> receivedMessages;

    // Dual-transport MIDI infrastructure (Phase B.3)
    std::unique_ptr<RealtimeMidiBuffer> realtimeBuffer;
    std::unique_ptr<RealtimeMidiTransport> realtimeTransport;
    std::unique_ptr<NonRealtimeMidiTransport> nonRealtimeTransport;

    // Atomic snapshots for fast lock-free queries from external threads
    std::atomic<NetworkConnection::State> stateSnapshot{NetworkConnection::State::Disconnected};
    std::atomic<int64_t> heartbeatSnapshot{0};

    // Callbacks (passed from NetworkConnection)
    std::function<void(NetworkConnection::State, NetworkConnection::State)> onStateChanged;
    std::function<void(const std::vector<DeviceInfo>&)> onDevicesReceived;
    std::function<void(const MidiMessage&)> onMidiMessageReceived;
    std::function<void(const juce::String&)> onError;

    static constexpr int64_t HEARTBEAT_TIMEOUT_MS = 3000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionWorker)
};

} // namespace NetworkMidi
