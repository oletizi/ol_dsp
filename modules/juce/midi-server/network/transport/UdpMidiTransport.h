#pragma once

#include "../core/MidiPacket.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <functional>
#include <memory>

namespace NetworkMidi {

/**
 * UDP transport layer for network MIDI communication.
 *
 * Features:
 * - Thread-safe UDP send/receive
 * - Automatic sequence numbering
 * - Callback-based packet reception
 * - Non-blocking receive loop
 */
class UdpMidiTransport {
public:
    /**
     * Constructor.
     * @param port Port number to bind to (0 = auto-assign by OS)
     */
    explicit UdpMidiTransport(int port = 0);

    /**
     * Destructor - ensures clean shutdown.
     */
    ~UdpMidiTransport();

    // Non-copyable
    UdpMidiTransport(const UdpMidiTransport&) = delete;
    UdpMidiTransport& operator=(const UdpMidiTransport&) = delete;

    /**
     * Start the transport layer.
     * Binds to the specified port and starts the receive thread.
     * @return true if started successfully
     */
    bool start();

    /**
     * Stop the transport layer.
     * Closes the socket and stops the receive thread.
     */
    void stop();

    /**
     * Check if transport is currently running.
     */
    bool isRunning() const { return running.load(); }

    /**
     * Get the local port number.
     * @return Port number (0 if not bound)
     */
    int getPort() const { return port; }

    /**
     * Send a MIDI message to a remote node.
     * @param destNode Destination node UUID
     * @param destAddress Destination IP address and port
     * @param deviceId Target device ID
     * @param midiData MIDI message bytes
     * @return true if sent successfully
     */
    bool sendMessage(
        const juce::Uuid& destNode,
        const juce::String& destAddress,
        int destPort,
        uint16_t deviceId,
        const std::vector<uint8_t>& midiData
    );

    /**
     * Send a pre-constructed packet.
     * @param packet The packet to send
     * @param destAddress Destination IP address
     * @param destPort Destination port
     * @return true if sent successfully
     */
    bool sendPacket(
        const MidiPacket& packet,
        const juce::String& destAddress,
        int destPort
    );

    /**
     * Set the local node UUID.
     * Used as the source node for all outgoing packets.
     */
    void setNodeId(const juce::Uuid& nodeId) { myNodeId = nodeId; }

    /**
     * Get the local node UUID.
     */
    juce::Uuid getNodeId() const { return myNodeId; }

    /**
     * Callback for received packets.
     * Called on the receive thread - keep processing minimal.
     */
    std::function<void(const MidiPacket&, const juce::String& sourceAddress, int sourcePort)>
        onPacketReceived;

    /**
     * Callback for transport errors.
     */
    std::function<void(const juce::String& error)> onError;

    /**
     * Get statistics.
     */
    struct Statistics {
        uint64_t packetsSent = 0;
        uint64_t packetsReceived = 0;
        uint64_t bytesSent = 0;
        uint64_t bytesReceived = 0;
        uint64_t sendErrors = 0;
        uint64_t receiveErrors = 0;
        uint64_t invalidPackets = 0;
    };

    Statistics getStatistics() const;
    void resetStatistics();

private:
    // Receive thread function
    void receiveLoop();

    // Helper to parse address
    bool parseAddress(const juce::String& address, juce::String& outIp, int& outPort);

    // Socket
    std::unique_ptr<juce::DatagramSocket> socket;
    int port;

    // Node identity
    juce::Uuid myNodeId;

    // Sequence number for outgoing packets
    std::atomic<uint16_t> nextSequence;

    // Thread control
    std::atomic<bool> running;
    std::unique_ptr<juce::Thread> receiveThread;

    // Statistics
    mutable juce::CriticalSection statsLock;
    Statistics stats;

    // Helper class for receive thread
    class ReceiveThread : public juce::Thread {
    public:
        ReceiveThread(UdpMidiTransport& transport)
            : juce::Thread("UdpMidiTransport")
            , transport(transport)
        {
        }

        void run() override {
            transport.receiveLoop();
        }

    private:
        UdpMidiTransport& transport;
    };
};

} // namespace NetworkMidi
